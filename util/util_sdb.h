/*
 * Copyright (C) 2022 CNPEM (cnpem.br)
 * Author: Érico Nogueira <erico.rolim@lnls.br>
 *
 * Released according to the GNU GPL, version 3 or any later version.
 */

#ifndef UPPER_SDB_H
#define UPPER_SDB_H

#include <optional>
#include <vector>

#include "pcie-defs.h"
#include "sdb-defs.h"

std::optional<struct sdb_device_info> read_sdb(struct pcie_bars *, device_match_fn);
std::optional<struct sdb_device_info> read_sdb(struct pcie_bars *, device_match_fn, unsigned);

bool print_sdb(const struct sdb_device_info &);

std::vector<struct sdb_synthesis_info> get_synthesis_info(struct pcie_bars *);

#endif
