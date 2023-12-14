/*
 * Copyright (C) 2023 CNPEM (cnpem.br)
 * Author: Érico Nogueira <erico.rolim@lnls.br>
 *
 * Released according to the GNU GPL, version 3 or any later version.
 */

#ifndef ORBIT_INTLK_H
#define ORBIT_INTLK_H

#include <memory>

#include "controllers.h"
#include "decoders.h"

namespace orbit_intlk {

/* forward declaration */
struct orbit_intlk_regs;

class Core: public RegisterDecoder {
    std::unique_ptr<struct orbit_intlk_regs> regs_storage;
    struct orbit_intlk_regs &regs;

    void decode() override;

  public:
    Core(struct pcie_bars &);
    ~Core() override;
};

class Controller: public RegisterController {
    std::unique_ptr<struct orbit_intlk_regs> regs_storage;
    struct orbit_intlk_regs &regs;

    void encode_params() override;
    void write_params() override;

  public:
    Controller(struct pcie_bars &);
    ~Controller();

    bool enable{}, clear{};
    bool min_sum_enable{};
    bool pos_enable{}, pos_clear{}, ang_enable{}, ang_clear{};

    int32_t min_sum{};
    int32_t pos_max_x{}, pos_max_y{}, ang_max_x{}, ang_max_y{};
    int32_t pos_min_x{}, pos_min_y{}, ang_min_x{}, ang_min_y{};
};

} /* namespace orbit_intlk */

#endif
