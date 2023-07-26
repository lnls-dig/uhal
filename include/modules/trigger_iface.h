/*
 * Copyright (C) 2023 CNPEM (cnpem.br)
 * Author: Érico Nogueira <erico.rolim@lnls.br>
 *
 * Released according to the GNU GPL, version 3 or any later version.
 */

#ifndef TRIGGER_IFACE_H
#define TRIGGER_IFACE_H

#include <memory>
#include <optional>
#include <vector>

#include "controllers.h"
#include "decoders.h"

namespace trigger_iface {

constexpr unsigned TRIGGER_IFACE_DEVID = 0xbcbb78d2;

inline const device_match_fn device_match_trigger_iface =
    device_match_impl<LNLS_VENDORID, TRIGGER_IFACE_DEVID, 1>;

/* forward declaration */
struct trigger_iface_regs;

class Core: public RegisterDecoder {
    std::unique_ptr<struct trigger_iface_regs> regs_storage;
    struct trigger_iface_regs &regs;

    void decode() override;

  public:
    Core(struct pcie_bars &);
    ~Core();
};

class Controller: public RegisterController {
  protected:
    std::unique_ptr<struct trigger_iface_regs> regs_storage;
    struct trigger_iface_regs &regs;

    void get_internal_values();
    void encode_config();

  public:
    Controller(struct pcie_bars &);
    ~Controller();

    static inline const device_match_fn device_match = device_match_trigger_iface;

    void write_params();

    struct parameters {
        /* rcv_count_rst and transm_count_rst are cleared automatically */
        std::optional<bool> direction, direction_polarity, rcv_count_rst, transm_count_rst;
        std::optional<uint8_t> rcv_len, transm_len;
    };
    std::vector<struct parameters> parameters;
};

} /* namespace trigger_iface */

#endif
