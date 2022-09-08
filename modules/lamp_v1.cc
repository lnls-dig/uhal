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

namespace lamp {

#include "hw/wb_rtmlamp_ohwr_regs_v1.h"

static constexpr unsigned MAX_NUM_CHAN = 12;
static constexpr unsigned REGISTERS_PER_CHAN = 6;
static constexpr unsigned CHANNEL_DISTANCE = 4 * REGISTERS_PER_CHAN;
static constexpr unsigned MAX_26BITS = 0x3ffffff;

CoreV1::CoreV1(struct pcie_bars &bars):
    RegisterDecoder(bars, {
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
    })
{
    read_size = sizeof *regs;
    regs = std::make_unique<struct rtmlamp_ohwr_regs>();
    read_dest = regs.get();

    device_match = device_match_v1;
}
CoreV1::~CoreV1() = default;

void CoreV1::decode()
{
    uint32_t t;

    number_of_channels = MAX_NUM_CHAN;

    auto add_general = [this](const char *name, auto value) {
        general_data[name] = value;
        if (!data_order_done)
            general_data_order.push_back(name);
    };

    t = regs->ctl;
    add_general("DAC_DATA_FROM_WB", t & RTMLAMP_OHWR_REGS_CTL_DAC_DATA_FROM_WB);

    unsigned i;
    auto add_channel = [this, &i](const char *name, auto value) {
        channel_data[name].resize(*number_of_channels);
        channel_data[name][i] = value;
        if (!data_order_done)
            channel_data_order.push_back(name);
    };

    uint32_t p[MAX_NUM_CHAN * REGISTERS_PER_CHAN];
    memcpy(p, &regs->ch_0_sta, sizeof p);
    for (i = 0; i < MAX_NUM_CHAN; i++) {
        struct channel_registers channel_regs;
        memcpy(&channel_regs, p + i*REGISTERS_PER_CHAN, sizeof channel_regs);
        t = channel_regs.sta;
        add_channel("AMP_IFLAG_L", t & RTMLAMP_OHWR_REGS_CH_0_STA_AMP_IFLAG_L);
        add_channel("AMP_TFLAG_L", t & RTMLAMP_OHWR_REGS_CH_0_STA_AMP_TFLAG_L);
        add_channel("AMP_IFLAG_R", t & RTMLAMP_OHWR_REGS_CH_0_STA_AMP_IFLAG_R);
        add_channel("AMP_TFLAG_R", t & RTMLAMP_OHWR_REGS_CH_0_STA_AMP_TFLAG_R);

        t = channel_regs.ctl;
        add_channel("AMP_EN", t & RTMLAMP_OHWR_REGS_CH_0_CTL_AMP_EN);
        add_channel("PI_ENABLE", t & RTMLAMP_OHWR_REGS_CH_0_CTL_PI_ENABLE);

        add_channel("PI_KP", (channel_regs.pi_kp & RTMLAMP_OHWR_REGS_CH_0_PI_KP_DATA_MASK) >> RTMLAMP_OHWR_REGS_CH_0_PI_KP_DATA_SHIFT);
        add_channel("PI_TI", (channel_regs.pi_ti & RTMLAMP_OHWR_REGS_CH_0_PI_TI_DATA_MASK) >> RTMLAMP_OHWR_REGS_CH_0_PI_TI_DATA_SHIFT);
        add_channel("PI_SP", (channel_regs.pi_sp & RTMLAMP_OHWR_REGS_CH_0_PI_SP_DATA_MASK) >> RTMLAMP_OHWR_REGS_CH_0_PI_SP_DATA_SHIFT);
        add_channel("DAC", (channel_regs.dac & RTMLAMP_OHWR_REGS_CH_0_DAC_DATA_MASK) >> RTMLAMP_OHWR_REGS_CH_0_DAC_DATA_SHIFT);

        data_order_done = true;
    }
}

ControllerV1::ControllerV1(struct pcie_bars &bars):
    Controller(bars, device_match_v1)
{
}

void ControllerV1::encode_config()
{
    if (limit_a || limit_b)
        throw std::logic_error("LAMP v1 doesn't have limits");
    if (cnt)
        throw std::logic_error("LAMP v1 doesn't have cnt");

    insert_bit(ctl, static_cast<bool>(dac), RTMLAMP_OHWR_REGS_CTL_DAC_DATA_FROM_WB);

    insert_bit(channel_regs.ctl, amp_enable, RTMLAMP_OHWR_REGS_CH_0_CTL_AMP_EN);

    static const std::unordered_map<std::string, int> mode_options({
        {"none", -1},
        {"triangular", 0},
        {"square", 1},
        {"setpoint-square", 2},
    });

    int mode_option;
    try {
        mode_option = mode_options.at(mode);
    } catch (std::out_of_range &e) {
        throw std::runtime_error("mode must be one of " + list_of_keys(mode_options));
    }
    if (mode_option >= 0) {
        /* TODO: figure out when to enable PI controller */
        //insert_bit(channel_regs.ctl, 1, RTMLAMP_OHWR_REGS_CH_0_CTL_PI_ENABLE);
        insert_bit(channel_regs.ctl, 1, RTMLAMP_OHWR_REGS_CH_0_CTL_PI_OL_TRIANG_ENABLE << mode_option);
    }

    if (pi_kp && pi_ti && pi_sp) {
        if (*pi_kp > MAX_26BITS || *pi_ti > MAX_26BITS)
            throw std::logic_error("pi_kp and pi_ti must fit in a number under 26 bits");

        clear_and_insert(channel_regs.pi_kp, *pi_kp, RTMLAMP_OHWR_REGS_CH_0_PI_KP_DATA_MASK);
        clear_and_insert(channel_regs.pi_ti, *pi_ti, RTMLAMP_OHWR_REGS_CH_0_PI_TI_DATA_MASK);
        clear_and_insert(channel_regs.pi_sp, (uint16_t)*pi_sp, RTMLAMP_OHWR_REGS_CH_0_PI_SP_DATA_MASK);
    }

    if (dac) {
        clear_and_insert(channel_regs.dac, (uint16_t)*dac, RTMLAMP_OHWR_REGS_CH_0_DAC_DATA_MASK);
        /* we need the WR bit so the values are actually sent to the DAC */
        insert_bit(channel_regs.dac, 1, RTMLAMP_OHWR_REGS_CH_0_DAC_WR);
    }
}

void ControllerV1::write_params()
{
    encode_config();

    bar4_write(&bars, addr + RTMLAMP_OHWR_REGS_CTL, ctl);
    bar4_write_v(&bars, addr + RTMLAMP_OHWR_REGS_CH_0_STA + channel * CHANNEL_DISTANCE, &channel_regs, sizeof channel_regs);
}

} /* namespace lamp */
