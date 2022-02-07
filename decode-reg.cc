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

#include <pciDriver/lib/pciDriver.h>
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
            "  -b <Device File>          Device file\n"
            "  -v                        Verbose output\n"
            "  -a <FPGA Address in hex>  Base address to start reads from\n",
            program_name);
}

static void unmap_bar(pd_device_t *pci_handle, unsigned bar, void *ptr)
{
    if (ptr)
        pd_unmapBAR(pci_handle, bar, ptr);
}

int main(int argc, char *argv[])
{
    int verbose = 0;
    char *devicefile_str = NULL;
    char *address_str = NULL;
    int opt;
    pd_device_t m_dev = {0};
    pd_device_t *dev = &m_dev;
	
    while ((opt = getopt (argc, argv, "hb:va:")) != -1) {
        /* Get the user selected options */
        switch (opt) {
            /* Display Help */
            case 'h':
                print_help (argv [0]);
                return 1;
            case 'b':
                devicefile_str = optarg;
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

    /* Device file is mandatory */
    if (devicefile_str == NULL) {
         fprintf (stderr, "no device has been selected!\n");
         print_help (argv [0]);
         return 1;
    } 

    /* Parse data/address */
    if (address_str == NULL) {
        fprintf (stderr, "no address has been selected!\n");
        return 1;
    }

    uint64_t address = 0;
    if (sscanf (address_str, "%" PRIx64, &address) != 1) {
        fprintf (stderr, "--address format is invalid!\n");
        print_help (argv [0]);
        return 1;
    }

    /* Open device */
    int err = pd_open (0, dev, devicefile_str);
    if (err != 0) {
         fprintf (stderr, "Could not open device %s, error = %d\n", devicefile_str, err);
         return 1;
    }

    struct pcie_bars bars = { .bar0 = pd_mapBAR (dev, 0), .bar4 = pd_mapBAR(dev, 4) };
    if (bars.bar0 == NULL || bars.bar4 == NULL) {
        fprintf (stderr, "Could not map BAR 0 and 4\n");
        goto exit_unmap_bar;
    }

    if (verbose) {
        fprintf (stdout, "BAR 0 host address = %p\n", bars.bar0);
        fprintf (stdout, "BAR 4 host address = %p\n", bars.bar4);
    }

#define ACQ_CORE_SIZE_32 (sizeof(struct acq_core) / 4)
    struct acq_core registers;
    bar4_read_u32s(&bars, address, &registers, ACQ_CORE_SIZE_32);

    decode_registers_print(&registers, stdout);

exit_unmap_bar:
    unmap_bar(dev, 0, bars.bar0);
    unmap_bar(dev, 4, bars.bar4);
    pd_close (dev);

    return 0;
}
