#ifndef FMC_ACTIVE_CLK_H
#define FMC_ACTIVE_CLK_H

#include <memory>

#include "controllers.h"
#include "decoders.h"

namespace fmc_active_clk {

struct fmc_active_clk;

class Core: public RegisterDecoder {
    std::unique_ptr<struct fmc_active_clk> regs_storage;
    struct fmc_active_clk &regs;

    void decode() override;

  public:
    Core(struct pcie_bars &);
    ~Core() override;
};

class Controller: public RegisterDecoderController {
    std::unique_ptr<struct fmc_active_clk> regs_storage;
    struct fmc_active_clk &regs;

    Core dec;

  public:
    Controller(struct pcie_bars &);
    ~Controller();
};

} /* namespace fmc_active_clk */

#endif
