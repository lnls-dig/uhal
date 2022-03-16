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

static const unsigned max_26bits = 0x3ffffff;
static const unsigned channel_distance = 4 * 6; /* 6 registers */

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
