/*
 * Copyright (C) 2022 CNPEM (cnpem.br)
 * Author: Érico Nogueira <erico.rolim@lnls.br>
 *
 * Released according to the GNU GPL, version 3 or any later version.
 */

#ifndef FOFB_PROCESSING_H
#define FOFB_PROCESSING_H

#include <array>

#include "decoders.h"
#include "hw/wb_fofb_processing_regs.h"

#define FOFB_PROCESSING_DEVID 0x49681ca6
inline const device_match_fn device_match_fofb_processing =
    device_match_impl<LNLS_VENDORID, FOFB_PROCESSING_DEVID, 1>;

#define FOFB_PROCESSING_RAM_BANK_SIZE (WB_FOFB_PROCESSING_REGS_RAM_BANK_1 - WB_FOFB_PROCESSING_REGS_RAM_BANK_0)

class LnlsFofbProcessing: public RegisterDecoder {
    struct wb_fofb_processing_regs regs;

  public:
    LnlsFofbProcessing()
    {
        read_size = sizeof regs;
        read_dest = &regs;

        device_match = device_match_fofb_processing;
    }
    void print(FILE *, bool);
};

class LnlsFofbProcessingController {
  protected:
    struct pcie_bars *bars;
    size_t addr;

    uint32_t fixed_point;

    struct wb_fofb_processing_regs regs;

    void get_internal_values();
    void encode_config();

  public:
    LnlsFofbProcessingController(struct pcie_bars *bars, size_t addr):
        bars(bars), addr(addr)
    {
    }

    static inline const device_match_fn device_match = device_match_fofb_processing;

    std::array<float, FOFB_PROCESSING_RAM_BANK_SIZE / sizeof(float)> ram_bank_values = {};
    unsigned channel = 0;

    void write_params();
};

#endif
