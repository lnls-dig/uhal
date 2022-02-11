/*
 * Copyright (C) 2022 CNPEM (cnpem.br)
 * Author: Érico Nogueira <erico.rolim@lnls.br>
 *
 * Released according to the GNU GPL, version 3 or any later version.
 */

#include "decoders.h"

void RegisterDecoder::read(const struct pcie_bars *bars, size_t address)
{
    bar4_read_u32s(bars, address, read_dest, read_size/4);
}
