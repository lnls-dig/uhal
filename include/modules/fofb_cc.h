/*
 * Copyright (C) 2022 CNPEM (cnpem.br)
 * Author: Érico Nogueira <erico.rolim@lnls.br>
 *
 * Released according to the GNU GPL, version 3 or any later version.
 */

#ifndef FOFB_CC_H
#define FOFB_CC_H

#include <memory>

#include "controllers.h"
#include "decoders.h"

namespace fofb_cc {

constexpr uint64_t DLS_VENDORID = 0x1000000000000d15;
constexpr unsigned FOFB_CC_DEVID = 0x4a1df147;

inline const device_match_fn device_match_fofb_cc =
    device_match_impl<DLS_VENDORID, FOFB_CC_DEVID, 1>;

/* forward declaration */
struct fofb_cc_regs;

class Core: public RegisterDecoder {
    std::unique_ptr<struct fofb_cc_regs> regs_storage;
    struct fofb_cc_regs &regs;

    void decode();

  public:
    Core(struct pcie_bars &);
    ~Core();
};

class Controller: public RegisterController {
  protected:
    std::unique_ptr<struct fofb_cc_regs> regs_storage;
    struct fofb_cc_regs &regs;

    void get_internal_values();
    void encode_config();

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

    static inline const device_match_fn device_match = device_match_fofb_cc;

    void write_params();
};

} /* namespace fofb_cc */

#endif
