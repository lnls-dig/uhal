/*
 * Copyright (C) 2022 CNPEM (cnpem.br)
 * Author: Érico Nogueira <erico.rolim@lnls.br>
 *
 * Released according to the GNU GPL, version 3 or any later version.
 */

#include "decoders.h"

void RegisterDecoder::set_devinfo(const struct sdb_device_info &new_devinfo)
{
    devinfo = new_devinfo;
}

void RegisterDecoder::read(struct pcie_bars *bars)
{
    bar4_read_v(bars, devinfo.start_addr, read_dest, read_size);
}
