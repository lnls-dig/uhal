#include <inttypes.h>

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <thread>

#include <argparse/argparse.hpp>

#include "decoders.h"
#include "defer.h"
#include "pcie.h"
#include "pcie-open.h"
#include "util.h"
#include "util_sdb.h"

#include "modules/acq.h"
#include "modules/ad9510.h"
#include "modules/afc_timing.h"
#include "modules/bpm_swap.h"
#include "modules/fmcpico1m_4ch.h"
#include "modules/fmc_active_clk.h"
#include "modules/fmc_adc_common.h"
#include "modules/fmc250m_4ch.h"
#include "modules/fofb_cc.h"
#include "modules/fofb_processing.h"
#include "modules/fofb_shaper_filt.h"
#include "modules/isla216p.h"
#include "modules/lamp.h"
#include "modules/orbit_intlk.h"
#include "modules/pos_calc.h"
#include "modules/si57x_ctrl.h"
#include "modules/spi.h"
#include "modules/sysid.h"
#include "modules/trigger_iface.h"
#include "modules/trigger_mux.h"

using namespace std::chrono_literals;

static void try_unsigned(unsigned &dest, const argparse::ArgumentParser &args, const char *flag)
{
    if (auto v = args.present<unsigned>(flag))
        dest = *v;
}

