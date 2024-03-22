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

class Controller: public RegisterController {
    std::unique_ptr<struct bpm_swap_regs> regs_storage;
    struct bpm_swap_regs &regs;

    void encode_params() override;
    void write_params() override;

  public:
    Controller(struct pcie_bars &);
    ~Controller();

    static const std::vector<std::string> mode_list;

    bool reset = false;
    std::string mode = "direct";
    bool swap_div_f_cnt_en = false;
    uint16_t swap_div_f = 0;
    uint16_t deswap_delay = 0;
};

} /* namespace bpm_swap */

#endif
