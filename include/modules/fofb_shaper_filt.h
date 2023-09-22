/*
 * Copyright (C) 2023 CNPEM (cnpem.br)
 * Author: Érico Nogueira <erico.rolim@lnls.br>
 *
 * Released according to the GNU GPL, version 3 or any later version.
 */

#ifndef FOFB_SHAPER_FILT_H
#define FOFB_SHAPER_FILT_H

#include <vector>
#include <memory>

#include "controllers.h"
#include "decoders.h"

namespace fofb_shaper_filt {

struct filter_coefficients {
    filter_coefficients();
    std::vector<std::vector<double>> values;
};

/* forward declaration */
struct wb_iir_filt_regs;

class Core: public RegisterDecoder {
    std::unique_ptr<struct wb_iir_filt_regs> regs_storage;
    struct wb_iir_filt_regs &regs;

    void decode() override;
    void read_monitors() override;
    void decode_monitors() override;

  public:
    Core(struct pcie_bars &);
    ~Core();

    filter_coefficients coefficients;
};

class Controller: public RegisterController {
  protected:
    std::unique_ptr<struct wb_iir_filt_regs> regs_storage;
    struct wb_iir_filt_regs &regs;

    unsigned fixed_point_coeff;

    void set_devinfo_callback() override;
    void encode_params() override;

  public:
    Controller(struct pcie_bars &);
    ~Controller();

    void write_params() override;

    filter_coefficients coefficients;
};

} /* namespace fofb_shaper_filt */

#endif
