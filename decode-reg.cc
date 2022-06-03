/*
 * Copyright (C) 2022 CNPEM (cnpem.br)
 * Author: Érico Nogueira <erico.rolim@lnls.br>
 *
 * Released according to the GNU GPL, version 3 or any later version.
 */

#include <inttypes.h>

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
#include "acq.h"
#include "lamp.h"

static void try_unsigned(unsigned &dest, const argparse::ArgumentParser &args, const char *flag)
{
    if (auto v = args.present<unsigned>(flag))
        dest = *v;
}

int main(int argc, char *argv[])
{
    /* argparse doesn't support subcommands, so we simulate them here */
    if (argc < 2) {
        fputs("Usage: decode-reg mode <mode specific options>\n\nPositional arguments:\nmode      mode of operation ('decode', 'acq' or 'lamp')\n", stderr);
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
    lamp_args.add_argument("-d").help("DAC value").scan<'u', unsigned>();
    lamp_args.add_argument("-l").help("Limit A").scan<'d', int>();
    lamp_args.add_argument("-L").help("Limit B").scan<'d', int>();
    lamp_args.add_argument("-C").help("Count value").scan<'u', unsigned>();

    argparse::ArgumentParser *pargs;
    if (mode == "decode") {
        pargs = &decode_args;
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

    struct pcie_bars bars = dev_open_slot(device_number.c_str());
    defer _(nullptr, [&bars](...){dev_close(bars);});

    if (verbose) read_sdb(&bars, print_sdb);

    if (mode == "decode") {
        auto type = args.get<std::string>("-q");
        std::unique_ptr<RegisterDecoder> dec;
        if (type == "acq") {
            dec = std::make_unique<LnlsBpmAcqCore>();
        } else if (type == "lamp") {
            dec = std::make_unique<LnlsRtmLampCoreV1>();
            /* if v1 can't be found, try v2;
             * assumes only one version of the device will be available */
            if (!read_sdb(&bars, dec->device_match, dev_index)) {
                dec = std::make_unique<LnlsRtmLampCoreV2>();
            }
        } else {
            fprintf(stderr, "Unknown type: '%s'\n", type.c_str());
            return 1;
        }
        if (auto d = read_sdb(&bars, dec->device_match, dev_index)) {
            if (verbose) {
                fprintf(stdout, "Found device in %08jx\n", (uintmax_t)d->start_addr);
            }
            dec->read(&bars, d->start_addr);
            dec->print(stdout, verbose);
        }
    }
    if (mode == "acq") {
        LnlsBpmAcqCoreController ctl{&bars};
        if (auto v = read_sdb(&bars, ctl.device_match, dev_index)) {
            ctl.set_addr(v->start_addr);
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

        auto res = std::get<std::vector<int32_t>>(ctl.result(data_sign::d_signed));
        for (auto &v: res)
            fprintf(stdout, "%d\n", (int)v);
    }
    if (mode == "lamp") {
        std::unique_ptr<LnlsRtmLampController> ctlp = std::make_unique<LnlsRtmLampControllerV1>(&bars);
        if (!read_sdb(&bars, ctlp->device_match, dev_index)) {
            ctlp = std::make_unique<LnlsRtmLampControllerV2>(&bars);
            if (!read_sdb(&bars, ctlp->device_match, dev_index)) {
                fprintf(stderr, "Couldn't find lamp module index %u\n", dev_index);
                return 1;
            }
        }
        LnlsRtmLampController &ctl = *ctlp;
        ctl.set_addr(read_sdb(&bars, ctl.device_match, dev_index)->start_addr);

        ctl.amp_enable = args.is_used("-e");
        ctl.mode = args.get<std::string>("-m");
        ctl.channel = args.get<unsigned>("-c");
        ctl.pi_kp = args.present<unsigned>("-k");
        ctl.pi_ti = args.present<unsigned>("-t");
        ctl.pi_sp = args.present<int>("-s");

        ctl.dac = args.present<unsigned>("-d");
        ctl.limit_a = args.present<int>("-l");
        ctl.limit_b = args.present<int>("-L");
        ctl.cnt = args.present<unsigned>("-C");

        ctl.write_params();
    }

    return 0;
}
