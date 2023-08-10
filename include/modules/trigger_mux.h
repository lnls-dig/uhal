/*
 * Copyright (C) 2023 CNPEM (cnpem.br)
 * Author: Érico Nogueira <erico.rolim@lnls.br>
 *
 * Released according to the GNU GPL, version 3 or any later version.
 */

#ifndef TRIGGER_MUX_H
#define TRIGGER_MUX_H

#include <memory>
#include <optional>
#include <vector>

#include "controllers.h"
#include "decoders.h"

namespace trigger_mux {

/* forward declaration */
struct trigger_mux_regs;

class Core: public RegisterDecoder {
    std::unique_ptr<struct trigger_mux_regs> regs_storage;
    struct trigger_mux_regs &regs;

    void decode() override;

  public:
    Core(struct pcie_bars &);
    ~Core();
};

class Controller: public RegisterController {
  protected:
    std::unique_ptr<struct trigger_mux_regs> regs_storage;
    struct trigger_mux_regs &regs;

    void get_internal_values();
    void encode_params() override;

  public:
    Controller(struct pcie_bars &);
    ~Controller();

    struct parameters {
        std::optional<bool> rcv_src, transm_src;
        std::optional<uint8_t> rcv_in_sel, transm_out_sel;
    };
    std::vector<struct parameters> parameters;
};

} /* namespace trigger_mux */

#endif
