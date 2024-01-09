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

struct wb_rtmlamp_ohwr_regs;

extern const std::vector<std::string> mode_list;

class Core: public RegisterDecoder {
    std::unique_ptr<struct wb_rtmlamp_ohwr_regs> regs_storage;
    struct wb_rtmlamp_ohwr_regs &regs;

    void decode() override;

  public:
    Core(struct pcie_bars &);
    ~Core();
};

class Controller: public RegisterDecoderController {
    std::unique_ptr<struct wb_rtmlamp_ohwr_regs> regs_storage;
    struct wb_rtmlamp_ohwr_regs &regs;

    Core dec;

    void unset_commands() override;

  public:
    Controller(struct pcie_bars &);
    virtual ~Controller();
};

} /* namespace lamp */

#endif
