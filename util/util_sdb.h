/** \file
 * This file contains the functions used to query a Self Describing Bus (SDB)
 * filesystem. */

#ifndef UPPER_SDB_H
#define UPPER_SDB_H

#include <optional>
#include <vector>

#include "pcie-defs.h"
#include "sdb-defs.h"

/** This function navigates the SDB filesystem and passes decoded SDB product
 * entries to \p device_match. When \p device_match returns true for the \p pos
 * -th time, this function returns that SDB device's information; the address
 * can be used to locate the device's register map, and the version information
 * makes it possible to gate functionality behind version checks. If \p
 * device_match is null, \p pos is ignored and this function simply prints all
 * SDB information. */
std::optional<struct sdb_device_info> read_sdb(struct pcie_bars *, device_match_fn, unsigned);

std::vector<struct sdb_synthesis_info> get_synthesis_info(struct pcie_bars *);

#endif
