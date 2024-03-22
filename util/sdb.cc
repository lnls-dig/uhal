#include <endian.h>
#include <limits.h>
#include <stdint.h>

extern "C" {
/* library uses "this" as a struct member name */
#pragma GCC diagnostic push
#ifdef __clang__
# pragma GCC diagnostic ignored "-Wkeyword-macro"
#endif
#define this thisp
#include "libsdbfs.h"
#undef this
#pragma GCC diagnostic pop
}

#include "defer.h"
#include "pcie.h"
#include "util_sdb.h"

static int read(struct sdbfs *fs, int offset, void *buf, int count)
{
    auto *bars = static_cast<struct pcie_bars *>(fs->drvdata);

    bar4_read_v(bars, offset, buf, count);
    return count;
}

static struct sdbfs sdbfs_init(struct pcie_bars *bars)
{
    struct sdbfs fs{};
    fs.name = "sdb-area";
    fs.drvdata = bars;
    fs.blocksize = 4;
    fs.entrypoint = 0;
    fs.read = read;
    sdbfs_dev_create(&fs);

    /* read the first device and discard it; this also initializes the fs object */
    sdbfs_scan(&fs, 1);

    return fs;
}

std::optional<struct sdb_device_info> read_sdb(struct pcie_bars *bars, device_match_fn device_match)
{
    return read_sdb(bars, device_match, UINT_MAX);
}

std::optional<struct sdb_device_info> read_sdb(struct pcie_bars *bars, device_match_fn device_match, unsigned pos)
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
        (unsigned)devinfo.abi_ver_major, (uintmax_t)devinfo.start_addr);
    return false;
}

std::vector<struct sdb_synthesis_info> get_synthesis_info(struct pcie_bars *bars)
{
    struct sdbfs fs = sdbfs_init(bars);
    defer _(nullptr, [&fs](...){sdbfs_dev_destroy(&fs);});

    std::vector<struct sdb_synthesis_info> rv;

    struct sdb_device *d;
    while ((d = sdbfs_scan(&fs, 0))) {
        struct sdb_product *p = &d->sdb_component.product;
        if (p->record_type != sdb_type_synthesis) continue;

        auto s = (struct sdb_synthesis *)(void *)d;

        struct sdb_synthesis_info syninfo;

        auto copy_string = [](auto &dest, const auto &src) {
            /* strings in sdb_synthesis aren't nul-terminated */
            static_assert(sizeof dest == sizeof src + 1);

            /* some fields might be empty (where padding can be nul or whitespace)  */
            if (src[0] && src[0] != ' ') {
                memcpy(dest, src, sizeof src);
                dest[sizeof src] = '\0';
            } else {
                strcpy(dest, "");
            }
        };

        copy_string(syninfo.name, s->syn_name);

        static_assert(sizeof syninfo.commit == sizeof s->commit_id * 2 + 1);
        /* sprintf nul-terminates it automatically for us */
        for (size_t i = 0; i < sizeof s->commit_id; i++)
            sprintf(syninfo.commit + i*2, "%02x", s->commit_id[i]);

        copy_string(syninfo.tool_name, s->tool_name);
        syninfo.tool_version = ntohl(s->tool_version);
        syninfo.date = ntohl(s->date);
        copy_string(syninfo.user_name, s->user_name);

        rv.push_back(syninfo);
    }

    return rv;
}
