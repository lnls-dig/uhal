/*
 * Copyright (C) 2022 CNPEM (cnpem.br)
 * Author: Érico Nogueira <erico.rolim@lnls.br>
 *
 * Released according to the GNU GPL, version 3 or any later version.
 */

#include <cstring>
#include <unordered_map>

#include "printer.h"
#include "util.h"

#include "hw/wb_rtmlamp_ohwr_regs_v2.h"

#define channel_registers_v2_impl wb_rtmlamp_ohwr_regs::ch
#include "lamp.h"

static const unsigned max_26bits = 0x3ffffff;
static const unsigned max_22bits = 0x3fffff;
static const unsigned channel_distance = sizeof(channel_registers_v2);

LnlsRtmLampCoreV2::LnlsRtmLampCoreV2()
{
    read_size = sizeof *regs;
    regs = std::make_unique<struct wb_rtmlamp_ohwr_regs>();
    read_dest = regs.get();

    device_match = device_match_lamp_v2;
}
LnlsRtmLampCoreV2::~LnlsRtmLampCoreV2() = default;

void LnlsRtmLampCoreV2::print(FILE *f, bool verbose)
{
    static const std::unordered_map<const char *, Printer> printers({
      /* per channel info */
      I("AMP_IFLAG_L", "Amplifier Left Current Limit Flag", PrinterType::boolean, "current under limit", "current over limit"),
      I("AMP_TFLAG_L", "Amplifier Left Thermal Limit Flag", PrinterType::boolean, "temperature under limit", "temperature over limit"),
      I("AMP_IFLAG_R", "Amplifier Right Current Limit Flag", PrinterType::boolean, "current under limit", "current over limit"),
      I("AMP_TFLAG_R", "Amplifier Right Thermal Limit Flag", PrinterType::boolean, "temperature under limit", "temperature over limit"),
      I("AMP_EN", "Amplifier Enable", PrinterType::boolean),
      /* TODO: add test mode when it becomes a single value */
      I("MODE", "Power supply operation mode", PrinterType::custom_function,
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
      I("PI_KP", "PI KP Coefficient", PrinterType::value),
      I("PI_TI", "PI TI Coefficient", PrinterType::value),
      I("PI_SP", "PI Setpoint", PrinterType::value_2c),
      I("DAC", "DAC Data For Channel", PrinterType::value),
      I("Limit A", "Signed limit 'a'", PrinterType::value_2c),
      I("Limit B", "Signed limit 'b'", PrinterType::value_2c),
      I("CNT", "Test mode period, in clock ticks", PrinterType::value),
    });

    bool v = verbose;
    unsigned indent = 0;
    uint32_t t;

    auto print = [f, v, &indent](const char *name, uint32_t value) {
        printers.at(name).print(f, v, indent, value);
    };

    unsigned i = 0;
    for (const auto &channel_regs: regs->ch) {
        fprintf(f, "channel %u:\n", i++);
        indent = 4;

        t = channel_regs.sta;
        print("AMP_IFLAG_L", t & WB_RTMLAMP_OHWR_REGS_CH_STA_AMP_IFLAG_L);
        print("AMP_TFLAG_L", t & WB_RTMLAMP_OHWR_REGS_CH_STA_AMP_TFLAG_L);
        print("AMP_IFLAG_R", t & WB_RTMLAMP_OHWR_REGS_CH_STA_AMP_IFLAG_R);
        print("AMP_TFLAG_R", t & WB_RTMLAMP_OHWR_REGS_CH_STA_AMP_TFLAG_R);

        t = channel_regs.ctl;
        print("AMP_EN", t & WB_RTMLAMP_OHWR_REGS_CH_CTL_AMP_EN);
        print("MODE", (t & WB_RTMLAMP_OHWR_REGS_CH_CTL_MODE_MASK) >> WB_RTMLAMP_OHWR_REGS_CH_CTL_MODE_SHIFT);

        print("PI_KP", (channel_regs.pi_kp & WB_RTMLAMP_OHWR_REGS_CH_PI_KP_DATA_MASK) >> WB_RTMLAMP_OHWR_REGS_CH_PI_KP_DATA_SHIFT);
        print("PI_TI", (channel_regs.pi_ti & WB_RTMLAMP_OHWR_REGS_CH_PI_TI_DATA_MASK) >> WB_RTMLAMP_OHWR_REGS_CH_PI_TI_DATA_SHIFT);
        print("PI_SP", (int32_t)(int16_t)((channel_regs.pi_sp & WB_RTMLAMP_OHWR_REGS_CH_PI_SP_DATA_MASK) >> WB_RTMLAMP_OHWR_REGS_CH_PI_SP_DATA_SHIFT));
        print("DAC", (channel_regs.dac & WB_RTMLAMP_OHWR_REGS_CH_DAC_DATA_MASK) >> WB_RTMLAMP_OHWR_REGS_CH_DAC_DATA_SHIFT);

        t = channel_regs.lim;
        print("Limit A", (int32_t)(int16_t)((t & WB_RTMLAMP_OHWR_REGS_CH_LIM_A_MASK) >> WB_RTMLAMP_OHWR_REGS_CH_LIM_A_SHIFT));
        print("Limit B", (int32_t)(int16_t)((t & WB_RTMLAMP_OHWR_REGS_CH_LIM_B_MASK) >> WB_RTMLAMP_OHWR_REGS_CH_LIM_B_SHIFT));

        print("CNT", (channel_regs.cnt & WB_RTMLAMP_OHWR_REGS_CH_CNT_DATA_MASK) >> WB_RTMLAMP_OHWR_REGS_CH_CNT_DATA_SHIFT);
    }
}

LnlsRtmLampControllerV2::LnlsRtmLampControllerV2(const struct pcie_bars *bars, size_t addr):
    LnlsRtmLampController(bars, addr, device_match_lamp_v2)
{
    channel_regs = std::make_unique<channel_registers_v2>();
}
LnlsRtmLampControllerV2::~LnlsRtmLampControllerV2() = default;

void LnlsRtmLampControllerV2::encode_config()
{
    static const std::unordered_map<std::string, int> mode_options({
        {"open-loop-dac", 0},
        {"open-loop-square", 1},
        {"closed-loop-pi_sp", 2},
        {"closed-loop-square", 3},
        {"closed-loop-external", 4},
    });

    int mode_option;
    try {
        mode_option = mode_options.at(mode);
    } catch (std::out_of_range &e) {
        throw std::runtime_error("mode must be one of " + list_of_keys(mode_options));
    }
    clear_and_insert(channel_regs->ctl, mode_option, WB_RTMLAMP_OHWR_REGS_CH_CTL_MODE_MASK, WB_RTMLAMP_OHWR_REGS_CH_CTL_MODE_SHIFT);
    insert_bit(channel_regs->ctl, amp_enable, WB_RTMLAMP_OHWR_REGS_CH_CTL_AMP_EN);

    bool closed_loop = mode.rfind("closed-loop", 0) == 0;

    if (closed_loop && !(pi_kp && pi_ti && pi_sp))
        throw std::logic_error("pi_kp, pi_ti and pi_sp must be set for closed-loop operation");

    if (*pi_kp > max_26bits || *pi_ti > max_26bits)
        throw std::logic_error("pi_kp and pi_ti must fit in a number under 26 bits");

    if (closed_loop) {
        clear_and_insert(channel_regs->pi_kp, *pi_kp, WB_RTMLAMP_OHWR_REGS_CH_PI_KP_DATA_MASK, WB_RTMLAMP_OHWR_REGS_CH_PI_KP_DATA_SHIFT);
        clear_and_insert(channel_regs->pi_ti, *pi_ti, WB_RTMLAMP_OHWR_REGS_CH_PI_TI_DATA_MASK, WB_RTMLAMP_OHWR_REGS_CH_PI_TI_DATA_SHIFT);
        clear_and_insert(channel_regs->pi_sp, (uint16_t)*pi_sp, WB_RTMLAMP_OHWR_REGS_CH_PI_SP_DATA_MASK, WB_RTMLAMP_OHWR_REGS_CH_PI_SP_DATA_SHIFT);
    }

    if ((mode == "open-loop-square" || mode == "closed-loop-square") && !(limit_a && limit_b && cnt))
        throw std::runtime_error("limits A and B and cnt must be specified for square wave operation");

    if (dac) {
        if (mode != "open-loop-dac")
            throw std::runtime_error("DAC can't receive values when not being used");
        clear_and_insert(channel_regs->dac, *dac, WB_RTMLAMP_OHWR_REGS_CH_DAC_DATA_MASK, WB_RTMLAMP_OHWR_REGS_CH_DAC_DATA_SHIFT);
    }

    if (limit_a) clear_and_insert(channel_regs->lim, (uint16_t)*limit_a, WB_RTMLAMP_OHWR_REGS_CH_LIM_A_MASK, WB_RTMLAMP_OHWR_REGS_CH_LIM_A_SHIFT);
    if (limit_b) clear_and_insert(channel_regs->lim, (uint16_t)*limit_b, WB_RTMLAMP_OHWR_REGS_CH_LIM_B_MASK, WB_RTMLAMP_OHWR_REGS_CH_LIM_B_SHIFT);

    if (cnt) {
        if (*cnt > max_22bits)
            throw std::logic_error("cnt must fit in a number under 22 bits");
        clear_and_insert(channel_regs->cnt, *cnt, WB_RTMLAMP_OHWR_REGS_CH_CNT_DATA_MASK, WB_RTMLAMP_OHWR_REGS_CH_CNT_DATA_SHIFT);
    }
}

void LnlsRtmLampControllerV2::write_params()
{
    encode_config();

    bar4_write_v(bars, addr + WB_RTMLAMP_OHWR_REGS_CH + channel * channel_distance, channel_regs.get(), channel_distance);
}
