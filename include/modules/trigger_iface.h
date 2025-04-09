#ifndef TRIGGER_IFACE_H
#define TRIGGER_IFACE_H

#include <memory>
#include <optional>
#include <vector>

#include "controllers.h"
#include "decoders.h"

namespace trigger_iface {

/* forward declaration */
struct trigger_iface_regs;

class Core: public RegisterDecoder {
    std::unique_ptr<struct trigger_iface_regs> regs_storage;
    struct trigger_iface_regs &regs;

    void decode() override;

  public:
    Core(struct pcie_bars &);
    ~Core() override;
};

class Controller: public RegisterController {
  protected:
    std::unique_ptr<struct trigger_iface_regs> regs_storage;
    struct trigger_iface_regs &regs;

    void encode_params() override;

  public:
    Controller(struct pcie_bars &);
    ~Controller();

    void write_params() override;

    struct parameters {
        /* rcv_count_rst and transm_count_rst are cleared automatically */
        std::optional<bool> direction, direction_polarity, rcv_count_rst, transm_count_rst;
        std::optional<uint8_t> rcv_len, transm_len;
    };
    std::vector<struct parameters> parameters;
};

} /* namespace trigger_iface */

#endif
