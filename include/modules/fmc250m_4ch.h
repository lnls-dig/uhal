#ifndef FMC250M_4CH_H
#define FMC250M_4CH_H

#include <memory>

#include "controllers.h"
#include "decoders.h"

namespace fmc250m_4ch {

struct fmc250m_4ch;

class Core: public RegisterDecoder {
    std::unique_ptr<struct fmc250m_4ch> regs_storage;
    struct fmc250m_4ch &regs;

    void decode() override;

  public:
    Core(struct pcie_bars &);
    ~Core() override;
};

class Controller: public RegisterDecoderController {
    std::unique_ptr<struct fmc250m_4ch> regs_storage;
    struct fmc250m_4ch &regs;

    Core dec;

    void unset_commands() override;

  public:
    Controller(struct pcie_bars &);
    ~Controller();
};

} /* namespace fmc250m_4ch */

#endif
