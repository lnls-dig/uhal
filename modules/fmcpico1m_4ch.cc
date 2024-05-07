#include "pcie.h"
#include "printer.h"
#include "util.h"
#include "modules/fmcpico1m_4ch.h"

namespace fmcpico1m_4ch {

#include "hw/wb_fmcpico1m_4ch_regs.h"

struct fmcpico1m_4ch {
    uint32_t fmc_status;
    uint32_t fmc_ctl;
    uint32_t rng_ctl;
    uint32_t data[4];
};

static_assert(WB_FMCPICO1M_4CH_CSR_REG_DATA2 == offsetof(fmcpico1m_4ch, data[2]));

namespace {
    constexpr unsigned NUM_CHAN = 4;

    constexpr unsigned FMCPICO1M_4CH_DEVID = 0x669f7e38;
    struct sdb_device_info ref_devinfo = {
        .vendor_id = LNLS_VENDORID,
        .device_id = FMCPICO1M_4CH_DEVID,
        .abi_ver_major = 1
    };
}

const std::vector<std::string> range_list = {
    "+/-100uA",
    "+/-1mA",
};

Core::Core(struct pcie_bars &bars):
    RegisterDecoder(bars, ref_devinfo, {
        PRINTER("PRSNT", "FMC Present", PrinterType::boolean, "no FMC card", "card present"),
        PRINTER("RANGE", "Input Range Control for ADC", PrinterType::boolean, range_list[1].c_str(), range_list[0].c_str()),
        PRINTER("DATA", "ADC Data from channel", PrinterType::value_hex),
    }),
    CONSTRUCTOR_REGS(struct fmcpico1m_4ch)
{
    set_read_dest(regs);

    number_of_channels = NUM_CHAN;
}
Core::~Core() = default;

void Core::decode()
{
    add_general("PRSNT", get_bit(regs.fmc_status, WB_FMCPICO1M_4CH_CSR_FMC_STATUS_PRSNT));

    add_channel("RANGE", 0, rf_get_bit(regs.rng_ctl, WB_FMCPICO1M_4CH_CSR_RNG_CTL_R0));
    add_channel("RANGE", 1, rf_get_bit(regs.rng_ctl, WB_FMCPICO1M_4CH_CSR_RNG_CTL_R1));
    add_channel("RANGE", 2, rf_get_bit(regs.rng_ctl, WB_FMCPICO1M_4CH_CSR_RNG_CTL_R2));
    add_channel("RANGE", 3, rf_get_bit(regs.rng_ctl, WB_FMCPICO1M_4CH_CSR_RNG_CTL_R3));

    for (unsigned i = 0; i < NUM_CHAN; i++) {
        add_channel("DATA", i, regs.data[i]);
    }
}

Controller::Controller(struct pcie_bars &bars):
    RegisterDecoderController(bars, ref_devinfo, &dec),
    CONSTRUCTOR_REGS(struct fmcpico1m_4ch),
    dec(bars)
{
    set_read_dest(regs);
}
Controller::~Controller() = default;

void Controller::write_params()
{
  /* we only support this register, for now. And writing into
   * WB_FMCPICO1M_4CH_CSR_REG_DATA3 (which RegisterController::write_params
   * would do) triggers a gateware bug */
  bar4_write(&bars, addr + WB_FMCPICO1M_4CH_CSR_REG_RNG_CTL, regs.rng_ctl);
}

} /* namespace fmcpico1m_4ch */
