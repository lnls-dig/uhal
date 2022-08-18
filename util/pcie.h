/*
 * Copyright (C) 2022 CNPEM (cnpem.br)
 * Author: Érico Nogueira <erico.rolim@lnls.br>
 *
 * Released according to the GNU GPL, version 3 or any later version.
 */

#ifndef PCIE_H
#define PCIE_H

#include <stdint.h>
#include <sys/types.h>

struct pcie_bars {
    volatile void *bar0;
    volatile void *bar2;
    volatile void *bar4;

    size_t sizes[3];

    /* private fields */
    uint32_t last_bar4_page;
};

#ifdef __cplusplus
extern "C" {
#endif
void bar2_read_v(const struct pcie_bars *bars, size_t addr, void *dest, size_t n);
void bar2_read_dma(const struct pcie_bars *bars, size_t addr, unsigned bar, unsigned long physical_address, size_t n);
void bar4_write(struct pcie_bars *bars, size_t addr, uint32_t value);
void bar4_write_v(struct pcie_bars *bars, size_t addr, const void *src, size_t n);
uint32_t bar4_read(struct pcie_bars *bars, size_t addr);
void bar4_read_v(struct pcie_bars *bars, size_t addr, void *dest, size_t n);
#ifdef __cplusplus
}
#endif

#endif
