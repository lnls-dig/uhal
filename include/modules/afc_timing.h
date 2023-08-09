/*
 * Copyright (C) 2023 CNPEM (cnpem.br)
 * Author: Érico Nogueira <erico.rolim@lnls.br>
 *
 * Released according to the GNU GPL, version 3 or any later version.
 */

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

class Controller: public RegisterController {
  protected:
    std::unique_ptr<struct afc_timing> regs_storage;
    struct afc_timing &regs;

    void set_devinfo_callback() override;
    void encode_config();

  public:
    Controller(struct pcie_bars &);
    ~Controller();

    static const std::vector<std::string> sources_list;

    bool event_receiver_enable = true;
    bool reset_latches = false;
    /** Configuration values for RTM and AFC reference clocks */
    struct clock {
        uint64_t rfreq;
        uint8_t n1, hs_div;
        struct {
            uint16_t kp, ki;
        } freq_loop, phase_loop;
        struct {
            uint16_t navg;
            uint8_t div_exp;
        } ddmtd_config;
    } rtm_clock = {}, afc_clock = {};

    struct parameters {
        bool enable, polarity, log, interlock;
        std::string source = "Trigger";
        bool direction, count_reset;
        uint16_t pulses;
        uint8_t event_code;
        uint32_t delay;
        uint32_t width;
    };
    std::vector<struct parameters> parameters;

    bool set_rtm_freq(double);
    bool set_afc_freq(double);

    void write_params();

  private:
    bool set_freq(double, struct Controller::clock &);
};

} /* namespace afc_timing */

#endif
