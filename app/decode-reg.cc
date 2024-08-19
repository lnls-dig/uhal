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
#include "modules/afc_timing.h"
#include "modules/bpm_swap.h"
#include "modules/fmcpico1m_4ch.h"
#include "modules/fofb_cc.h"
#include "modules/fofb_processing.h"
#include "modules/fofb_shaper_filt.h"
#include "modules/lamp.h"
#include "modules/orbit_intlk.h"
#include "modules/pos_calc.h"
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
            "mode      mode of operation ('reset', 'build_info', 'decode', 'ram', 'acq', 'lamp', 'timing', 'pos_calc')\n",
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

    argparse::ArgumentParser *pargs;
    if (mode == "reset" || mode == "timing" || mode == "pos_calc") {
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

    if (verbose) read_sdb(&bars, print_sdb);

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
    if (mode == "timing") {
        afc_timing::Controller ctl(bars);
        if (auto v = read_sdb(&bars, ctl.match_devinfo_lambda, dev_index)) {
            ctl.set_devinfo(*v);
        } else {
            fprintf(stderr, "Couldn't find timing module index %u\n", dev_index);
            return 1;
        }

        ctl.event_receiver_enable = true;

        ctl.set_rtm_freq(499667736.71/4.);
        ctl.set_afc_freq(499667736.71/4. * 5./9.);

        ctl.afc_clock.freq_loop.kp = 1;
        ctl.afc_clock.freq_loop.ki = 1500;
        ctl.afc_clock.phase_loop.kp = 10;
        ctl.afc_clock.phase_loop.ki = 1;

        ctl.afc_clock.ddmtd_config.div_exp = 1;
        ctl.afc_clock.ddmtd_config.navg = 7;

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

    return 0;
}
