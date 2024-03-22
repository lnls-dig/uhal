#ifndef SDB_DEFS_H
#define SDB_DEFS_H

#include <cstdint>
#include <functional>

struct sdb_device_info {
    uint64_t start_addr = 0;
    uint64_t vendor_id = 0;
    uint32_t device_id = 0;
    uint8_t abi_ver_major = 0;
    uint8_t abi_ver_minor = 0;
};

struct sdb_synthesis_info {
    char name[17];
    char commit[33];
    char tool_name[9];
    uint32_t tool_version;
    uint32_t date;
    char user_name[16];
};

typedef std::function<bool(const struct sdb_device_info &)> device_match_fn;

#endif
