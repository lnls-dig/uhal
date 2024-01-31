/*
 * Copyright (C) 2022 CNPEM (cnpem.br)
 * Author: Érico Nogueira <erico.rolim@lnls.br>
 *
 * Released according to the GNU GPL, version 3 or any later version.
 */

#include "pcie.h"

#include "controllers.h"

RegisterController::RegisterController(struct pcie_bars &bars, const struct sdb_device_info &ref_devinfo):
    RegisterDecoderBase(bars, ref_devinfo)
{
}

void RegisterController::set_devinfo(const struct sdb_device_info &new_devinfo)
{
    RegisterDecoderBase::set_devinfo(new_devinfo);

    set_devinfo_callback();
}

void RegisterController::write_params()
{
    check_devinfo_is_set();

    encode_params();
    bar4_write_v(&bars, addr, read_dest, read_size);

    unset_commands();
}
