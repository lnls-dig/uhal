#ifndef FMC_ADC_COMMON_H
#define FMC_ADC_COMMON_H

#include <memory>

#include "controllers.h"
#include "decoders.h"

namespace fmc_adc_common {

struct fmc_adc_common;

class Core: public RegisterDecoder {
    std::unique_ptr<struct fmc_adc_common> regs_storage;
    struct fmc_adc_common &regs;

    void decode() override;

  public:
    Core(struct pcie_bars &);
    ~Core() override;
};

class Controller: public RegisterDecoderController {
    std::unique_ptr<struct fmc_adc_common> regs_storage;
    struct fmc_adc_common &regs;

    Core dec;

  public:
    Controller(struct pcie_bars &);
    ~Controller();
};

} /* namespace fmc_adc_common */

#endif
