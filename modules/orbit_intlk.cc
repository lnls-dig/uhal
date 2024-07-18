#include <stdexcept>

#include "pcie.h"
#include "printer.h"
#include "util.h"
#include "modules/orbit_intlk.h"

namespace orbit_intlk {

#include "hw/orbit_intlk_regs.h"

namespace {
    constexpr unsigned ORBIT_INTLK_DEVID = 0x87efeda8;
    struct sdb_device_info ref_devinfo = {
        .vendor_id = LNLS_VENDORID,
        .device_id = ORBIT_INTLK_DEVID,
        .abi_ver_major = 1
    };
}

struct orbit_intlk_regs {
    uint32_t ctrl;
    uint32_t sts;
    uint32_t min_sum;
    struct {
        struct {
            uint32_t x;
            uint32_t y;
        } trans, ang;
    } max, min;
    struct {
        uint32_t x;
        uint32_t y;
    } trans_diff, ang_diff;
};

static_assert(sizeof(struct orbit_intlk_regs) == ORBIT_INTLK_REG_ANG_Y_DIFF + sizeof(uint32_t));
static_assert(offsetof(orbit_intlk_regs, min.trans.y) == ORBIT_INTLK_REG_TRANS_MIN_Y);
static_assert(offsetof(orbit_intlk_regs, trans_diff.y) == ORBIT_INTLK_REG_TRANS_Y_DIFF);

Core::Core(struct pcie_bars &bars):
    RegisterDecoder(bars, ref_devinfo, {
        PRINTER("EN", "Interlock enable", PrinterType::enable),
        PRINTER("MIN_SUM_EN", "Interlock minimum sum enable", PrinterType::enable),
        PRINTER("POS_EN", "Position Interlock enable", PrinterType::enable),
        PRINTER("ANG_EN", "Angular Interlock enable", PrinterType::enable),
        PRINTER("POS_UPPER_X", "", PrinterType::boolean),
        PRINTER("POS_UPPER_Y", "", PrinterType::boolean),
        PRINTER("POS_UPPER_LTC_X", "", PrinterType::boolean),
        PRINTER("POS_UPPER_LTC_Y", "", PrinterType::boolean),
        PRINTER("ANG_UPPER_X", "", PrinterType::boolean),
        PRINTER("ANG_UPPER_Y", "", PrinterType::boolean),
        PRINTER("ANG_UPPER_LTC_X", "", PrinterType::boolean),
        PRINTER("ANG_UPPER_LTC_Y", "", PrinterType::boolean),
        PRINTER("INTLK", "Interlock Trip", PrinterType::boolean),
        PRINTER("INTLK_LTC", "Interlock Trip Latch", PrinterType::boolean),
        PRINTER("POS_LOWER_X", "", PrinterType::boolean),
        PRINTER("POS_LOWER_Y", "", PrinterType::boolean),
        PRINTER("POS_LOWER_LTC_X", "", PrinterType::boolean),
        PRINTER("POS_LOWER_LTC_Y", "", PrinterType::boolean),
        PRINTER("ANG_LOWER_X", "", PrinterType::boolean),
        PRINTER("ANG_LOWER_Y", "", PrinterType::boolean),
        PRINTER("ANG_LOWER_LTC_X", "", PrinterType::boolean),
        PRINTER("ANG_LOWER_LTC_Y", "", PrinterType::boolean),
        PRINTER("MIN_SUM", "Minimum Sum Threshold", PrinterType::value),
        PRINTER("POS_MAX_X", "Maximum X Threshold", PrinterType::value),
        PRINTER("POS_MAX_Y", "Maximum Y Threshold", PrinterType::value),
        PRINTER("ANG_MAX_X", "Maximum X Threshold", PrinterType::value),
        PRINTER("ANG_MAX_Y", "Maximum Y Threshold", PrinterType::value),
        PRINTER("POS_MIN_X", "Minimum X Threshold", PrinterType::value),
        PRINTER("POS_MIN_Y", "Minimum Y Threshold", PrinterType::value),
        PRINTER("ANG_MIN_X", "Minimum X Threshold", PrinterType::value),
        PRINTER("ANG_MIN_Y", "Minimum Y Threshold", PrinterType::value),
        PRINTER("POS_X_INST", "Instantaneous X Position", PrinterType::value),
        PRINTER("POS_Y_INST", "Instantaneous Y Position", PrinterType::value),
        PRINTER("ANG_X_INST", "Instantaneous X Angle", PrinterType::value),
        PRINTER("ANG_Y_INST", "Instantaneous Y Angle", PrinterType::value),
    }),
    CONSTRUCTOR_REGS(struct orbit_intlk_regs)
{
    set_read_dest(regs);
}
Core::~Core() = default;

void Core::decode()
{
    uint32_t t;

    t = regs.ctrl;
    add_general("EN", get_bit(t, ORBIT_INTLK_CTRL_EN));
    add_general("MIN_SUM_EN", get_bit(t, ORBIT_INTLK_CTRL_MIN_SUM_EN));
    add_general("POS_EN", get_bit(t, ORBIT_INTLK_CTRL_TRANS_EN));
    add_general("ANG_EN", get_bit(t, ORBIT_INTLK_CTRL_ANG_EN));

    t = regs.sts;
    add_general("POS_UPPER_X", get_bit(t, ORBIT_INTLK_STS_TRANS_BIGGER_X));
    add_general("POS_UPPER_Y", get_bit(t, ORBIT_INTLK_STS_TRANS_BIGGER_Y));
    add_general("POS_UPPER_LTC_X", get_bit(t, ORBIT_INTLK_STS_TRANS_BIGGER_LTC_X));
    add_general("POS_UPPER_LTC_Y", get_bit(t, ORBIT_INTLK_STS_TRANS_BIGGER_LTC_Y));
    add_general("ANG_UPPER_X", get_bit(t, ORBIT_INTLK_STS_ANG_BIGGER_X));
    add_general("ANG_UPPER_Y", get_bit(t, ORBIT_INTLK_STS_ANG_BIGGER_Y));
    add_general("ANG_UPPER_LTC_X", get_bit(t, ORBIT_INTLK_STS_ANG_BIGGER_LTC_X));
    add_general("ANG_UPPER_LTC_Y", get_bit(t, ORBIT_INTLK_STS_ANG_BIGGER_LTC_Y));
    add_general("INTLK", get_bit(t, ORBIT_INTLK_STS_INTLK));
    add_general("INTLK_LTC", get_bit(t, ORBIT_INTLK_STS_INTLK_LTC));
    add_general("POS_LOWER_X", get_bit(t, ORBIT_INTLK_STS_TRANS_SMALLER_X));
    add_general("POS_LOWER_Y", get_bit(t, ORBIT_INTLK_STS_TRANS_SMALLER_Y));
    add_general("POS_LOWER_LTC_X", get_bit(t, ORBIT_INTLK_STS_TRANS_SMALLER_LTC_X));
    add_general("POS_LOWER_LTC_Y", get_bit(t, ORBIT_INTLK_STS_TRANS_SMALLER_LTC_Y));
    add_general("ANG_LOWER_X", get_bit(t, ORBIT_INTLK_STS_ANG_SMALLER_X));
    add_general("ANG_LOWER_Y", get_bit(t, ORBIT_INTLK_STS_ANG_SMALLER_Y));
    add_general("ANG_LOWER_LTC_X", get_bit(t, ORBIT_INTLK_STS_ANG_SMALLER_LTC_X));
    add_general("ANG_LOWER_LTC_Y", get_bit(t, ORBIT_INTLK_STS_ANG_SMALLER_LTC_Y));

    add_general("MIN_SUM", regs.min_sum);
    add_general("POS_MAX_X", regs.max.trans.x);
    add_general("POS_MAX_Y", regs.max.trans.y);
    add_general("ANG_MAX_X", regs.max.ang.x);
    add_general("ANG_MAX_Y", regs.max.ang.y);
    add_general("POS_MIN_X", regs.min.trans.x);
    add_general("POS_MIN_Y", regs.min.trans.y);
    add_general("ANG_MIN_X", regs.min.ang.x);
    add_general("ANG_MIN_Y", regs.min.ang.y);

    add_general("POS_X_INST", regs.trans_diff.x);
    add_general("POS_Y_INST", regs.trans_diff.y);
    add_general("ANG_X_INST", regs.ang_diff.x);
    add_general("ANG_Y_INST", regs.ang_diff.y);
}

Controller::Controller(struct pcie_bars &bars):
    RegisterController(bars, ref_devinfo),
    CONSTRUCTOR_REGS(struct orbit_intlk_regs)
{
    set_read_dest(regs);
}
Controller::~Controller() = default;

void Controller::encode_params()
{
    insert_bit(regs.ctrl, enable, ORBIT_INTLK_CTRL_EN);
    insert_bit(regs.ctrl, clear, ORBIT_INTLK_CTRL_CLR);
    insert_bit(regs.ctrl, min_sum_enable, ORBIT_INTLK_CTRL_MIN_SUM_EN);
    insert_bit(regs.ctrl, pos_enable, ORBIT_INTLK_CTRL_TRANS_EN);
    insert_bit(regs.ctrl, pos_clear, ORBIT_INTLK_CTRL_TRANS_CLR);
    insert_bit(regs.ctrl, ang_enable, ORBIT_INTLK_CTRL_ANG_EN);
    insert_bit(regs.ctrl, ang_clear, ORBIT_INTLK_CTRL_ANG_CLR);

    regs.min_sum = min_sum;
    regs.max.trans.x = pos_max_x;
    regs.max.trans.y = pos_max_y;
    regs.max.ang.x = ang_max_x;
    regs.max.ang.y = ang_max_y;
    regs.min.trans.x = pos_min_x;
    regs.min.trans.y = pos_min_y;
    regs.min.ang.x = ang_min_x;
    regs.min.ang.y = ang_min_y;
}

void Controller::write_params()
{
    RegisterController::write_params();

    clear = pos_clear = ang_clear = false;
}

} /* namespace orbit_intlk */
