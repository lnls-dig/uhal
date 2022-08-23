/*
 * Copyright (C) 2022 CNPEM (cnpem.br)
 * Author: Érico Nogueira <erico.rolim@lnls.br>
 *
 * Released according to the GNU GPL, version 3 or any later version.
 */

#ifndef DECODERS_H
#define DECODERS_H

#include <cstdint>
#include <cstdio>
#include <optional>

#include <sys/types.h>

#include "pcie.h"
#include "util_sdb.h"

#define LNLS_VENDORID 0x1000000000001215

template<uint64_t t_vendor_id, uint32_t t_device_id, uint8_t major_version>
bool device_match_impl(const struct sdb_device_info &devinfo)
{
    return (devinfo.vendor_id == t_vendor_id) && (devinfo.device_id == t_device_id) &&
      (devinfo.abi_ver_major == major_version);
}

class RegisterDecoder {
  protected:
    size_t read_size;
    void *read_dest;

    struct sdb_device_info devinfo;

  public:
    virtual ~RegisterDecoder() {};
    virtual void read(struct pcie_bars *);
    virtual void print(FILE *, bool) = 0;
    void set_devinfo(const struct sdb_device_info &);

    device_match_fn device_match;

    std::optional<unsigned> channel;
};

#endif
