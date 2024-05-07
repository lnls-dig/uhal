#ifndef FMCPICO1M_4CH_H
#define FMCPICO1M_4CH_H

#include <memory>

#include "controllers.h"
#include "decoders.h"

namespace fmcpico1m_4ch {

extern const std::vector<std::string> range_list;

struct fmcpico1m_4ch;

class Core: public RegisterDecoder {
    std::unique_ptr<struct fmcpico1m_4ch> regs_storage;
    struct fmcpico1m_4ch &regs;

    void decode() override;

  public:
    Core(struct pcie_bars &);
    ~Core() override;
};

class Controller: public RegisterDecoderController {
    std::unique_ptr<struct fmcpico1m_4ch> regs_storage;
    struct fmcpico1m_4ch &regs;

    Core dec;

    void write_params() override;

  public:
    Controller(struct pcie_bars &);
    ~Controller();
};

} /* namespace fmcpico1m_4ch */

#endif
