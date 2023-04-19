/*
 * Copyright (C) 2022 CNPEM (cnpem.br)
 * Author: Érico Nogueira <erico.rolim@lnls.br>
 *
 * Released according to the GNU GPL, version 3 or any later version.
 */

#include <inttypes.h>

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>

#include <argparse/argparse.hpp>

#include "decoders.h"
#include "defer.h"
#include "pcie.h"
#include "pcie-open.h"
#include "util_sdb.h"
#include "modules/acq.h"
#include "modules/fofb_cc.h"
#include "modules/fofb_processing.h"
#include "modules/lamp.h"
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
            "mode      mode of operation ('reset', 'build_info', 'decode', 'ram', 'acq', 'lamp')\n",
            stderr);
        return 1;
    }
    std::string mode = argv[1];

    argparse::ArgumentParser parent_args("decode-reg", "1.0", argparse::default_arguments::none);
    parent_args.add_argument("-b").help("device identifier").required();
    parent_args.add_argument("-a").help("enumerated position of device").required().scan<'u', unsigned>().default_value((unsigned)0);
    parent_args.add_argument("-v").help("verbose output").default_value(false).implicit_value(true);

    argparse::ArgumentParser decode_args("decode-reg decode", "1.0", argparse::default_arguments::help);
    decode_args.add_parents(parent_args);
    decode_args.add_argument("-q").help("type of registers").required();
    decode_args.add_argument("-c").help("channel number").scan<'u', unsigned>();
    decode_args.add_argument("-w").help("watch registers").default_value(false).implicit_value(true);
    decode_args.add_argument("-t").help("time register reading").default_value(false).implicit_value(true);

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
    if (mode == "reset" || mode == "build_info") {
        pargs = &parent_args;
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

    auto device_number = args.get<std::string>("-b");
    auto dev_index = args.get<unsigned>("-a");
    auto verbose = args.is_used("-v");

    struct pcie_bars bars;
    dev_open_slot(bars, device_number.c_str());
    defer _(nullptr, [&bars](...){dev_close(bars);});

    if (mode == "reset") {
        device_reset(&bars);
        return 0;
    }
    if (mode == "build_info") {
        auto build_info = get_synthesis_info(&bars);

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
        } else if (type == "lamp") {
            dec = std::make_unique<lamp::CoreV2>(bars);
        } else if (type == "fofb_cc") {
            dec = std::make_unique<fofb_cc::Core>(bars);
        } else if (type == "fofb_processing") {
            dec = std::make_unique<fofb_processing::Core>(bars);
        } else if (type == "sys_id") {
            dec = std::make_unique<sys_id::Core>(bars);
        } else if (type == "trigger_iface") {
            dec = std::make_unique<trigger_iface::Core>(bars);
        } else if (type == "trigger_mux") {
            dec = std::make_unique<trigger_mux::Core>(bars);
        } else {
            fprintf(stderr, "Unknown type: '%s'\n", type.c_str());
            return 1;
        }

        dec->channel = args.present<unsigned>("-c");

        if (auto d = read_sdb(&bars, dec->device_match, dev_index)) {
            if (verbose) {
                fprintf(stdout, "Found device in %08jx\n", (uintmax_t)d->start_addr);
            }
            dec->set_devinfo(*d);

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

                dec->read();
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
        if (auto v = read_sdb(&bars, ctl.device_match, dev_index)) {
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
        lamp::ControllerV2 ctl{bars};
        if (auto v = read_sdb(&bars, ctl.device_match, dev_index)) {
            ctl.set_devinfo(*v);
        } else {
            fprintf(stderr, "Couldn't find lamp module index %u\n", dev_index);
            return 1;
        }

        ctl.amp_enable = args.is_used("-e");
        ctl.mode = args.get<std::string>("-m");
        ctl.channel = args.get<unsigned>("-c");
        ctl.pi_kp = args.present<unsigned>("-k");
        ctl.pi_ti = args.present<unsigned>("-t");
        ctl.pi_sp = args.present<int>("-s");

        ctl.dac = args.present<int>("-d");
        ctl.limit_a = args.present<int>("-l");
        ctl.limit_b = args.present<int>("-L");
        ctl.cnt = args.present<unsigned>("-C");

        if (auto trigger_enable = args.present<unsigned>("-T")) {
            ctl.trigger_enable = *trigger_enable;
        }

        ctl.write_params();
    }

    return 0;
}
