/*
 * Copyright (C) 2022 CNPEM (cnpem.br)
 * Author: Érico Nogueira <erico.rolim@lnls.br>
 *
 * Released according to the GNU GPL, version 3 or any later version.
 */

#ifndef PCIE_OPEN_H
#define PCIE_OPEN_H

#include "pcie.h"

struct pcie_bars dev_open(int slot);
struct pcie_bars dev_open(const char *pci_address);
void dev_close(struct pcie_bars &bars);

#endif
