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

#include <pciDriver/lib/PciDevice.h>

#include "docopt.h"

#include "decoders.h"
#include "pcie.h"
#include "acq.h"
#include "lamp.h"

static const char usage[] =
R"(Simple FPGA Register Reader

Usage:
    decode-reg [-hv] -q <device_type> -b <device> -a <base_address>
    decode-reg [-hv] -r -b <device> -a <base_address> -c <channel> -n <pre_samples> -p <post_samples> -s <shots> [-t <trigger>] [-e <data_trigger_threshold>] [-l] [-z <data_trigger_selection>] [-i <data_trigger_filter>] [-C <data_trigger_channel>] [-d <trigger_delay>]
    decode-reg [-hv] -x -b <device> -a <base_address> [-e] -c <channel> -m <mode> -k <pi_kp> -t <pi_ti> -s <pi_sp> [-d <dac>]

  -h                 Display this usage information
  -v                 Verbose output
  -r                 Perform read acquisition
  -l                 Use negative edge data trigger
  -b <device>        Device number as used in /dev/fpga-N
  -a <base_address>  Base address to start reads from (in hex)
  -c <channel>       Channel to be used for acquisition
  -x                 Set RTM LAMP parameters
  -e                 Enable amplifier for channel - for RTM LAMP
  -m                 Choose output mode for RTM LAMP. Should be "none" if using -d argument
)";


static void try_long(unsigned &dest, const docopt::Options &args, const char *flag, const char *parameter)
{
    if (args.at(flag))
        dest = args.at(parameter).asLong();
}

int main(int argc, char *argv[])
{
    const docopt::Options args = docopt::docopt(usage, {argv+1, argv+argc}, true);

    bool verbose = args.at("-v").asBool();
    std::string address_str = args.at("<base_address>").asString();
    size_t address = std::stol(address_str, nullptr, 16);
    int device_number = args.at("<device>").asLong();

    pciDriver::PciDevice dev{device_number};
    dev.open();

    struct pcie_bars bars = {dev.mapBAR(0), dev.mapBAR(2), dev.mapBAR(4)};

    if (verbose) {
        fprintf (stdout, "BAR 0 host address = %p\n", bars.bar0);
        fprintf (stdout, "BAR 4 host address = %p\n", bars.bar4);
    }

    if (args.at("-r").asBool()) {
        LnlsBpmAcqCoreController ctl{&bars, address};
        ctl.channel = args.at("<channel>").asLong();
        ctl.pre_samples = args.at("<pre_samples>").asLong();
        ctl.post_samples = args.at("<post_samples>").asLong();
        ctl.number_shots = args.at("<shots>").asLong();
        if (args.at("-t")) ctl.trigger_type = args.at("<trigger>").asString();
        try_long(ctl.data_trigger_threshold, args, "-e", "<data_trigger_threshold>");
        ctl.data_trigger_polarity_neg = args.at("-l").asBool();
        try_long(ctl.data_trigger_sel, args, "-z", "<data_trigger_selection>");
        try_long(ctl.data_trigger_filt, args, "-i", "<data_trigger_filter>");
        try_long(ctl.data_trigger_channel, args, "-C", "<data_trigger_channel>");
        try_long(ctl.trigger_delay, args, "-d", "<trigger_delay>");

        ctl.device = &dev;

        auto res = std::get<std::vector<int32_t>>(ctl.result(data_sign::d_signed));
        for (auto &v: res)
            fprintf(stdout, "%d\n", (int)v);
    } else if (args.at("-x").asBool()) {
        LnlsRtmLampController ctl(&bars, address);

        ctl.amp_enable = args.at("-e").asBool();
        ctl.mode = args.at("<mode>").asString();
        ctl.channel = args.at("<channel>").asLong();
        ctl.pi_kp = args.at("<pi_kp>").asLong();
        ctl.pi_ti = args.at("<pi_ti>").asLong();
        ctl.pi_sp = args.at("<pi_sp>").asLong();

        if (args.at("-d").asBool()) {
            ctl.dac_data = true;
            ctl.dac = args.at("<dac>").asLong();
        }

        ctl.write_params();
    } else {
        std::string device_type = args.at("<device_type>").asString();

        std::unique_ptr<RegisterDecoder> dec;
        if (device_type == "acq") {
            dec = std::make_unique<LnlsBpmAcqCore>();
        } else if (device_type == "lamp") {
            dec = std::make_unique<LnlsRtmLampCore>();
        } else {
            fprintf(stderr, "Unknown device: '%s'\n", device_type.c_str());
            return 1;
        }
        dec->read(&bars, address);
        dec->print(stdout, verbose);
    }

    return 0;
}
