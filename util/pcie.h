/** \file
 * This file contains the functions used to read and write from BARs 2 and 4.
 * All addresses and amounts to read or write are in bytes. */

#ifndef PCIE_H
#define PCIE_H

#include "pcie-defs.h"

#ifdef __cplusplus
extern "C" {
#endif
void bar2_read_v(struct pcie_bars *bars, size_t addr, void *dest, size_t n);
void bar4_write(struct pcie_bars *bars, size_t addr, uint32_t value);
void bar4_write_v(struct pcie_bars *bars, size_t addr, const void *src, size_t n);
uint32_t bar4_read(struct pcie_bars *bars, size_t addr);
void bar4_read_v(struct pcie_bars *bars, size_t addr, void *dest, size_t n);

void device_reset(const struct pcie_bars *bars);
#ifdef __cplusplus
}
#endif

#endif
