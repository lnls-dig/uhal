/*
 * Copyright (C) 2023 CNPEM (cnpem.br)
 * Author: Érico Nogueira <erico.rolim@lnls.br>
 *
 * Released according to the GNU GPL, version 3 or any later version.
 */

#include "util.h"

#include "trigger_iface.h"

#include "hw/wb_trigger_iface_regs.h"

namespace trigger_iface {

namespace internal {
    static constexpr unsigned number_of_channels = 24;
}

/* hw header didn't have a struct so we write it by hand */
struct trigger_iface_regs {
    struct {
        uint32_t ctl, cfg, count;
    } ch[internal::number_of_channels];
};

Core::Core(struct pcie_bars &bars):
    RegisterDecoder(bars, {
        PRINTER("DIR", "Trigger Direction", PrinterType::boolean, "Receiver Mode", "Transmitter Mode"),
        PRINTER("DIR_POL", "Trigger Direction Polarity", PrinterType::boolean, "Reversed backplane trigger direction", "Same backplane trigger direction"),
        PRINTER("RCV_LEN", "Receiver Pulse Length", PrinterType::value),
        PRINTER("TRANSM_LEN", "Transmitter Pulse Length", PrinterType::value),
        PRINTER("RCV_COUNT", "Receiver Counter", PrinterType::value),
        PRINTER("TRANSM_COUNT", "Transmitter Counter", PrinterType::value),
    }),
    regs_storage(new struct trigger_iface_regs),
    regs(*regs_storage)
{
    read_size = sizeof regs;
    read_dest = &regs;

    device_match = device_match_trigger_iface;
}
Core::~Core() = default;

void Core::decode()
{
    number_of_channels = internal::number_of_channels;

    for (unsigned i = 0; i < *number_of_channels; i++) {
        auto add_channel = [this, i](const char *name, auto value) {
            channel_data[name].resize(*number_of_channels);
            add_channel_impl(name, i, value);
        };

        uint32_t t = regs.ch[i].ctl;

        add_channel("DIR", t & WB_TRIG_IFACE_CH0_CTL_DIR);
        add_channel("DIR_POL", t & WB_TRIG_IFACE_CH0_CTL_DIR_POL);

        t = regs.ch[i].cfg;
        add_channel("RCV_LEN", extract_value<uint8_t>(t, WB_TRIG_IFACE_CH0_CFG_RCV_LEN_MASK));
        add_channel("TRANSM_LEN", extract_value<uint8_t>(t, WB_TRIG_IFACE_CH0_CFG_TRANSM_LEN_MASK));

        t = regs.ch[i].count;
        add_channel("RCV_COUNT", extract_value<uint16_t>(t, WB_TRIG_IFACE_CH0_COUNT_RCV_MASK));
        add_channel("TRANSM_COUNT", extract_value<uint16_t>(t, WB_TRIG_IFACE_CH0_COUNT_TRANSM_MASK));

        data_order_done = true;
    }
}

Controller::Controller(struct pcie_bars &bars):
    RegisterController(bars),
    regs_storage(new struct trigger_iface_regs),
    regs(*regs_storage),
    parameters(internal::number_of_channels)
{
}
Controller::~Controller() = default;

void Controller::get_internal_values()
{
    bar4_read_v(&bars, addr, &regs, sizeof regs);
}

void Controller::encode_config()
{
    get_internal_values();

    for (unsigned i = 0; i < internal::number_of_channels; i++) {
        auto &p = parameters[i];
        auto &r = regs.ch[i];

        if (p.direction) insert_bit(r.ctl, *p.direction, WB_TRIG_IFACE_CH0_CTL_DIR);
        if (p.direction_polarity) insert_bit(r.ctl, *p.direction_polarity, WB_TRIG_IFACE_CH0_CTL_DIR_POL);
        if (p.rcv_count_rst) insert_bit(r.ctl, *p.rcv_count_rst, WB_TRIG_IFACE_CH0_CTL_RCV_COUNT_RST);
        if (p.transm_count_rst) insert_bit(r.ctl, *p.transm_count_rst, WB_TRIG_IFACE_CH0_CTL_TRANSM_COUNT_RST);

        if (p.rcv_len) clear_and_insert(r.cfg, *p.rcv_len, WB_TRIG_IFACE_CH0_CFG_RCV_LEN_MASK);
        if (p.transm_len) clear_and_insert(r.cfg, *p.transm_len, WB_TRIG_IFACE_CH0_CFG_TRANSM_LEN_MASK);
    }
}

void Controller::write_params()
{
    encode_config();

    bar4_write_v(&bars, addr, &regs, sizeof regs);

    /* reset these strobe values */
    for (unsigned i = 0; i < internal::number_of_channels; i++) {
        parameters[i].rcv_count_rst = parameters[i].transm_count_rst = std::nullopt;
    }
}

} /* namespace trigger_iface */
