/*
 * Copyright (C) 2022 CNPEM (cnpem.br)
 * Author: Érico Nogueira <erico.rolim@lnls.br>
 *
 * Released according to the GNU GPL, version 3 or any later version.
 */

#ifndef UPPER_SDB_H
#define UPPER_SDB_H

#include <optional>
#include <functional>

#include "pcie.h"

struct sdb_device_info {
    uint64_t start_addr;
    uint64_t vendor_id;
    uint32_t device_id;
    uint8_t abi_ver_major;
    uint8_t abi_ver_minor;
};

typedef std::function<bool(const struct sdb_device_info &)> device_match_fn;

std::optional<struct sdb_device_info> read_sdb(const struct pcie_bars *, device_match_fn);
std::optional<struct sdb_device_info> read_sdb(const struct pcie_bars *, device_match_fn, unsigned);

bool print_sdb(const struct sdb_device_info &);

#endif
