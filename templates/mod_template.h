#ifndef MOD_TEMPLATE_H
#define MOD_TEMPLATE_H

#include <memory>

#include "controllers.h"
#include "decoders.h"

namespace mod_template {

struct mod_template;

class Core: public RegisterDecoder {
    std::unique_ptr<struct mod_template> regs_storage;
    struct mod_template &regs;

    void decode() override;

  public:
    Core(struct pcie_bars &);
    ~Core() override;
};

class Controller: public RegisterDecoderController {
    std::unique_ptr<struct mod_template> regs_storage;
    struct mod_template &regs;

    Core dec;

  public:
    Controller(struct pcie_bars &);
    ~Controller();
};

} /* namespace mod_template */

#endif
