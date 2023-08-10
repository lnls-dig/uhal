/*
 * Copyright (C) 2023 CNPEM (cnpem.br)
 * Author: Érico Nogueira <erico.rolim@lnls.br>
 *
 * Released according to the GNU GPL, version 3 or any later version.
 */

#include "util.h"

#include "pcie.h"
#include "printer.h"
#include "modules/trigger_mux.h"

#include "hw/wb_trigger_mux_regs.h"

namespace trigger_mux {

namespace internal {
    static constexpr unsigned number_of_channels = 24;
}

namespace {
    struct sdb_device_info ref_devinfo = {
        .vendor_id = LNLS_VENDORID,
        .device_id = TRIGGER_MUX_DEVID,
        .abi_ver_major = 1
    };
}

/* hw header didn't have a struct so we write it by hand */
struct trigger_mux_regs {
    struct {
        uint32_t ctl, dummy;
    } ch[internal::number_of_channels];
};

Core::Core(struct pcie_bars &bars):
    RegisterDecoder(bars, ref_devinfo, {
        PRINTER("RCV_SRC", "Receiver Source", PrinterType::boolean, "Internal Signals", "Triggers"),
        PRINTER("RCV_IN_SEL", "Select Receiver Input", PrinterType::value),
        PRINTER("TRANSM_SRC", "Transmitter Source", PrinterType::boolean, "Internal Signals", "Triggers"),
        PRINTER("TRANSM_OUT_SEL", "Select Transmitter Output", PrinterType::value),
    }),
    regs_storage(new struct trigger_mux_regs),
    regs(*regs_storage)
{
    read_size = sizeof regs;
    read_dest = &regs;
}
Core::~Core() = default;

void Core::decode()
{
    number_of_channels = internal::number_of_channels;

    for (unsigned i = 0; i < *number_of_channels; i++) {
        uint32_t t = regs.ch[i].ctl;

        add_channel("RCV_SRC", i, t & WB_TRIG_MUX_CH0_CTL_RCV_SRC);
        add_channel("RCV_IN_SEL", i, extract_value<uint32_t>(t, WB_TRIG_MUX_CH0_CTL_RCV_IN_SEL_MASK));
        add_channel("TRANSM_SRC", i, t & WB_TRIG_MUX_CH0_CTL_TRANSM_SRC);
        add_channel("TRANSM_OUT_SEL", i, extract_value<uint32_t>(t, WB_TRIG_MUX_CH0_CTL_TRANSM_OUT_SEL_MASK));

        data_order_done = true;
    }
}

Controller::Controller(struct pcie_bars &bars):
    RegisterController(bars, ref_devinfo),
    regs_storage(new struct trigger_mux_regs),
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
        auto insert_val = [this, i](auto value, uint32_t mask) {
            if (value) clear_and_insert(regs.ch[i].ctl, *value, mask);
        };
        auto insert_bit_val = [this, i](auto value, uint32_t mask) {
            if (value) insert_bit(regs.ch[i].ctl, *value, mask);
        };

        insert_bit_val(parameters[i].rcv_src, WB_TRIG_MUX_CH0_CTL_RCV_SRC);
        insert_val(parameters[i].rcv_in_sel, WB_TRIG_MUX_CH0_CTL_RCV_IN_SEL_MASK);
        insert_bit_val(parameters[i].transm_src, WB_TRIG_MUX_CH0_CTL_TRANSM_SRC);
        insert_val(parameters[i].transm_out_sel, WB_TRIG_MUX_CH0_CTL_TRANSM_OUT_SEL_MASK);
    }
}

void Controller::write_params()
{
    encode_config();

    bar4_write_v(&bars, addr, &regs, sizeof regs);
}

} /* namespace trigger_mux */
