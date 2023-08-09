/*
 * Copyright (C) 2023 CNPEM (cnpem.br)
 * Author: Érico Nogueira <erico.rolim@lnls.br>
 *
 * Released according to the GNU GPL, version 3 or any later version.
 */

#include <stdexcept>

#include "pcie.h"
#include "decoders.h"

RegisterDecoderBase::RegisterDecoderBase(struct pcie_bars &bars, const struct sdb_device_info &ref_devinfo):
    vendor_id(ref_devinfo.vendor_id),
    device_id(ref_devinfo.device_id),
    major_version(ref_devinfo.abi_ver_major),
    check_devinfo(true),
    bars(bars),
    match_devinfo_lambda([this](auto const &devinfo){ return match_devinfo(devinfo); })
{
}

bool RegisterDecoderBase::match_devinfo(const struct sdb_device_info &match) const
{
    return
      match.vendor_id == vendor_id &&
      match.device_id == device_id &&
      match.abi_ver_major == major_version;
}

void RegisterDecoderBase::set_devinfo(const struct sdb_device_info &new_devinfo)
{
    if (check_devinfo && !match_devinfo(new_devinfo))
        throw std::logic_error("assigned devinfo doesn't work for this device");

    devinfo = new_devinfo;
    addr = devinfo.start_addr;

    devinfo_is_set = true;
}

void RegisterDecoderBase::read()
{
    bar4_read_v(&bars, devinfo.start_addr, read_dest, read_size);
}

void RegisterDecoderBase::check_devinfo_is_set() const
{
    if (!devinfo_is_set)
        throw std::logic_error("set_devinfo() has not been called");
}
