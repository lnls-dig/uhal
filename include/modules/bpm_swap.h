/*
 * Copyright (C) 2023 CNPEM (cnpem.br)
 * Author: Érico Nogueira <erico.rolim@lnls.br>
 *
 * Released according to the GNU GPL, version 3 or any later version.
 */

#ifndef BPM_SWAP_H
#define BPM_SWAP_H

#include <memory>

#include "controllers.h"
#include "decoders.h"

namespace bpm_swap {

/* forward declaration */
struct bpm_swap_regs;

class Core: public RegisterDecoder {
    std::unique_ptr<struct bpm_swap_regs> regs_storage;
    struct bpm_swap_regs &regs;

    void decode() override;

  public:
    Core(struct pcie_bars &);
    ~Core() override;
};

} /* namespace bpm_swap */

#endif
