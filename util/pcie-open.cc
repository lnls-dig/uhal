/*
 * Copyright (C) 2022 CNPEM (cnpem.br)
 * Author: Érico Nogueira <erico.rolim@lnls.br>
 *
 * Released according to the GNU GPL, version 3 or any later version.
 */

#include <cstdio>
#include <fstream>
#include <string>

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "pcie-open.h"

struct pcie_bars dev_open(int slot)
{
    char slot_path[64];
    snprintf(slot_path, sizeof slot_path, "/sys/bus/pci/slots/%d/address", slot);
    std::ifstream fslot{slot_path};
    std::string pci_address;
    fslot >> pci_address;
    return dev_open(pci_address.c_str());
}

struct pcie_bars dev_open(const char *pci_address)
{
    const char resource_path_fmt[] = "/sys/bus/pci/devices/%s.0/resource%d";
    char resource_path[sizeof resource_path_fmt + 32];

    struct pcie_bars rv{};
    for (unsigned i = 0; i < 3; i++) {
        unsigned bar = i * 2; // bars 0, 2, 4
        snprintf(resource_path, sizeof resource_path, resource_path_fmt, pci_address, bar);
        int fd = open(resource_path, O_RDWR);
        if (fd < 0) {
            throw std::runtime_error(std::string("couldn't open resource file: ") + pci_address);
        }
        struct stat st;
        fstat(fd, &st);
        rv.sizes[i] = st.st_size;
        void *ptr = mmap(NULL, st.st_size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
        close(fd);
        if (ptr == MAP_FAILED) {
            throw std::runtime_error(std::string("couldn't mmap resource file: ") + pci_address);
        }
        switch (bar) {
            case 0: rv.bar0 = ptr; break;
            case 2: rv.bar2 = ptr; break;
            case 4: rv.bar4 = ptr; break;
        }
    }

    return rv;
}

void dev_close(struct pcie_bars &bars)
{
    auto unmap = [&bars](auto &p, unsigned i) {
        munmap(const_cast<void *>(p), bars.sizes[i]);
        p = 0;
        bars.sizes[i] = 0;
    };
    unmap(bars.bar0, 0);
    unmap(bars.bar2, 1);
    unmap(bars.bar4, 2);
}
