/*
 * Copyright (C) 2022 CNPEM (cnpem.br)
 * Author: Érico Nogueira <erico.rolim@lnls.br>
 *
 * Released according to the GNU GPL, version 3 or any later version.
 */

#ifndef LAMP_H
#define LAMP_H

#include <string>

#include "pcie.h"
#include "util.h"

#include "wb_rtmlamp_ohwr_regs.h"

class LnlsRtmLampController {
    const struct pcie_bars *bars;
    size_t addr;

    /* control registers */
    uint32_t ctl=0;
    struct {
        uint32_t
            sta=0, /* sta is here to guarantee the struct layout */
            ctl=0, pi_kp=0, pi_ti=0, pi_sp=0, dac=0;
    } channel_regs;

    void encode_config();

  public:
    LnlsRtmLampController(const struct pcie_bars *bars, size_t addr):
        bars(bars), addr(addr)
    {
    }

    unsigned channel = 0;
    bool dac_data = false;
    /* per channel properties */
    bool amp_enable = true;
    std::string mode = "none";
    uint32_t pi_kp = 0, pi_ti = 0; /* are actually 26-bit */
    int16_t pi_sp = 0, dac = 0;

    void write_params();
};

#endif
