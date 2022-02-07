/*
 * Copyright (C) 2022 CNPEM (cnpem.br)
 * Author: Érico Nogueira <erico.rolim@lnls.br>
 *
 * Released according to the GNU GPL, version 3 or any later version.
 */

#include <inttypes.h>
#include <unistd.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <pciDriver/lib/PciDevice.h>

extern "C" {
#include "pcie.h"
}
#include "wb_acq_core_regs.h"

static void print_help (char *program_name)
{
    fprintf (stdout, "Simple FPGA Register Reader\n"
            "Usage: %s [options]\n"
            "  -h                        Display this usage information\n"
            "  -b <Device Number>        Device number as used in /dev/fpga-N\n"
            "  -v                        Verbose output\n"
            "  -a <FPGA Address in hex>  Base address to start reads from\n",
            program_name);
}

int main(int argc, char *argv[])
{
    int verbose = 0;
    char *device_number_str = NULL;
    char *address_str = NULL;
    int opt;

    while ((opt = getopt (argc, argv, "hb:va:")) != -1) {
        /* Get the user selected options */
        switch (opt) {
            /* Display Help */
            case 'h':
                print_help (argv [0]);
                return 1;
            case 'b':
                device_number_str = optarg;
                break;
            case 'v':
                verbose = 1;
                break;
            case 'a':
                address_str = optarg;
                break;
            default:
                fprintf (stderr, "Could not parse options\n");
                print_help (argv [0]);
                return 1;
        }
    }

    /* Parse data/address */
    if (address_str == NULL) {
        fprintf (stderr, "no address has been selected!\n");
        return 1;
    }

    uint64_t address = 0;
    if (sscanf (address_str, "%" PRIx64, &address) != 1) {
        fprintf (stderr, "-a format is invalid!\n");
        print_help (argv [0]);
        return 1;
    }

    int device_number;
    if (device_number_str == NULL || sscanf (device_number_str, "%d", &device_number) != 1) {
        fprintf(stderr, "invalid or unspecified device number\n");
        print_help(argv[0]);
        return 1;
    }

    pciDriver::PciDevice dev{device_number};
    dev.open();

    struct pcie_bars bars = { .bar0 = dev.mapBAR (0), .bar4 = dev.mapBAR(4) };

    if (verbose) {
        fprintf (stdout, "BAR 0 host address = %p\n", bars.bar0);
        fprintf (stdout, "BAR 4 host address = %p\n", bars.bar4);
    }

#define ACQ_CORE_SIZE_32 (sizeof(struct acq_core) / 4)
    struct acq_core registers;
    bar4_read_u32s(&bars, address, &registers, ACQ_CORE_SIZE_32);

    decode_registers_print(&registers, stdout);

    return 0;
}
