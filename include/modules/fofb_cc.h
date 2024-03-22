#ifndef FOFB_CC_H
#define FOFB_CC_H

#include <memory>

#include "controllers.h"
#include "decoders.h"

namespace fofb_cc {

/* forward declaration */
struct fofb_cc_regs;

class Core: public RegisterDecoder {
    std::unique_ptr<struct fofb_cc_regs> regs_storage;
    struct fofb_cc_regs &regs;

    void read() override;
    void decode() override;

  public:
    Core(struct pcie_bars &);
    ~Core() override;
};

class Controller: public RegisterController {
  protected:
    std::unique_ptr<struct fofb_cc_regs> regs_storage;
    struct fofb_cc_regs &regs;

    void get_internal_values();
    void encode_params() override;

  public:
    Controller(struct pcie_bars &);
    ~Controller();

    /* err_clr, toa_rd_str, rcb_rd_str are cleared automatically */
    std::optional<bool>
        err_clr, cc_enable, tfs_override,
        toa_rd_en, toa_rd_str, rcb_rd_en, rcb_rd_str;

    std::optional<uint16_t> bpm_id, time_frame_len;
    std::optional<bool> mgt_powerdown, mgt_loopback;
    std::optional<uint16_t> time_frame_delay;
    std::optional<bool> rx_polarity;
    std::optional<uint32_t> payload_sel, fofb_data_sel;

    void write_params() override;
};

} /* namespace fofb_cc */

#endif
