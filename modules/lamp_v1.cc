/*
 * Copyright (C) 2022 CNPEM (cnpem.br)
 * Author: Érico Nogueira <erico.rolim@lnls.br>
 *
 * Released according to the GNU GPL, version 3 or any later version.
 */

#include <cstring>
#include <stdexcept>
#include <unordered_map>

#include "printer.h"
#include "util.h"
#include "lamp.h"

#include "hw/wb_rtmlamp_ohwr_regs_v1.h"

#define MAX_NUM_CHAN 12
#define REGISTERS_PER_CHAN 6
static const unsigned max_26bits = 0x3ffffff;
static const unsigned channel_distance = 4 * REGISTERS_PER_CHAN;

LnlsRtmLampCoreV1::LnlsRtmLampCoreV1()
{
    read_size = sizeof *regs;
    regs = std::make_unique<struct rtmlamp_ohwr_regs>();
    read_dest = regs.get();

}
LnlsRtmLampCoreV1::~LnlsRtmLampCoreV1() = default;

void LnlsRtmLampCoreV1::print(FILE *f, bool verbose)
{
    static const std::unordered_map<const char *, Printer> printers({
      I("DAC_DATA_FROM_WB", "Use data from WB for DACs", PrinterType::boolean, "DAC data from RTM module input ports", "DAC data from Wishbone"),
      /* per channel info */
      I("AMP_IFLAG_L", "Amplifier Left Current Limit Flag", PrinterType::boolean, "current under limit", "current over limit"),
      I("AMP_TFLAG_L", "Amplifier Left Thermal Limit Flag", PrinterType::boolean, "temperature under limit", "temperature over limit"),
      I("AMP_IFLAG_R", "Amplifier Right Current Limit Flag", PrinterType::boolean, "current under limit", "current over limit"),
      I("AMP_TFLAG_R", "Amplifier Right Thermal Limit Flag", PrinterType::boolean, "temperature under limit", "temperature over limit"),
      I("AMP_EN", "Amplifier Enable", PrinterType::boolean),
      /* TODO: add test mode when it becomes a single value */
      I("PI_ENABLE", "PI Enable", PrinterType::boolean),
      I("PI_KP", "PI KP Coefficient", PrinterType::value),
      I("PI_TI", "PI TI Coefficient", PrinterType::value),
      I("PI_SP", "PI Setpoint", PrinterType::value),
      I("DAC", "DAC Data For Channel", PrinterType::value),
      /* PI control */
      I("PI_OL_DAC_CNT_MAX", "PI open-loop DAC test mode period, in clock ticks", PrinterType::value),
      I("PI_SP_LIM_INF", "PI open-loop DAC test mode inferior limit. The upper limit is the setpoint itself", PrinterType::value),
    });

    bool v = verbose;
    unsigned indent = 0;
    uint32_t t;

    auto print_reg = [f, v, &indent](const char *reg, unsigned offset) {
        print_reg_impl(f, v, indent, reg, offset);
    };
    auto print = [f, v, &indent](const char *name, uint32_t value) {
        printers.at(name).print(f, v, indent, value);
    };

    print_reg("control register", RTMLAMP_OHWR_REGS_CTL);
    t = regs->ctl;
    print("DAC_DATA_FROM_WB", t & RTMLAMP_OHWR_REGS_CTL_DAC_DATA_FROM_WB);

    uint32_t p[MAX_NUM_CHAN * REGISTERS_PER_CHAN];
    memcpy(p, &regs->ch_0_sta, sizeof p);
    for (unsigned i = 0; i < MAX_NUM_CHAN; i++) {
        fprintf(f, "channel %u (starting at %02X):\n",
            i, (unsigned)RTMLAMP_OHWR_REGS_CH_0_STA + 4*i*REGISTERS_PER_CHAN);
        indent = 4;

        struct channel_registers channel_regs;
        memcpy(&channel_regs, p + i*REGISTERS_PER_CHAN, sizeof channel_regs);
        t = channel_regs.sta;
        print("AMP_IFLAG_L", t & RTMLAMP_OHWR_REGS_CH_0_STA_AMP_IFLAG_L);
        print("AMP_TFLAG_L", t & RTMLAMP_OHWR_REGS_CH_0_STA_AMP_TFLAG_L);
        print("AMP_IFLAG_R", t & RTMLAMP_OHWR_REGS_CH_0_STA_AMP_IFLAG_R);
        print("AMP_TFLAG_R", t & RTMLAMP_OHWR_REGS_CH_0_STA_AMP_TFLAG_R);

        t = channel_regs.ctl;
        print("AMP_EN", t & RTMLAMP_OHWR_REGS_CH_0_CTL_AMP_EN);
        print("PI_ENABLE", t & RTMLAMP_OHWR_REGS_CH_0_CTL_PI_ENABLE);

        print("PI_KP", (channel_regs.pi_kp & RTMLAMP_OHWR_REGS_CH_0_PI_KP_DATA_MASK) >> RTMLAMP_OHWR_REGS_CH_0_PI_KP_DATA_SHIFT);
        print("PI_TI", (channel_regs.pi_ti & RTMLAMP_OHWR_REGS_CH_0_PI_TI_DATA_MASK) >> RTMLAMP_OHWR_REGS_CH_0_PI_TI_DATA_SHIFT);
        print("PI_SP", (channel_regs.pi_sp & RTMLAMP_OHWR_REGS_CH_0_PI_SP_DATA_MASK) >> RTMLAMP_OHWR_REGS_CH_0_PI_SP_DATA_SHIFT);
        print("DAC", (channel_regs.dac & RTMLAMP_OHWR_REGS_CH_0_DAC_DATA_MASK) >> RTMLAMP_OHWR_REGS_CH_0_DAC_DATA_SHIFT);
    }
}

