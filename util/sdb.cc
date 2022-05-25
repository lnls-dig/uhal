/*
 * Copyright (C) 2022 CNPEM (cnpem.br)
 * Author: Érico Nogueira <erico.rolim@lnls.br>
 *
 * Released according to the GNU GPL, version 3 or any later version.
 */

#include <endian.h>
#include <limits.h>

extern "C" {
/* library uses "this" as a struct member name */
#define this thisp
#include "libsdbfs.h"
#undef this
}

#include "defer.h"
#include "pcie.h"
#include "util_sdb.h"

static int read(struct sdbfs *fs, int offset, void *buf, int count)
{
    const auto *bars = static_cast<const struct pcie_bars *>(fs->drvdata);

    bar4_read_v(bars, offset, buf, count);
    return count;
}

static struct sdbfs sdbfs_init(const struct pcie_bars *bars)
{
    struct sdbfs fs{};
    fs.name = "sdb-area";
    fs.drvdata = const_cast<struct pcie_bars *>(bars);
    fs.blocksize = 4;
    fs.entrypoint = 0;
    fs.read = read;
    sdbfs_dev_create(&fs);

    /* read the first device and discard it; this also initializes the fs object */
    sdbfs_scan(&fs, 1);

    return fs;
}

std::optional<struct sdb_device_info> read_sdb(const struct pcie_bars *bars, device_match_fn device_match)
{
    return read_sdb(bars, device_match, UINT_MAX);
}

std::optional<struct sdb_device_info> read_sdb(const struct pcie_bars *bars, device_match_fn device_match, unsigned pos)
{
    struct sdbfs fs = sdbfs_init(bars);
    defer _(nullptr, [&fs](...){sdbfs_dev_destroy(&fs);});

    struct sdb_device *d;
    while((d = sdbfs_scan(&fs, 0))) {
        struct sdb_component *c = &d->sdb_component;
        struct sdb_product *p = &c->product;
        if (p->record_type != sdb_type_device) continue;

        struct sdb_device_info devinfo;
        devinfo.start_addr = fs.base[fs.depth] + be64toh(c->addr_first);
        devinfo.vendor_id = be64toh(p->vendor_id);
        devinfo.device_id = be32toh(p->device_id);
        devinfo.abi_ver_major = d->abi_ver_major;
        devinfo.abi_ver_minor = d->abi_ver_minor;

        if (device_match && device_match(devinfo)) {
            if (pos == 0) return devinfo;
            else pos--;
        }
    }
    return std::nullopt;
}

bool print_sdb(const struct sdb_device_info &devinfo)
{
    fprintf(stdout, "id %08jx vendor %016jx version %04x addr %08jx\n",
        (uintmax_t)devinfo.device_id, (uintmax_t)devinfo.vendor_id,
        devinfo.abi_ver_major, (uintmax_t)devinfo.start_addr);
    return false;
}
