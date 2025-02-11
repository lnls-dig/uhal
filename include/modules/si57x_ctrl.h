#ifndef SI57X_CTRL_H
#define SI57X_CTRL_H

#include <memory>

#include "controllers.h"
#include "decoders.h"

namespace si57x_ctrl {

struct wb_si57x_ctrl_regs;

class Core: public RegisterDecoder {
    std::unique_ptr<struct wb_si57x_ctrl_regs> regs_storage;
    struct wb_si57x_ctrl_regs &regs;

    void decode() override;

  public:
    Core(struct pcie_bars &);
    ~Core() override;
};

class Controller: public RegisterDecoderController {
    std::unique_ptr<struct wb_si57x_ctrl_regs> regs_storage;
    struct wb_si57x_ctrl_regs &regs;

    Core dec;

    /** The device's startup frequency. */
    double fstartup;
    /** The device's internal crystal frequency. */
    double fxtal;

    /** Update all registers and return state of busy flag. */
    bool get_busy();
    /** Loop until timeout while busy, return immediately otherwise. Updates all
     * registers every iteration. Returns the final state of the busy flag. */
    bool still_busy();

  public:
    /** The startup frequency has to be provided if read_startup_regs() is going
     * to be called. */
    Controller(struct pcie_bars &, double = 0);
    ~Controller();

    void write_params() override;
    /** Read the device's startup registers and obtain #fxtal. Will reset it if
     * necessary, as determined by the value of STRP_COMPLETE. #fstartup must be
     * set. This function blocks while the startup registers are read. Returns
     * false in case of failure. */
    bool read_startup_regs();
    /** Apply the frequency configuration provided by set_freq(). This function
     * blocks while the configuration is applied. Returns false in case of
     * failure. */
    bool apply_config();
    /** Determine the value for the device's registers in order to output the
     * desired frequency. If read_startup_regs() wasn't called, uses the nominal
     * value for #fxtal. Returns false in case of failure. */
    bool set_freq(double);
};

} /* namespace si57x_ctrl */

#endif