void LnlsRtmLampController::encode_config()
{
    insert_bit(ctl, dac_data, RTMLAMP_OHWR_REGS_CTL_DAC_DATA_FROM_WB);

    insert_bit(channel_regs.ctl, amp_enable, RTMLAMP_OHWR_REGS_CH_0_CTL_AMP_EN);

    static const std::unordered_map<std::string, int> mode_options({
        {"none", -1},
        {"triangular", 0},
        {"square", 1},
        {"setpoint-square", 2},
    });

    int mode_option = mode_options.at(mode);
    if (mode_option >= 0) {
        /* TODO: figure out when to enable PI controller */
        //insert_bit(channel_regs.ctl, 1, RTMLAMP_OHWR_REGS_CH_0_CTL_PI_ENABLE);
        insert_bit(channel_regs.ctl, 1, RTMLAMP_OHWR_REGS_CH_0_CTL_PI_OL_TRIANG_ENABLE << mode_option);
    }

    if (pi_kp > max_26bits || pi_ti > max_26bits)
        throw std::logic_error("pi_kp and pi_ti must fit in a number under 26 bits");

    clear_and_insert(channel_regs.pi_kp, pi_kp, RTMLAMP_OHWR_REGS_CH_0_PI_KP_DATA_MASK, RTMLAMP_OHWR_REGS_CH_0_PI_KP_DATA_SHIFT);
    clear_and_insert(channel_regs.pi_ti, pi_ti, RTMLAMP_OHWR_REGS_CH_0_PI_TI_DATA_MASK, RTMLAMP_OHWR_REGS_CH_0_PI_TI_DATA_SHIFT);
    clear_and_insert(channel_regs.pi_sp, (uint16_t)pi_sp, RTMLAMP_OHWR_REGS_CH_0_PI_SP_DATA_MASK, RTMLAMP_OHWR_REGS_CH_0_PI_SP_DATA_SHIFT);

    if (dac_data) {
        clear_and_insert(
            channel_regs.dac,
            /* we need the WR bit so the values are actually sent to the DAC */
            ((uint16_t)dac) | RTMLAMP_OHWR_REGS_CH_0_DAC_WR,
            RTMLAMP_OHWR_REGS_CH_0_DAC_DATA_MASK | RTMLAMP_OHWR_REGS_CH_0_DAC_WR,
            RTMLAMP_OHWR_REGS_CH_0_DAC_DATA_SHIFT
        );
    }
}

void LnlsRtmLampController::write_params()
{
    encode_config();

    bar4_write(bars, addr + RTMLAMP_OHWR_REGS_CTL, ctl);
    bar4_write_v(bars, addr + RTMLAMP_OHWR_REGS_CH_0_STA + channel * channel_distance, &channel_regs, sizeof channel_regs);
}
