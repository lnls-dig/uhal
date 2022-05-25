/*
 * Copyright (C) 2022 CNPEM (cnpem.br)
 * Author: Érico Nogueira <erico.rolim@lnls.br>
 *
 * Released according to the GNU GPL, version 3 or any later version.
 */

#ifndef LAMP_H
#define LAMP_H

#include <memory>
#include <string>

#include "pcie.h"
#include "util.h"

#include "decoders.h"

#define LAMP_DEVID 0xa1248bec
inline const device_match_fn device_match_lamp_v1 =
    device_match_impl<LNLS_VENDORID, LAMP_DEVID, 1>;
inline const device_match_fn device_match_lamp_v2 =
    device_match_impl<LNLS_VENDORID, LAMP_DEVID, 2>;

/* forward declarations to avoid conflicts in headers */
struct rtmlamp_ohwr_regs;
struct wb_rtmlamp_ohwr_regs;

/* v1 */
struct channel_registers {
    uint32_t sta, ctl, pi_kp, pi_ti, pi_sp, dac;
};

class LnlsRtmLampCoreV1: public RegisterDecoder {
    std::unique_ptr<struct rtmlamp_ohwr_regs> regs;

  public:
    LnlsRtmLampCoreV1();
    ~LnlsRtmLampCoreV1();
    void print(FILE *, bool);
};

class LnlsRtmLampCoreV2: public RegisterDecoder {
    std::unique_ptr<struct wb_rtmlamp_ohwr_regs> regs;

  public:
    LnlsRtmLampCoreV2();
    ~LnlsRtmLampCoreV2();
    void print(FILE *, bool);
};

class LnlsRtmLampController {
    const struct pcie_bars *bars;
    size_t addr;

    /* control registers */
    uint32_t ctl = 0;
    struct channel_registers channel_regs{};

    void encode_config();

  public:
    LnlsRtmLampController(const struct pcie_bars *bars, size_t addr):
        bars(bars), addr(addr)
    {
    }

    static inline const device_match_fn device_match = device_match_lamp_v1;

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
