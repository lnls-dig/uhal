/*
 * Copyright (C) 2022 CNPEM (cnpem.br)
 * Author: Érico Nogueira <erico.rolim@lnls.br>
 *
 * Released according to the GNU GPL, version 3 or any later version.
 */

#include <cstring>
#include <unordered_map>

#include "pcie.h"
#include "printer.h"
#include "util.h"

namespace lamp {
#include "hw/wb_rtmlamp_ohwr_regs_v2.h"
}

#define channel_registers_v2_impl wb_rtmlamp_ohwr_regs::ch
#include "modules/lamp.h"

namespace lamp {

static constexpr unsigned CHANNEL_DISTANCE = sizeof(channel_registers_v2);
static constexpr unsigned NUM_CHAN = 12;
static constexpr unsigned TRIGGER_ENABLE_VERSION = 1;

CoreV2::CoreV2(struct pcie_bars &bars):
    RegisterDecoder(bars, {
        PRINTER("AMP_STATUS", "Amplifier flags", PrinterType::value_hex),
        PRINTER("AMP_IFLAG_L", "Amplifier Left Current Limit Flag", PrinterType::boolean, "current under limit", "current over limit"),
        PRINTER("AMP_TFLAG_L", "Amplifier Left Thermal Limit Flag", PrinterType::boolean, "temperature under limit", "temperature over limit"),
        PRINTER("AMP_IFLAG_R", "Amplifier Right Current Limit Flag", PrinterType::boolean, "current under limit", "current over limit"),
        PRINTER("AMP_TFLAG_R", "Amplifier Right Thermal Limit Flag", PrinterType::boolean, "temperature under limit", "temperature over limit"),
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
    })
{
    read_size = sizeof *regs;
    regs = std::make_unique<struct wb_rtmlamp_ohwr_regs>();
    read_dest = regs.get();

    device_match = device_match_v2;
}
CoreV2::~CoreV2() = default;

#define STA_AMP_MASK \
    (WB_RTMLAMP_OHWR_REGS_CH_STA_AMP_IFLAG_L | WB_RTMLAMP_OHWR_REGS_CH_STA_AMP_TFLAG_L | \
     WB_RTMLAMP_OHWR_REGS_CH_STA_AMP_IFLAG_R | WB_RTMLAMP_OHWR_REGS_CH_STA_AMP_TFLAG_R)

void CoreV2::decode()
{
    uint32_t t;

    /* add printer if this value ever gets flags;
     * this is being done for IOC compatibility */
    add_general("PS_STATUS", regs->sta, true);

    number_of_channels = NUM_CHAN;

    unsigned i = 0;
    auto add_channel = [this, &i](const char *name, auto value, bool skip=false) {
        channel_data[name].resize(*number_of_channels);
        add_channel_impl(name, i, value, skip);
    };

    for (const auto &channel_regs: regs->ch) {
        t = channel_regs.sta;
        /* we want AMP_STATUS to be 0 if everything is fine */
        add_channel("AMP_STATUS", (~t) & STA_AMP_MASK);
        add_channel("AMP_IFLAG_L", t & WB_RTMLAMP_OHWR_REGS_CH_STA_AMP_IFLAG_L);
        add_channel("AMP_TFLAG_L", t & WB_RTMLAMP_OHWR_REGS_CH_STA_AMP_TFLAG_L);
        add_channel("AMP_IFLAG_R", t & WB_RTMLAMP_OHWR_REGS_CH_STA_AMP_IFLAG_R);
        add_channel("AMP_TFLAG_R", t & WB_RTMLAMP_OHWR_REGS_CH_STA_AMP_TFLAG_R);

        t = channel_regs.ctl;
        add_channel("AMP_EN", t & WB_RTMLAMP_OHWR_REGS_CH_CTL_AMP_EN);
        add_channel("MODE", extract_value<uint32_t>(t, WB_RTMLAMP_OHWR_REGS_CH_CTL_MODE_MASK));
        if (devinfo.abi_ver_minor >= TRIGGER_ENABLE_VERSION) {
            add_channel("TRIG_EN", t & WB_RTMLAMP_OHWR_REGS_CH_CTL_TRIG_EN);
        } else {
            add_channel("TRIG_EN", 0, true);
        }

        add_channel("PI_KP", extract_value<uint32_t>(channel_regs.pi_kp, WB_RTMLAMP_OHWR_REGS_CH_PI_KP_DATA_MASK));
        add_channel("PI_TI", extract_value<uint32_t>(channel_regs.pi_ti, WB_RTMLAMP_OHWR_REGS_CH_PI_TI_DATA_MASK));
        add_channel("PI_SP", extract_value<int16_t>(channel_regs.pi_sp, WB_RTMLAMP_OHWR_REGS_CH_PI_SP_DATA_MASK));
        add_channel("DAC", extract_value<int16_t>(channel_regs.dac, WB_RTMLAMP_OHWR_REGS_CH_DAC_DATA_MASK));

        t = channel_regs.lim;
        add_channel("LIMIT_A", extract_value<int16_t>(t, WB_RTMLAMP_OHWR_REGS_CH_LIM_A_MASK));
        add_channel("LIMIT_B", extract_value<int16_t>(t, WB_RTMLAMP_OHWR_REGS_CH_LIM_B_MASK));

        add_channel("CNT", extract_value<uint32_t>(channel_regs.cnt, WB_RTMLAMP_OHWR_REGS_CH_CNT_DATA_MASK));

        t = channel_regs.adc_dac_eff;
        add_channel("ADC_INST", extract_value<int16_t>(t, WB_RTMLAMP_OHWR_REGS_CH_ADC_DAC_EFF_ADC_MASK));
        add_channel("DAC_EFF", extract_value<int16_t>(t, WB_RTMLAMP_OHWR_REGS_CH_ADC_DAC_EFF_DAC_MASK));

        add_channel("SP_EFF", extract_value<int16_t>(channel_regs.sp_eff, WB_RTMLAMP_OHWR_REGS_CH_SP_EFF_SP_MASK));

        /* after the first iteration, we are done adding things to the vector */
        data_order_done = true;

        i++;
    }
}

ControllerV2::ControllerV2(struct pcie_bars &bars):
    Controller(bars, device_match_v2)
{
    channel_regs = std::make_unique<channel_registers_v2>();
}
ControllerV2::~ControllerV2() = default;

void ControllerV2::encode_config()
{
    static const std::unordered_map<std::string_view, int> mode_options({
        {"open-loop-dac", 0},
        {"open-loop-square", 1},
        {"closed-loop-pi_sp", 2},
        {"closed-loop-square", 3},
        {"closed-loop-external", 4},
    });

    int mode_option;
    if (mode_numeric) mode_option = *mode_numeric;
    else try {
        mode_option = mode_options.at(mode);
    } catch (std::out_of_range &e) {
        throw std::runtime_error("mode must be one of " + list_of_keys(mode_options));
    }

    if (channel > NUM_CHAN-1)
        throw std::runtime_error("there are only 12 channels");

    bar4_read_v(&bars, addr + WB_RTMLAMP_OHWR_REGS_CH + channel * CHANNEL_DISTANCE, channel_regs.get(), CHANNEL_DISTANCE);

    clear_and_insert(channel_regs->ctl, mode_option, WB_RTMLAMP_OHWR_REGS_CH_CTL_MODE_MASK);
    insert_bit(channel_regs->ctl, amp_enable, WB_RTMLAMP_OHWR_REGS_CH_CTL_AMP_EN);

    if (trigger_enable) {
        if (devinfo.abi_ver_minor >= TRIGGER_ENABLE_VERSION)
            insert_bit(channel_regs->ctl, *trigger_enable, WB_RTMLAMP_OHWR_REGS_CH_CTL_TRIG_EN);
        else if (*trigger_enable)
            throw std::runtime_error("this core doesn't support trigger_enable");
    }

    if (pi_kp) clear_and_insert(channel_regs->pi_kp, *pi_kp, WB_RTMLAMP_OHWR_REGS_CH_PI_KP_DATA_MASK);
    if (pi_ti) clear_and_insert(channel_regs->pi_ti, *pi_ti, WB_RTMLAMP_OHWR_REGS_CH_PI_TI_DATA_MASK);
    if (pi_sp) clear_and_insert(channel_regs->pi_sp, (uint16_t)*pi_sp, WB_RTMLAMP_OHWR_REGS_CH_PI_SP_DATA_MASK);

    if (dac) clear_and_insert(channel_regs->dac, (uint16_t)*dac, WB_RTMLAMP_OHWR_REGS_CH_DAC_DATA_MASK);

    if (limit_a) clear_and_insert(channel_regs->lim, (uint16_t)*limit_a, WB_RTMLAMP_OHWR_REGS_CH_LIM_A_MASK);
    if (limit_b) clear_and_insert(channel_regs->lim, (uint16_t)*limit_b, WB_RTMLAMP_OHWR_REGS_CH_LIM_B_MASK);

    if (cnt) clear_and_insert(channel_regs->cnt, *cnt, WB_RTMLAMP_OHWR_REGS_CH_CNT_DATA_MASK);
}

void ControllerV2::write_params()
{
    encode_config();

    bar4_write_v(&bars, addr + WB_RTMLAMP_OHWR_REGS_CH + channel * CHANNEL_DISTANCE, channel_regs.get(), CHANNEL_DISTANCE);
}

} /* namespace lamp */
