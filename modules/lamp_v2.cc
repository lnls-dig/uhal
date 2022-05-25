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
#include "lamp.h"

#include "hw/wb_rtmlamp_ohwr_regs_v2.h"

static const unsigned max_26bits = 0x3ffffff;
static const unsigned registers_per_chan = 8;

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
          }
      ),
      I("PI_KP", "PI KP Coefficient", PrinterType::value),
      I("PI_TI", "PI TI Coefficient", PrinterType::value),
      I("PI_SP", "PI Setpoint", PrinterType::value),
      I("DAC", "DAC Data For Channel", PrinterType::value),
      I("Limit A", "Signed limit 'a')", PrinterType::value),
      I("Limit B", "Signed limit 'b')", PrinterType::value),
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
        i++;

        fprintf(f, "channel %u:\n", i);
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
        print("PI_SP", (channel_regs.pi_sp & WB_RTMLAMP_OHWR_REGS_CH_PI_SP_DATA_MASK) >> WB_RTMLAMP_OHWR_REGS_CH_PI_SP_DATA_SHIFT);
        print("DAC", (channel_regs.dac & WB_RTMLAMP_OHWR_REGS_CH_DAC_DATA_MASK) >> WB_RTMLAMP_OHWR_REGS_CH_DAC_DATA_SHIFT);

        t = channel_regs.lim;
        print("Limit A", (t & WB_RTMLAMP_OHWR_REGS_CH_LIM_A_MASK) >> WB_RTMLAMP_OHWR_REGS_CH_LIM_A_SHIFT);
        print("Limit B", (t & WB_RTMLAMP_OHWR_REGS_CH_LIM_B_MASK) >> WB_RTMLAMP_OHWR_REGS_CH_LIM_B_SHIFT);

        print("CNT", (channel_regs.cnt & WB_RTMLAMP_OHWR_REGS_CH_CNT_DATA_MASK) >> WB_RTMLAMP_OHWR_REGS_CH_CNT_DATA_SHIFT);
    }
}
