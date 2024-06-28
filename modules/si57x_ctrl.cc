#include "printer.h"
#include "util.h"
#include "si57x_util.h"
#include "modules/si57x_ctrl.h"

namespace si57x_ctrl {

#include "hw/wb_si57x_ctrl_regs.h"

namespace {
    constexpr unsigned SI57X_CTRL_DEVID = 0x293c7542;
    struct sdb_device_info ref_devinfo = {
        .vendor_id = LNLS_VENDORID,
        .device_id = SI57X_CTRL_DEVID,
        .abi_ver_major = 1
    };
}

Core::Core(struct pcie_bars &bars):
    RegisterDecoder(bars, ref_devinfo, {
        PRINTER("STRP_COMPLETE", "Startup registers status", "invalid", "valid"),
        PRINTER("CFG_IN_SYNC", "Registers synchronization status", "values NOT in sync", "values in sync"),
        PRINTER("I2C_ERR", "I2C error status", "No errors", "I2C error ocurred"),
        PRINTER("BUSY", "Controller busy status", PrinterType::boolean),
        PRINTER("RFREQ_MSB_STRP", "RFREQ startup value (most significant bits)", PrinterType::value_hex),
        PRINTER("N1_STRP", "N1 startup value", PrinterType::value),
        PRINTER("HSDIV_STRP", "HSDIV startup value", PrinterType::value),
        PRINTER("RFREQ_LSB_STRP", "RFREQ startup value (least significant bits)", PrinterType::value_hex),
        PRINTER("RFREQ_MSB", "RFREQ value (most significant bits)", PrinterType::value_hex),
        PRINTER("N1", "N1 value", PrinterType::value),
        PRINTER("HSDIV", "HSDIV value", PrinterType::value),
        PRINTER("RFREQ_LSB", "RFREQ value (least significant bits)", PrinterType::value_hex),
    }),
    CONSTRUCTOR_REGS(struct wb_si57x_ctrl_regs)
{
    set_read_dest(regs);
}
Core::~Core() = default;

void Core::decode()
{
    add_general("READ_STRP_REGS", rf_get_bit(regs.ctl, WB_SI57X_CTRL_REGS_CTL_READ_STRP_REGS));
    add_general("APPLY_CFG", rf_get_bit(regs.ctl, WB_SI57X_CTRL_REGS_CTL_APPLY_CFG));

    add_general("STRP_COMPLETE", get_bit(regs.sta, WB_SI57X_CTRL_REGS_STA_STRP_COMPLETE));
    add_general("CFG_IN_SYNC", get_bit(regs.sta, WB_SI57X_CTRL_REGS_STA_CFG_IN_SYNC));
    add_general("I2C_ERR", get_bit(regs.sta, WB_SI57X_CTRL_REGS_STA_I2C_ERR));
    add_general("BUSY", get_bit(regs.sta, WB_SI57X_CTRL_REGS_STA_BUSY));

    add_general("RFREQ_MSB_STRP", extract_value(regs.hsdiv_n1_rfreq_msb_strp, WB_SI57X_CTRL_REGS_HSDIV_N1_RFREQ_MSB_STRP_RFREQ_MSB_STRP_MASK));
    add_general("N1_STRP", extract_value(regs.hsdiv_n1_rfreq_msb_strp, WB_SI57X_CTRL_REGS_HSDIV_N1_RFREQ_MSB_STRP_N1_STRP_MASK));
    add_general("HSDIV_STRP", extract_value(regs.hsdiv_n1_rfreq_msb_strp, WB_SI57X_CTRL_REGS_HSDIV_N1_RFREQ_MSB_STRP_HSDIV_STRP_MASK));

    add_general("RFREQ_LSB_STRP", regs.rfreq_lsb_strp);

    add_general("RFREQ_MSB", rf_extract_value(regs.hsdiv_n1_rfreq_msb, WB_SI57X_CTRL_REGS_HSDIV_N1_RFREQ_MSB_RFREQ_MSB_MASK, false));
    add_general("N1", rf_extract_value(regs.hsdiv_n1_rfreq_msb, WB_SI57X_CTRL_REGS_HSDIV_N1_RFREQ_MSB_N1_MASK, false));
    add_general("HSDIV", rf_extract_value(regs.hsdiv_n1_rfreq_msb, WB_SI57X_CTRL_REGS_HSDIV_N1_RFREQ_MSB_HSDIV_MASK, false));

    add_general("RFREQ_LSB", rf_whole_register(regs.rfreq_lsb, false));
}

Controller::Controller(struct pcie_bars &bars, double fstartup):
    RegisterDecoderController(bars, ref_devinfo, &dec),
    CONSTRUCTOR_REGS(struct wb_si57x_ctrl_regs),
    dec(bars),
    fstartup(fstartup),
    fxtal(0)
{
    set_read_dest(regs);
}
Controller::~Controller() = default;

void Controller::write_params()
{
    dec.get_data();
    if (dec.get_general_data<int32_t>("BUSY")) {
        /* clear any action bits */
        regs.ctl = 0;
        throw std::runtime_error("writes not allowed while busy");
    }

    RegisterDecoderController::write_params();

    regs.ctl = 0;
}

bool Controller::read_startup_regs()
{
    if (fstartup == 0)
        throw std::logic_error("read_startup_regs requires fstartup");

    if (!dec.get_general_data<int32_t>("STRP_COMPLETE")) {
        write_general("READ_STRP_REGS", 1);
        write_params();

        do {
            dec.get_data();
        } while (dec.get_general_data<int32_t>("BUSY"));

        if (!dec.get_general_data<int32_t>("STRP_COMPLETE"))
            return false;
    }

    si57x_parameters si57x;
    si57x.fstartup = fstartup;

    uint64_t rfreq_msb = dec.get_general_data<int32_t>("RFREQ_MSB_STRP");
    uint32_t rfreq_lsb = dec.get_general_data<int32_t>("RFREQ_LSB_STRP");

    si57x.rfreq = (rfreq_msb << 32) | rfreq_lsb;

    si57x.hs_div = dec.get_general_data<int32_t>("HSDIV_STRP");
    si57x.n1 = dec.get_general_data<int32_t>("N1_STRP");

    si57x.calc_fxtal();
    fxtal = si57x.fxtal;

    return true;
}

bool Controller::apply_config()
{
    /* write changes to registers so they go into effect with apply_cfg=1 */
    write_params();

    write_general("APPLY_CFG", 1);
    write_params();

    do {
        dec.get_data();
    } while (dec.get_general_data<int32_t>("BUSY"));

    if (!dec.get_general_data<int32_t>("CFG_IN_SYNC"))
        return false;

    return true;
}

bool Controller::set_freq(double freq)
{
    si57x_parameters params;
    /* if the user hasn't configured fxtal, use the nominal value */
    if (fxtal != 0)
        params.fxtal = fxtal;

    if (!params.set_freq(freq))
        return false;

    int32_t rfreq_msb = params.rfreq >> 32;
    int32_t rfreq_lsb = (uint32_t)params.rfreq;
    write_general("RFREQ_MSB", rfreq_msb);
    write_general("RFREQ_LSB", rfreq_lsb);
    write_general("HSDIV", (int32_t)params.hs_div);
    write_general("N1", (int32_t)params.n1);

    return true;
}

} /* namespace si57x_ctrl */
