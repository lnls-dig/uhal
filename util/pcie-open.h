/*
 * Copyright (C) 2022 CNPEM (cnpem.br)
 * Author: Érico Nogueira <erico.rolim@lnls.br>
 *
 * Released according to the GNU GPL, version 3 or any later version.
 */

#ifndef PCIE_OPEN_H
#define PCIE_OPEN_H

#include "pcie.h"

void dev_open_slot(struct pcie_bars &bars, int slot);
void dev_open_slot(struct pcie_bars &bars, const char *slot);
void dev_open(struct pcie_bars &bars, const char *pci_address);
void dev_close(struct pcie_bars &bars);

#endif
