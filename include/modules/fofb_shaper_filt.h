#ifndef FOFB_SHAPER_FILT_H
#define FOFB_SHAPER_FILT_H

#include <vector>
#include <memory>

#include "controllers.h"
#include "decoders.h"

namespace fofb_shaper_filt {

struct filter_coefficients {
    filter_coefficients();
    void set_num_biquads(size_t);
    std::vector<std::vector<double>> values;
};

/* forward declaration */
struct wb_fofb_shaper_filt_regs;

class Core: public RegisterDecoder {
    std::unique_ptr<struct wb_fofb_shaper_filt_regs> regs_storage;
    struct wb_fofb_shaper_filt_regs &regs;

    void decode() override;
    void read_monitors() override;
    void decode_monitors() override;

  public:
    Core(struct pcie_bars &);
    ~Core() override;

    void print(FILE *, bool) const override;

    filter_coefficients coefficients;
};

class Controller: public RegisterController {
  protected:
    std::unique_ptr<struct wb_fofb_shaper_filt_regs> regs_storage;
    struct wb_fofb_shaper_filt_regs &regs;

    unsigned fixed_point_coeff, num_biquads;

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