int main(int argc, char *argv[])
{
    /* argparse doesn't support subcommands, so we simulate them here */
    if (argc < 2) {
        fputs(
            "Usage: decode-reg mode <mode specific options>\n\n"
            "Positional arguments:\n"
            "mode      mode of operation ('reset', 'build_info', 'sdb', 'decode', 'ram', 'acq', 'lamp', 'pos_calc', 'si57x', 'fmc_active_clk', 'fmc250m_4ch', 'spi')\n",
            stderr);
        return 1;
    }
    std::string mode = argv[1];

    argparse::ArgumentParser parent_args("decode-reg", "1.0", argparse::default_arguments::none);
    parent_args.add_argument("--slot").help("device slot");
    parent_args.add_argument("--address").help("device address");
    parent_args.add_argument("--serial").help("device serial port");
    parent_args.add_argument("-a").help("enumerated position of device").required().scan<'u', unsigned>().default_value((unsigned)0);
    parent_args.add_argument("-v").help("verbose output").default_value(false).implicit_value(true);

    argparse::ArgumentParser parent_args_with_help("decode-reg <action>", "1.0", argparse::default_arguments::help);
    parent_args_with_help.add_parents(parent_args);

    argparse::ArgumentParser build_info_args("decode-reg build_info", "1.0", argparse::default_arguments::help);
    build_info_args.add_parents(parent_args);
    build_info_args.add_argument("-q").help("quiet: print only main synthesis name").default_value(false).implicit_value(true);

    argparse::ArgumentParser decode_args("decode-reg decode", "1.0", argparse::default_arguments::help);
    decode_args.add_parents(parent_args);
    decode_args.add_argument("-q").help("type of registers").required();
    decode_args.add_argument("-c").help("channel number").scan<'u', unsigned>();
    decode_args.add_argument("-w").help("watch registers").default_value(false).implicit_value(true);
    decode_args.add_argument("-t").help("time register reading").default_value(false).implicit_value(true);
    decode_args.add_argument("-B").help("binary dump").default_value(false).implicit_value(true);

    argparse::ArgumentParser ram_args("decode-reg ram", "1.0", argparse::default_arguments::help);
    ram_args.add_parents(parent_args);
    ram_args.add_argument("-f").help("first address").required().scan<'x', unsigned>();
    ram_args.add_argument("-l").help("last address").required().scan<'x', unsigned>();

    argparse::ArgumentParser acq_args("decode-reg acq", "1.0", argparse::default_arguments::help);
    acq_args.add_parents(parent_args);
    acq_args.add_argument("-c").help("channel number").required().scan<'u', unsigned>();
    acq_args.add_argument("-n").help("number of pre samples").required().scan<'u', unsigned>();
    acq_args.add_argument("-p").help("number of post samples").scan<'u', unsigned>();
    acq_args.add_argument("-s").help("number of shots").scan<'u', unsigned>();
    acq_args.add_argument("-t").help("trigger type");
    acq_args.add_argument("-e").help("data trigger threshold").scan<'u', unsigned>();
    acq_args.add_argument("-l").help("use negative edge data trigger").default_value(false).implicit_value(true);
    acq_args.add_argument("-z").help("data trigger selection").scan<'u', unsigned>();
    acq_args.add_argument("-i").help("data trigger filter").scan<'u', unsigned>();
    acq_args.add_argument("-C").help("data trigger channel").scan<'u', unsigned>();
    acq_args.add_argument("-d").help("trigger delay").scan<'u', unsigned>();

    argparse::ArgumentParser lamp_args("decode-reg lamp", "1.0", argparse::default_arguments::help);
    lamp_args.add_parents(parent_args);
    lamp_args.add_argument("-e").help("enable amplifier for channel").default_value(false).implicit_value(true);
    lamp_args.add_argument("-c").help("channel number").required().scan<'u', unsigned>();
    lamp_args.add_argument("-m").help("choose output mode. use 'help' to see available options").required();
    lamp_args.add_argument("-k").help("pi_kp value").scan<'u', unsigned>();
    lamp_args.add_argument("-t").help("pi_ti value").scan<'u', unsigned>();
    lamp_args.add_argument("-s").help("pi_sp value").scan<'d', int>();
    lamp_args.add_argument("-d").help("DAC value").scan<'d', int>();
    lamp_args.add_argument("-l").help("Limit A").scan<'d', int>();
    lamp_args.add_argument("-L").help("Limit B").scan<'d', int>();
    lamp_args.add_argument("-C").help("Count value").scan<'u', unsigned>();
    lamp_args.add_argument("-T").help("Trigger enable").scan<'u', unsigned>();

    argparse::ArgumentParser si57x_args("decode-reg si57x", "1.0", argparse::default_arguments::help);
    si57x_args.add_parents(parent_args);
    si57x_args.add_argument("-s").help("startup frequency").required().scan<'f', double>();
    si57x_args.add_argument("-f").help("desired frequency").required().scan<'f', double>();

    argparse::ArgumentParser spi_args("decode-reg spi", "1.0", argparse::default_arguments::help);
    spi_args.add_parents(parent_args);
    spi_args.add_argument("-q").help("type of SPI device").required();
    spi_args.add_argument("-r").help("register address").scan<'x', uint8_t>().required();
    spi_args.add_argument("-w").help("register value").scan<'x', uint8_t>();
    spi_args.add_argument("-d").help("set default parameters").default_value(false).implicit_value(true);
    spi_args.add_argument("-s").help("slave select").default_value((unsigned)0).scan<'d', unsigned>().required();

    argparse::ArgumentParser *pargs;
    if (mode == "reset" || mode == "sdb" || mode == "pos_calc" || mode == "fmc_active_clk" || mode == "fmc250m_4ch") {
        pargs = &parent_args_with_help;
    } else if (mode == "build_info") {
        pargs = &build_info_args;
    } else if (mode == "decode") {
        pargs = &decode_args;
    } else if (mode == "ram") {
        pargs = &ram_args;
    } else if (mode == "acq") {
        pargs = &acq_args;
    } else if (mode == "lamp") {
        pargs = &lamp_args;
    } else if (mode == "si57x") {
        pargs = &si57x_args;
    } else if (mode == "spi") {
        pargs = &spi_args;
    } else {
        fprintf(stderr, "Unsupported type: '%s'\n", mode.c_str());
        return 1;
    }
    argparse::ArgumentParser &args = *pargs;

    try {
        args.parse_args(argc-1, argv+1);
    } catch (const std::runtime_error &err) {
        fprintf(stderr, "argparse error: %s\n", err.what());
        return 1;
    }

    auto dev_index = args.get<unsigned>("-a");
    auto verbose = args.is_used("-v");

    struct pcie_bars bars;
    if (auto v = args.present<std::string>("--slot")) dev_open_slot(bars, v->c_str());
    else if (auto v = args.present<std::string>("--address")) dev_open(bars, v->c_str());
    else if (auto v = args.present<std::string>("--serial")) dev_open_serial(bars, v->c_str());
    else {
        fputs("no device specified (--slot, --address, --serial)\n", stderr);
        return 1;
    }

    defer _(nullptr, [&bars](...){dev_close(bars);});

    if (mode == "reset") {
        device_reset(&bars);
        return 0;
    }
    if (mode == "build_info") {
        auto build_info = get_synthesis_info(&bars);

        if (build_info.empty()) {
            fputs("no SDB was found\n", stderr);
            return 1;
        }

        if (args.is_used("-q"))
            puts(build_info[0].name);
        else
            for (auto &b: build_info) {
                printf("name %s\n", b.name);
                printf("    commit-id %s\n", b.commit);
                if (b.tool_name[0]) printf("    tool-name %s\n", b.tool_name);
                if (b.tool_version) printf("    tool-version 0x%08x\n", (unsigned)b.tool_version);
                if (b.date) printf("    date 0x%08x\n", (unsigned)b.date);
                if (b.user_name[0]) printf("    user-name %s\n", b.user_name);
            }

        return 0;
    }
    if (mode == "sdb") {
        read_sdb(&bars, nullptr, 0);
        return 0;
    }

    if (mode == "decode") {
        auto type = args.get<std::string>("-q");
        std::unique_ptr<RegisterDecoder> dec;
        if (type == "acq") {
            dec = std::make_unique<acq::Core>(bars);
        } else if (type == "timing") {
            dec = std::make_unique<afc_timing::Core>(bars);
        } else if (type == "lamp") {
            dec = std::make_unique<lamp::Core>(bars);
        } else if (type == "fmcpico1m_4ch") {
            dec = std::make_unique<fmcpico1m_4ch::Core>(bars);
        } else if (type == "fmc_active_clk") {
            dec = std::make_unique<fmc_active_clk::Core>(bars);
        } else if (type == "fmc_adc_common") {
            dec = std::make_unique<fmc_adc_common::Core>(bars);
        } else if (type == "fmc250m_4ch") {
            dec = std::make_unique<fmc250m_4ch::Core>(bars);
        } else if (type == "fofb_cc") {
            dec = std::make_unique<fofb_cc::Core>(bars);
        } else if (type == "fofb_processing") {
            dec = std::make_unique<fofb_processing::Core>(bars);
        } else if (type == "fofb_shaper_filt") {
            dec = std::make_unique<fofb_shaper_filt::Core>(bars);
        } else if (type == "sys_id") {
            dec = std::make_unique<sys_id::Core>(bars);
        } else if (type == "trigger_iface") {
            dec = std::make_unique<trigger_iface::Core>(bars);
        } else if (type == "trigger_mux") {
            dec = std::make_unique<trigger_mux::Core>(bars);
        } else if (type == "bpm_swap") {
            dec = std::make_unique<bpm_swap::Core>(bars);
        } else if (type == "orbit_intlk") {
            dec = std::make_unique<orbit_intlk::Core>(bars);
        } else if (type == "pos_calc") {
            dec = std::make_unique<pos_calc::Core>(bars);
        } else if (type == "si57x_ctrl") {
            dec = std::make_unique<si57x_ctrl::Core>(bars);
        } else if (type == "spi") {
            dec = std::make_unique<spi::Core>(bars);
        } else {
            fprintf(stderr, "Unknown type: '%s'\n", type.c_str());
            return 1;
        }

        dec->channel = args.present<unsigned>("-c");

        if (auto d = read_sdb(&bars, dec->match_devinfo_lambda, dev_index)) {
            if (verbose) {
                fprintf(stdout, "Found device in %08jx\n", (uintmax_t)d->start_addr);
            }
            dec->set_devinfo(*d);

            if (args.is_used("-B")) {
                dec->get_data();
                dec->binary_dump(stdout);
                return 0;
            }

            bool watch = args.is_used("-w");
            bool time_watch = args.is_used("-t");
            uintmax_t loop_count = 0;
            auto ti = std::chrono::high_resolution_clock::now();
            do {
                if (watch) {
                    fprintf(stderr, "read #%ju\n", loop_count++);
                    if (time_watch && loop_count % 1000 == 999) {
                        auto tf = std::chrono::high_resolution_clock::now();
                        auto d = tf - ti;
                        ti = tf;
                        uintmax_t dv = d / 1ms;
                        fprintf(stderr, "1k iterations took %ju ms\n", dv);
                    }
                }

                dec->get_data();

                if (type == "pos_calc") {
                    /* force module to actually read these registers */
                    auto dec_pos_calc = dynamic_cast<pos_calc::Core *>(dec.get());
                    dec_pos_calc->fifo_empty();
                    dec_pos_calc->get_fifo_amps();
                }

                dec->print(stdout, verbose);
            } while (watch);
        } else {
            fprintf(stderr, "Couldn't find %s module index %u\n", type.c_str(), dev_index);
            return 1;
        }
    }
    if (mode == "ram") {
        auto first = args.get<unsigned>("-f");
        auto last = args.get<unsigned>("-l");

        auto space = last - first;
        std::vector<uint32_t> samples((space) / 4);
        bar2_read_v(&bars, first, samples.data(), space);
        for (auto v: samples) printf("%u ", v);
    }
    if (mode == "acq") {
        acq::Controller ctl{bars};
        if (auto v = read_sdb(&bars, ctl.match_devinfo_lambda, dev_index)) {
            ctl.set_devinfo(*v);
        } else {
            fprintf(stderr, "Couldn't find acq module index %u\n", dev_index);
            return 1;
        }
        ctl.channel = args.get<unsigned>("-c");
        ctl.pre_samples = args.get<unsigned>("-n");
        try_unsigned(ctl.post_samples, args, "-p");
        try_unsigned(ctl.number_shots, args, "-s");
        if (auto v = args.present<std::string>("-t")) ctl.trigger_type = *v;

        try_unsigned(ctl.data_trigger_threshold, args, "-e");
        ctl.data_trigger_polarity_neg = args.is_used("-l");
        try_unsigned(ctl.data_trigger_sel, args, "-z");
        try_unsigned(ctl.data_trigger_filt, args, "-i");
        try_unsigned(ctl.data_trigger_channel, args, "-C");
        try_unsigned(ctl.trigger_delay, args, "-d");

        auto res = ctl.result<int32_t>();
        ctl.print_csv(stdout, res);
    }
    if (mode == "lamp") {
        lamp::Controller ctl{bars};
        if (auto v = read_sdb(&bars, ctl.match_devinfo_lambda, dev_index)) {
            ctl.set_devinfo(*v);
        } else {
            fprintf(stderr, "Couldn't find lamp module index %u\n", dev_index);
            return 1;
        }

        auto channel = args.get<unsigned>("-c");

        ctl.write_channel("AMP_EN", channel, args.is_used("-e"));
        ctl.write_channel("MODE", channel, (int32_t)get_index(args.get<std::string>("-m"), lamp::mode_list));
        ctl.write_channel("PI_KP", channel, (int32_t)args.get<unsigned>("-k"));
        ctl.write_channel("PI_TI", channel, (int32_t)args.get<unsigned>("-t"));
        ctl.write_channel("PI_SP", channel, args.get<int>("-s"));

        ctl.write_channel("DAC", channel, args.get<int>("-d"));
        ctl.write_channel("LIMIT_A", channel, args.get<int>("-l"));
        ctl.write_channel("LIMIT_B", channel, args.get<int>("-L"));
        ctl.write_channel("CNT", channel, args.get<int>("-C"));

        ctl.write_channel("TRIG_EN", channel, (int32_t)args.get<unsigned>("-T"));

        ctl.write_params();
    }
    if (mode == "pos_calc") {
        pos_calc::Core dec(bars);
        if (auto v = read_sdb(&bars, dec.match_devinfo_lambda, dev_index)) {
            dec.set_devinfo(*v);
        } else {
            fprintf(stderr, "Couldn't find pos_calc module index %u\n", dev_index);
            return 1;
        }

        dec.get_data();

        while (true) {
            while (dec.fifo_empty()) std::this_thread::sleep_for(50ms);
            dec.get_fifo_amps();

            dec.print(stdout, false);
        }
    }
    if (mode == "si57x") {
        si57x_ctrl::Core dec(bars);
        si57x_ctrl::Controller ctl(bars, args.get<double>("-s"));

        if (auto v = read_sdb(&bars, dec.match_devinfo_lambda, dev_index)) {
            dec.set_devinfo(*v);
            ctl.set_devinfo(*v);
        } else {
            fprintf(stderr, "Couldn't find si57x_ctrl module index %u\n", dev_index);
            return 1;
        }

        puts("Original values...");
        dec.get_data();
        dec.print(stdout, true);

        ctl.read_startup_regs();

        puts("\nAfter read_startup_regs...");
        dec.get_data();
        dec.print(stdout, true);

        ctl.set_freq(args.get<double>("-f"));
        ctl.apply_config();

        puts("\nAfter apply_config...");
        dec.get_data();
        dec.print(stdout, true);
    }
    if (mode == "fmc_active_clk") {
        fmc_active_clk::Controller ctl(bars);

        if (auto v = read_sdb(&bars, ctl.match_devinfo_lambda, dev_index)) {
            ctl.set_devinfo(*v);
        } else {
            fprintf(stderr, "Couldn't find fmc_active_clk module index %u\n", dev_index);
            return 1;
        }

        ctl.write_general("SI571_OE", 1);
        ctl.write_general("PLL_FUNCTION", 1);
        ctl.write_general("CLK_SEL", 1);

        ctl.write_params();
    }
    if (mode == "fmc250m_4ch") {
        fmc250m_4ch::Controller ctl(bars);

        if (auto v = read_sdb(&bars, ctl.match_devinfo_lambda, dev_index)) {
            ctl.set_devinfo(*v);
        } else {
            fprintf(stderr, "Couldn't find fmc250m_4ch module index %u\n", dev_index);
            return 1;
        }

        ctl.write_general("RST_ADCS", 1);
        ctl.write_general("RST_DIV_ADCS", 1);
        ctl.write_general("SLEEP_ADCS", 0);

        ctl.write_params();
    }
    if (mode == "spi") {
        auto spi_fn = [&bars, &args, dev_index](auto &ctl, const auto &name) {
            spi::Channel channel{args.get<unsigned>("-s")};

            if (auto v = read_sdb(&bars, [&ctl](auto const &d){ return ctl.match_devinfo(d); }, dev_index)) {
                ctl.set_devinfo(*v);
            } else {
                fprintf(stderr, "Couldn't find %s module index %u\n", name.c_str(), dev_index);
                std::exit(1);
            }

            if (args.is_used("-d"))
                if (!ctl.set_defaults(channel))
                    fputs("set_defaults error\n", stderr);

            auto reg_addr = args.get<uint8_t>("-r");

            if (args.present<uint8_t>("-w"))
                if (!ctl.set_reg(reg_addr, args.get<uint8_t>("-w"), channel))
                    fputs("set_reg error\n", stderr);

            uint8_t reg_read;
            if (ctl.get_reg(reg_addr, reg_read, channel))
                printf("reg@%#02x: %#02x\n", reg_addr, reg_read);
            else
                fputs("error when calling get_reg\n", stderr);
        };

        auto dev_type = args.get<std::string>("-q");
        if (dev_type == "ad9510") {
            ad9510::Controller ctl(bars);
            spi_fn(ctl, dev_type);
        } else if (dev_type == "isla216p") {
            isla216p::Controller ctl(bars);
            spi_fn(ctl, dev_type);
        }
    }

    return 0;
}
