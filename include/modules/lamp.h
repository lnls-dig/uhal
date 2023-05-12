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

#include "controllers.h"
#include "decoders.h"

namespace lamp {

constexpr unsigned LAMP_DEVID = 0xa1248bec;

inline const device_match_fn device_match_v2 =
    device_match_impl<LNLS_VENDORID, LAMP_DEVID, 2>;

struct wb_rtmlamp_ohwr_regs;

/* v2 forward declaration */
typedef struct channel_registers_v2_impl channel_registers_v2;

class CoreV2: public RegisterDecoder {
    std::unique_ptr<struct wb_rtmlamp_ohwr_regs> regs;

    void decode();

  public:
    CoreV2(struct pcie_bars &);
    ~CoreV2();
};

class Controller: public RegisterController {
  protected:
    virtual void encode_config() = 0;

  public:
    Controller(struct pcie_bars &bars, device_match_fn device_match):
        RegisterController(bars),
        device_match(device_match)
    {
    }
    virtual ~Controller() = default;

    const device_match_fn device_match;

    unsigned channel = 0;
    /* per channel properties */
    bool amp_enable = true;
    std::string mode = "none";
    std::optional<int> mode_numeric;
    std::optional<bool> trigger_enable;
    bool reset_latch = false;
    std::optional<uint32_t> pi_kp, pi_ti; /* are actually 26-bit */
    std::optional<int16_t> pi_sp;
    std::optional<int16_t> dac;
    std::optional<int16_t> limit_a, limit_b;
    std::optional<uint32_t> cnt;

    virtual void write_params() = 0;
};

class ControllerV2: public Controller {
    /* control registers */
    std::unique_ptr<channel_registers_v2> channel_regs;

    void encode_config();

  public:
    ControllerV2(struct pcie_bars &);
    ~ControllerV2();

    void write_params();
};

} /* namespace lamp */

#endif
