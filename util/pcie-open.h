/*
 * Copyright (C) 2022 CNPEM (cnpem.br)
 * Author: Érico Nogueira <erico.rolim@lnls.br>
 *
 * Released according to the GNU GPL, version 3 or any later version.
 */

/** \file
 * This file contains the functions used to initialize and destroy a pcie_bars
 * struct. */

#ifndef PCIE_OPEN_H
#define PCIE_OPEN_H

#include "pcie-defs.h"

/** Memory map the BARs of PCIe device in \p slot */
void dev_open_slot(struct pcie_bars &bars, int slot);
/** Memory map the BARs of PCIe device in \p slot, when \p slot isn't a simple
 * integer */
void dev_open_slot(struct pcie_bars &bars, const char *slot);
/** Memory map the BARs of PCIe device with address \p pci_address, found in
 * "/sys/bus/pci/devices/<pci_address>.0/" */
void dev_open(struct pcie_bars &bars, const char *pci_address);
void dev_open_serial(struct pcie_bars &bars, const char *dev_file);
/** Unmap memory and destroy mutexes of pcie_bars */
void dev_close(struct pcie_bars &bars);

#endif
