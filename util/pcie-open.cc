/*
 * Copyright (C) 2022 CNPEM (cnpem.br)
 * Author: Érico Nogueira <erico.rolim@lnls.br>
 *
 * Released according to the GNU GPL, version 3 or any later version.
 */

#include <charconv>
#include <cstdio>
#include <fstream>
#include <string>

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "pcie-open.h"

void dev_open_slot(struct pcie_bars &bars, int slot)
{
    char slot_chars[64];
    auto r = std::to_chars(slot_chars, slot_chars + sizeof slot_chars, slot);
    *r.ptr = '\0';
    dev_open_slot(bars, slot_chars);
}

void dev_open_slot(struct pcie_bars &bars, const char *slot)
{
    char slot_path[64];
    snprintf(slot_path, sizeof slot_path, "/sys/bus/pci/slots/%s/address", slot);
    std::ifstream fslot{slot_path};
    std::string pci_address;
    fslot >> pci_address;
    dev_open(bars, pci_address.c_str());
}

void dev_open(struct pcie_bars &bars, const char *pci_address)
{
    const char resource_path_fmt[] = "/sys/bus/pci/devices/%s.0/resource%d";
    const char wc_resource_path_fmt[] = "/sys/bus/pci/devices/%s.0/resource%d_wc";
    char resource_path[sizeof resource_path_fmt + 32];

    for (unsigned i = 0; i < 3; i++) {
        unsigned bar = i * 2; // bars 0, 2, 4
        snprintf(resource_path, sizeof resource_path, wc_resource_path_fmt, pci_address, bar);
        int fd = open(resource_path, O_RDWR);
        if (fd < 0) {
            snprintf(resource_path, sizeof resource_path, resource_path_fmt, pci_address, bar);
            fd = open(resource_path, O_RDWR);
            if (fd < 0)
                throw std::runtime_error(std::string("couldn't open resource file: ") + pci_address);
        }
        struct stat st;
        fstat(fd, &st);
        bars.sizes[i] = st.st_size;
        void *ptr = mmap(NULL, st.st_size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
        close(fd);
        if (ptr == MAP_FAILED) {
            throw std::runtime_error(std::string("couldn't mmap resource file: ") + pci_address);
        }
        switch (bar) {
            case 0: bars.bar0 = ptr; break;
            case 2: bars.bar2 = ptr; break;
            case 4: bars.bar4 = ptr; break;
        }
    }

    /* set -1 so it's always different on the first run */
    bars.last_bar4_page = -1;

    pthread_mutexattr_t mattr;
    if (pthread_mutexattr_init(&mattr) ||
        pthread_mutexattr_settype(&mattr, PTHREAD_MUTEX_RECURSIVE))
        throw std::runtime_error("couldn't set mutexattr as recursive");

    for (auto &lock: bars.locks) {
        if (pthread_mutex_init(&lock, &mattr))
            throw std::runtime_error("couldn't initialize mutex");
    }
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
