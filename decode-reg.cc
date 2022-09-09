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
#include "pcie.h"
#include "pcie-open.h"
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
    parent_args.add_argument("-b").help("device number").required().scan<'d', int>();
    parent_args.add_argument("-a").help("base address").required().scan<'x', size_t>().default_value((size_t)0);
    parent_args.add_argument("-v").help("verbose output").implicit_value(true);

    argparse::ArgumentParser decode_args("decode-reg decode", "2.0");
    decode_args.add_parents(parent_args);
    decode_args.add_argument("-q").help("type of registers").required();

    argparse::ArgumentParser acq_args("decode-reg acq");
    acq_args.add_parents(parent_args);
    acq_args.add_argument("-c").help("channel number").required().scan<'d', unsigned>();
    acq_args.add_argument("-n").help("number of pre samples").required().scan<'d', unsigned>();
    acq_args.add_argument("-p").help("number of post samples").scan<'d', unsigned>();
    acq_args.add_argument("-s").help("number of shots").scan<'d', unsigned>();
    acq_args.add_argument("-t").help("trigger type");
    acq_args.add_argument("-e").help("data trigger threshold").scan<'d', unsigned>();
    acq_args.add_argument("-l").help("use negative edge data trigger").implicit_value(true);
    acq_args.add_argument("-z").help("data trigger selection").scan<'d', unsigned>();
    acq_args.add_argument("-i").help("data trigger filter").scan<'d', unsigned>();
    acq_args.add_argument("-C").help("data trigger channel").scan<'d', unsigned>();
    acq_args.add_argument("-d").help("trigger delay").scan<'d', unsigned>();

    argparse::ArgumentParser lamp_args("decode-reg lamp");
    lamp_args.add_parents(parent_args);
    lamp_args.add_argument("-e").help("enable amplifier for channel").implicit_value(true);
    lamp_args.add_argument("-c").help("channel number").required().scan<'d', unsigned>();
    lamp_args.add_argument("-m").help("choose output mode. should be 'none' if using -d").required();
    lamp_args.add_argument("-k").help("pi_kp value").required().scan<'d', unsigned>();
    lamp_args.add_argument("-t").help("pi_ti value").required().scan<'d', unsigned>();
    lamp_args.add_argument("-s").help("pi_sp value").required().scan<'d', unsigned>();
    lamp_args.add_argument("-d").help("DAC value").scan<'d', unsigned>();

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

    auto device_number = args.get<int>("-b");
    auto address = args.get<size_t>("-a");
    auto verbose = args.is_used("-v");

    struct pcie_bars bars = dev_open(device_number);

    if (mode == "decode") {
        auto type = args.get<std::string>("-q");
        std::unique_ptr<RegisterDecoder> dec;
        if (type == "acq") {
            dec = std::make_unique<LnlsBpmAcqCore>();
        } else if (type == "lamp") {
            dec = std::make_unique<LnlsRtmLampCoreV1>();
        } else {
            fprintf(stderr, "Unknown type: '%s'\n", type.c_str());
            return 1;
        }
        dec->read(&bars, address);
        dec->print(stdout, verbose);
    }
    if (mode == "acq") {
        LnlsBpmAcqCoreController ctl{&bars, address};
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
        LnlsRtmLampController ctl(&bars, address);
        ctl.amp_enable = args.is_used("-e");
        ctl.mode = args.get<std::string>("-m");
        ctl.channel = args.get<unsigned>("-c");
        ctl.pi_kp = args.get<unsigned>("-p");
        ctl.pi_ti = args.get<unsigned>("-t");
        ctl.pi_sp = args.get<unsigned>("-s");

        if (auto v = args.present<unsigned>("-d")) {
            ctl.dac_data = true;
            ctl.dac = *v;
        }

        ctl.write_params();
    }

    return 0;
}
