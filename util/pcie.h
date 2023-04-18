/*
 * Copyright (C) 2022 CNPEM (cnpem.br)
 * Author: Érico Nogueira <erico.rolim@lnls.br>
 *
 * Released according to the GNU GPL, version 3 or any later version.
 */

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
