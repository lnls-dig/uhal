/*
 * Copyright (C) 2022 CNPEM (cnpem.br)
 * Author: Érico Nogueira <erico.rolim@lnls.br>
 *
 * Released according to the GNU GPL, version 3 or any later version.
 */

#include <cstring>

#include "pcie.h"
#include "printer.h"
#include "util.h"

#include "modules/lamp.h"

namespace lamp {
#include "hw/wb_rtmlamp_ohwr_regs.h"

namespace {
    static constexpr unsigned NUM_CHAN = 12;
    static constexpr unsigned TRIGGER_ENABLE_VERSION = 1;

    constexpr unsigned LAMP_DEVID = 0xa1248bec;
    struct sdb_device_info ref_devinfo = {
        .vendor_id = LNLS_VENDORID,
        .device_id = LAMP_DEVID,
        .abi_ver_major = 2
    };
}

const std::vector<std::string> mode_list({
    "open_loop_manual",
    "open_loop_test_sqr",
    "closed_loop_manual",
    "closed_loop_test_sqr",
    "closed_loop_fofb",
});

Core::Core(struct pcie_bars &bars):
    RegisterDecoder(bars, ref_devinfo, {
        PRINTER("AMP_STATUS", "Amplifier flags", PrinterType::value_hex),
        PRINTER("AMP_IFLAG_L", "Amplifier Left Current Limit Flag", PrinterType::boolean, "current under limit", "current over limit"),
        PRINTER("AMP_TFLAG_L", "Amplifier Left Thermal Limit Flag", PrinterType::boolean, "temperature under limit", "temperature over limit"),
        PRINTER("AMP_IFLAG_R", "Amplifier Right Current Limit Flag", PrinterType::boolean, "current under limit", "current over limit"),
        PRINTER("AMP_TFLAG_R", "Amplifier Right Thermal Limit Flag", PrinterType::boolean, "temperature under limit", "temperature over limit"),
        PRINTER("AMP_STATUS_LATCH", "Amplifier flags", PrinterType::value_hex),
        PRINTER("AMP_IFLAG_L_LATCH", "Amplifier Left Current Limit Flag", PrinterType::boolean, "current under limit", "current over limit"),
        PRINTER("AMP_TFLAG_L_LATCH", "Amplifier Left Thermal Limit Flag", PrinterType::boolean, "temperature under limit", "temperature over limit"),
        PRINTER("AMP_IFLAG_R_LATCH", "Amplifier Right Current Limit Flag", PrinterType::boolean, "current under limit", "current over limit"),
        PRINTER("AMP_TFLAG_R_LATCH", "Amplifier Right Thermal Limit Flag", PrinterType::boolean, "temperature under limit", "temperature over limit"),
        PRINTER("AMP_EN", "Amplifier Enable", PrinterType::boolean),
        /* TODO: add test mode when it becomes a single value */
        PRINTER("MODE", "Power supply operation mode", PrinterType::custom_function,
            [](FILE *f, bool v, uint32_t value){
                (void)v;
                static const char *modes[8] = {
                    "Open loop (voltage) manual control via dac",
                    "Open loop (voltage) test square wave",
                    "Closed loop (current) manual control via pi_sp",
                    "Closed loop (current) test square wave",
                    "Closed loop (current) external control",
                    "reserved",
                    "reserved",
                    "reserved"
                };

                fputs(modes[value], f);
                fputc('\n', f);
            }
        ),
        PRINTER("TRIG_EN", "Trigger enable", PrinterType::enable),
        PRINTER("PI_KP", "PI KP Coefficient", PrinterType::value),
        PRINTER("PI_TI", "PI TI Coefficient", PrinterType::value),
        PRINTER("PI_SP", "PI Setpoint", PrinterType::value),
        PRINTER("DAC", "DAC Data For Channel", PrinterType::value),
        PRINTER("LIMIT_A", "Signed limit 'a'", PrinterType::value),
        PRINTER("LIMIT_B", "Signed limit 'b'", PrinterType::value),
        PRINTER("CNT", "Test mode period, in clock ticks", PrinterType::value),
        PRINTER("ADC_INST", "ADC instantaneous measurement", PrinterType::value),
        PRINTER("DAC_EFF", "DAC effective measurement - actual value sent to DAC", PrinterType::value),
        PRINTER("SP_EFF", "Set point instantaneous effective data", PrinterType::value),
    }),
    CONSTRUCTOR_REGS(struct wb_rtmlamp_ohwr_regs)
{
    set_read_dest(regs);
}
Core::~Core() = default;

#define STA_AMP_MASK \
    (WB_RTMLAMP_OHWR_REGS_CH_STA_AMP_IFLAG_L | WB_RTMLAMP_OHWR_REGS_CH_STA_AMP_TFLAG_L | \
     WB_RTMLAMP_OHWR_REGS_CH_STA_AMP_IFLAG_R | WB_RTMLAMP_OHWR_REGS_CH_STA_AMP_TFLAG_R)
#define STA_AMP_LATCH_MASK \
    (WB_RTMLAMP_OHWR_REGS_CH_STA_AMP_IFLAG_L_LATCH | WB_RTMLAMP_OHWR_REGS_CH_STA_AMP_TFLAG_L_LATCH | \
     WB_RTMLAMP_OHWR_REGS_CH_STA_AMP_IFLAG_R_LATCH | WB_RTMLAMP_OHWR_REGS_CH_STA_AMP_TFLAG_R_LATCH)

void Core::decode()
{
    uint32_t t, *pt;

    /* add printer if this value ever gets flags;
     * this is being done for IOC compatibility */
    add_general("PS_STATUS", regs.sta);

    number_of_channels = NUM_CHAN;

    unsigned i = 0;
    for (auto &channel_regs: regs.ch) {
        t = channel_regs.sta;
        /* we want AMP_STATUS to be 0 if everything is fine */
        add_channel("AMP_STATUS", i, extract_value(~t, STA_AMP_MASK));
        add_channel("AMP_IFLAG_L", i, get_bit(t, WB_RTMLAMP_OHWR_REGS_CH_STA_AMP_IFLAG_L));
        add_channel("AMP_TFLAG_L", i, get_bit(t, WB_RTMLAMP_OHWR_REGS_CH_STA_AMP_TFLAG_L));
        add_channel("AMP_IFLAG_R", i, get_bit(t, WB_RTMLAMP_OHWR_REGS_CH_STA_AMP_IFLAG_R));
        add_channel("AMP_TFLAG_R", i, get_bit(t, WB_RTMLAMP_OHWR_REGS_CH_STA_AMP_TFLAG_R));

        add_channel("AMP_STATUS_LATCH", i, extract_value(~t, STA_AMP_LATCH_MASK));
        add_channel("AMP_IFLAG_L_LATCH", i, get_bit(t, WB_RTMLAMP_OHWR_REGS_CH_STA_AMP_IFLAG_L_LATCH));
        add_channel("AMP_TFLAG_L_LATCH", i, get_bit(t, WB_RTMLAMP_OHWR_REGS_CH_STA_AMP_TFLAG_L_LATCH));
        add_channel("AMP_IFLAG_R_LATCH", i, get_bit(t, WB_RTMLAMP_OHWR_REGS_CH_STA_AMP_IFLAG_R_LATCH));
        add_channel("AMP_TFLAG_R_LATCH", i, get_bit(t, WB_RTMLAMP_OHWR_REGS_CH_STA_AMP_TFLAG_R_LATCH));

        pt = &channel_regs.ctl;
        add_channel("AMP_EN", i, rf_get_bit(*pt, WB_RTMLAMP_OHWR_REGS_CH_CTL_AMP_EN));
        add_channel("MODE", i, rf_extract_value(*pt, WB_RTMLAMP_OHWR_REGS_CH_CTL_MODE_MASK));
        if (devinfo.abi_ver_minor >= TRIGGER_ENABLE_VERSION) {
            add_channel("TRIG_EN", i, rf_get_bit(*pt, WB_RTMLAMP_OHWR_REGS_CH_CTL_TRIG_EN));
        } else {
            add_channel("TRIG_EN", i, 0);
        }
        add_channel("RST_LATCH", i, rf_get_bit(*pt, WB_RTMLAMP_OHWR_REGS_CH_CTL_RST_LATCH_STS));

        add_channel("PI_KP", i, rf_extract_value(channel_regs.pi_kp, WB_RTMLAMP_OHWR_REGS_CH_PI_KP_DATA_MASK));
        add_channel("PI_TI", i, rf_extract_value(channel_regs.pi_ti, WB_RTMLAMP_OHWR_REGS_CH_PI_TI_DATA_MASK));
        add_channel("PI_SP", i, rf_extract_value(channel_regs.pi_sp, WB_RTMLAMP_OHWR_REGS_CH_PI_SP_DATA_MASK, true));
        add_channel("DAC", i, rf_extract_value(channel_regs.dac, WB_RTMLAMP_OHWR_REGS_CH_DAC_DATA_MASK, true));

        pt = &channel_regs.lim;
        add_channel("LIMIT_A", i, rf_extract_value(*pt, WB_RTMLAMP_OHWR_REGS_CH_LIM_A_MASK, true));
        add_channel("LIMIT_B", i, rf_extract_value(*pt, WB_RTMLAMP_OHWR_REGS_CH_LIM_B_MASK, true));

        add_channel("CNT", i, rf_extract_value(channel_regs.cnt, WB_RTMLAMP_OHWR_REGS_CH_CNT_DATA_MASK));

        pt = &channel_regs.adc_dac_eff;
        add_channel("ADC_INST", i, rf_extract_value(*pt, WB_RTMLAMP_OHWR_REGS_CH_ADC_DAC_EFF_ADC_MASK, true));
        add_channel("DAC_EFF", i, extract_value(*pt, WB_RTMLAMP_OHWR_REGS_CH_ADC_DAC_EFF_DAC_MASK, true));

        add_channel("SP_EFF", i, rf_extract_value(channel_regs.sp_eff, WB_RTMLAMP_OHWR_REGS_CH_SP_EFF_SP_MASK, true));

        i++;
    }
}

Controller::Controller(struct pcie_bars &bars):
    RegisterDecoderController(bars, ref_devinfo, &dec),
    CONSTRUCTOR_REGS(struct wb_rtmlamp_ohwr_regs),
    dec(bars)
{
    set_read_dest(regs);
}
Controller::~Controller() = default;

void Controller::unset_commands()
{
    for (unsigned i = 0; i < NUM_CHAN; i++)
        write_channel("RST_LATCH", i, 0);
}

} /* namespace lamp */
