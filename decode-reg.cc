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

static const char usage[] =
R"(Simple FPGA Register Reader

Usage:
    decode-reg [-hv] -b <device> -a <base_address>

-h                 Display this usage information
-v                 Verbose output
-b <device>        Device number as used in /dev/fpga-N
-a <base_address>  Base address to start reads from (in hex)
)";

int main(int argc, char *argv[])
{
    auto args = docopt::docopt(usage, {argv+1, argv+argc}, true);

    bool verbose = args["-v"].asBool();
    std::string address_str = args["<base_address>"].asString();
    size_t address = std::stol(address_str, nullptr, 16);
    int device_number = args["<device>"].asLong();

    pciDriver::PciDevice dev{device_number};
    dev.open();

    struct pcie_bars bars = { .bar0 = dev.mapBAR (0), .bar4 = dev.mapBAR(4) };

    if (verbose) {
        fprintf (stdout, "BAR 0 host address = %p\n", bars.bar0);
        fprintf (stdout, "BAR 4 host address = %p\n", bars.bar4);
    }

    std::unique_ptr<RegisterDecoder> dec{new LnlsBpmAcqCore};
    dec->read(&bars, address);
    dec->print(stdout, verbose);

    return 0;
}
