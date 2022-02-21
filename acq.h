/*
 * Copyright (C) 2022 CNPEM (cnpem.br)
 * Author: Érico Nogueira <erico.rolim@lnls.br>
 *
 * Released according to the GNU GPL, version 3 or any later version.
 */

#ifndef ACQ_H
#define ACQ_H

#include <string>

#include "decoders.h"

class LnlsBpmAcqCoreController {
    const struct pcie_bars *bars;
    size_t addr;

    struct acq_core regs{};

  public:
    LnlsBpmAcqCoreController(const struct pcie_bars *bars, size_t addr):
        bars(bars), addr(addr)
    {
    }

    unsigned channel = 0;
    unsigned pre_samples = 4;
    unsigned post_samples = 16;
    unsigned number_shots = 1;
    std::string trigger_type = "immediate";
    unsigned data_trigger_threshold = 0;
    bool data_trigger_polarity_neg = true;
    unsigned data_trigger_sel = 0;
    unsigned data_trigger_filt = 1;
    unsigned data_trigger_channel = 0;
    unsigned trigger_delay = 0;

    void encode_config();
    void write_config();
    void start_acquisition();
    void wait_for_acquisition();
};

#endif
