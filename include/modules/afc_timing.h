#ifndef AFC_TIMING_H
#define AFC_TIMING_H

#include <memory>
#include <string>
#include <vector>

#include "controllers.h"
#include "decoders.h"

namespace afc_timing {

/* forward declaration */
struct afc_timing;

class Core: public RegisterDecoder {
    std::unique_ptr<struct afc_timing> regs_storage;
    struct afc_timing &regs;

    void decode() override;

  public:
    Core(struct pcie_bars &);
    ~Core() override;
};

class Controller: public RegisterDecoderController {
    std::unique_ptr<struct afc_timing> regs_storage;
    struct afc_timing &regs;

    Core dec;

    void encode_params() override;
    void unset_commands() override;

    /** Configuration values for RTM and AFC reference clocks */
    struct clock {
        uint64_t rfreq;
        uint8_t n1, hs_div;
    } rtm_clock = {}, afc_clock = {};

    bool set_freq(double, struct Controller::clock &);

  public:
    Controller(struct pcie_bars &);
    ~Controller();

    static const std::vector<std::string> sources_list;

    bool set_rtm_freq(double);
    bool set_afc_freq(double);
};

} /* namespace afc_timing */

#endif
