/*
 * Copyright (C) 2022 CNPEM (cnpem.br)
 * Author: Érico Nogueira <erico.rolim@lnls.br>
 *
 * Released according to the GNU GPL, version 3 or any later version.
 */

#ifndef FOFB_PROCESSING_H
#define FOFB_PROCESSING_H

#include <memory>
#include <vector>

#include "controllers.h"
#include "decoders.h"

namespace fofb_processing {

constexpr unsigned FOFB_PROCESSING_DEVID = 0x49681ca6;

/* forward declaration */
struct wb_fofb_processing_regs;

class Core: public RegisterDecoder {
    std::unique_ptr<struct wb_fofb_processing_regs> regs_storage;
    struct wb_fofb_processing_regs &regs;

    void decode() override;
    void read_monitors() override;
    void print(FILE *, bool) const override;

  public:
    Core(struct pcie_bars &);
    ~Core();

    std::vector<int32_t> ref_orb_x, ref_orb_y;
    std::vector<std::vector<double>> coefficients_x, coefficients_y;
};

class Controller: public RegisterController {
  protected:
    std::unique_ptr<struct wb_fofb_processing_regs> regs_storage;
    struct wb_fofb_processing_regs &regs;

    void set_devinfo_callback() override;
    void encode_config();

    unsigned fixed_point_coeff, fixed_point_gains;

  public:
    Controller(struct pcie_bars &);
    ~Controller();

    bool intlk_sta_clr = false, intlk_en_orb_distort = false, intlk_en_packet_loss = false;
    unsigned orb_distort_limit = 0, min_num_packets = 0;

    std::vector<int32_t> ref_orb_x, ref_orb_y;

    struct parameters {
        std::vector<double> coefficients_x, coefficients_y;
        bool acc_clear = false, acc_freeze = false;
        double acc_gain = 0;
        uint32_t sp_limit_max = 0, sp_limit_min = 0;
        /** Decimation ratio must be at least 1 */
        uint32_t sp_decim_ratio = 1;
    };
    std::vector<struct parameters> parameters;

    void write_params();
};

} /* namespace fofb_processing */

#endif
