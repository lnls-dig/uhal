/*
 * Copyright (C) 2022 CNPEM (cnpem.br)
 * Author: Érico Nogueira <erico.rolim@lnls.br>
 *
 * Released according to the GNU GPL, version 3 or any later version.
 */

#ifndef LAMP_H
#define LAMP_H

#include <memory>
#include <optional>
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
/* v2 forward declaration */
typedef struct channel_registers_v2_impl channel_registers_v2;

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
  protected:
    const struct pcie_bars *bars;
    size_t addr;

    virtual void encode_config() = 0;

  public:
    LnlsRtmLampController(const struct pcie_bars *bars, size_t addr, device_match_fn device_match):
        bars(bars), addr(addr), device_match(device_match)
    {
    }
    virtual ~LnlsRtmLampController() = default;

    const device_match_fn device_match;

    unsigned channel = 0;
    /* per channel properties */
    bool amp_enable = true;
    std::string mode = "none";
    std::optional<uint32_t> pi_kp, pi_ti; /* are actually 26-bit */
    std::optional<int16_t> pi_sp;
    std::optional<uint16_t> dac;
    std::optional<int16_t> limit_a, limit_b;
    std::optional<uint32_t> cnt;

    void set_addr(size_t naddr) { addr = naddr; }

    virtual void write_params() = 0;
};

class LnlsRtmLampControllerV1: public LnlsRtmLampController {
    /* control registers */
    uint32_t ctl = 0;
    struct channel_registers channel_regs{};

    void encode_config();

  public:
    LnlsRtmLampControllerV1(const struct pcie_bars *bars, size_t addr):
        LnlsRtmLampController(bars, addr, device_match_lamp_v1)
    {
    }

    void write_params();
};

class LnlsRtmLampControllerV2: public LnlsRtmLampController {
    /* control registers */
    std::unique_ptr<channel_registers_v2> channel_regs;

    void encode_config();

  public:
    LnlsRtmLampControllerV2(const struct pcie_bars *bars, size_t addr);
    ~LnlsRtmLampControllerV2();

    void write_params();
};

#endif
