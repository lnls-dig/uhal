/*
 * Copyright (C) 2022 CNPEM (cnpem.br)
 * Author: Érico Nogueira <erico.rolim@lnls.br>
 *
 * Released according to the GNU GPL, version 3 or any later version.
 */

#ifndef FOFB_PROCESSING_H
#define FOFB_PROCESSING_H

#include <memory>
#include <vector>

#include "decoders.h"

#define FOFB_PROCESSING_DEVID 0x49681ca6
inline const device_match_fn device_match_fofb_processing =
    device_match_impl<LNLS_VENDORID, FOFB_PROCESSING_DEVID, 1>;

/* forward declaration */
struct wb_fofb_processing_regs;

class LnlsFofbProcessing: public RegisterDecoder {
    std::unique_ptr<struct wb_fofb_processing_regs> regs_storage;
    struct wb_fofb_processing_regs &regs;

  public:
    LnlsFofbProcessing();
    ~LnlsFofbProcessing();
    void print(FILE *, bool);
};

class LnlsFofbProcessingController {
  protected:
    struct pcie_bars *bars;
    size_t addr;

    uint32_t fixed_point;

    std::unique_ptr<struct wb_fofb_processing_regs> regs_storage;
    struct wb_fofb_processing_regs &regs;

    void get_internal_values();
    void encode_config();

  public:
    LnlsFofbProcessingController(struct pcie_bars *, size_t);
    ~LnlsFofbProcessingController();

    static inline const device_match_fn device_match = device_match_fofb_processing;

    std::vector<float> ram_bank_values;
    unsigned channel = 0;

    void write_params();
};

#endif
