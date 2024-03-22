#include "util.h"

#include "printer.h"
#include "modules/trigger_iface.h"

#include "hw/wb_trigger_iface_regs.h"

namespace trigger_iface {

namespace internal {
    static constexpr unsigned number_of_channels = 24;
}

namespace {
    constexpr unsigned TRIGGER_IFACE_DEVID = 0xbcbb78d2;
    struct sdb_device_info ref_devinfo = {
        .vendor_id = LNLS_VENDORID,
        .device_id = TRIGGER_IFACE_DEVID,
        .abi_ver_major = 1
    };
}

/* hw header didn't have a struct so we write it by hand */
struct trigger_iface_regs {
    struct {
        uint32_t ctl, cfg, count;
    } ch[internal::number_of_channels];
};

Core::Core(struct pcie_bars &bars):
    RegisterDecoder(bars, ref_devinfo, {
        PRINTER("DIR", "Trigger Direction", PrinterType::boolean, "Receiver Mode", "Transmitter Mode"),
        PRINTER("DIR_POL", "Trigger Direction Polarity", PrinterType::boolean, "Reversed backplane trigger direction", "Same backplane trigger direction"),
        PRINTER("RCV_LEN", "Receiver Pulse Length", PrinterType::value),
        PRINTER("TRANSM_LEN", "Transmitter Pulse Length", PrinterType::value),
        PRINTER("RCV_COUNT", "Receiver Counter", PrinterType::value),
        PRINTER("TRANSM_COUNT", "Transmitter Counter", PrinterType::value),
    }),
    CONSTRUCTOR_REGS(struct trigger_iface_regs)
{
    set_read_dest(regs);
}
Core::~Core() = default;

void Core::decode()
{
    number_of_channels = internal::number_of_channels;

    for (unsigned i = 0; i < *number_of_channels; i++) {
        uint32_t t = regs.ch[i].ctl;

        add_channel("DIR", i, t & WB_TRIG_IFACE_CH0_CTL_DIR);
        add_channel("DIR_POL", i, t & WB_TRIG_IFACE_CH0_CTL_DIR_POL);

        t = regs.ch[i].cfg;
        add_channel("RCV_LEN", i, extract_value<uint8_t>(t, WB_TRIG_IFACE_CH0_CFG_RCV_LEN_MASK));
        add_channel("TRANSM_LEN", i, extract_value<uint8_t>(t, WB_TRIG_IFACE_CH0_CFG_TRANSM_LEN_MASK));

        t = regs.ch[i].count;
        add_channel("RCV_COUNT", i, extract_value<uint16_t>(t, WB_TRIG_IFACE_CH0_COUNT_RCV_MASK));
        add_channel("TRANSM_COUNT", i, extract_value<uint16_t>(t, WB_TRIG_IFACE_CH0_COUNT_TRANSM_MASK));
    }
}

Controller::Controller(struct pcie_bars &bars):
    RegisterController(bars, ref_devinfo),
    CONSTRUCTOR_REGS(struct trigger_iface_regs),
    parameters(internal::number_of_channels)
{
    set_read_dest(regs);
}
Controller::~Controller() = default;

void Controller::encode_params()
{
    read();

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
    RegisterController::write_params();

    /* reset these strobe values */
    for (unsigned i = 0; i < internal::number_of_channels; i++) {
        parameters[i].rcv_count_rst = parameters[i].transm_count_rst = std::nullopt;
    }
}

} /* namespace trigger_iface */
