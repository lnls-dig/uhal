#include "printer.h"
#include "util.h"
#include "modules/fmc250m_4ch.h"

namespace fmc250m_4ch {

#include "hw/wb_fmc250m_4ch_regs.h"

struct fmc250m_4ch {
    uint32_t adc_sta;
    uint32_t adc_ctl;
    /* there are more members after this but we don't use any of them */
};

namespace {
    constexpr unsigned FMC250M_4CH_DEVID = 0x68e3b1af;
    struct sdb_device_info ref_devinfo = {
        .vendor_id = LNLS_VENDORID,
        .device_id = FMC250M_4CH_DEVID,
        .abi_ver_major = 1
    };
}

Core::Core(struct pcie_bars &bars):
    RegisterDecoder(bars, ref_devinfo, {
        PRINTER("RST_ADCS", "Reset ADCs", PrinterType::enable),
        PRINTER("RST_DIV_ADCS", "Reset Div ADCs", PrinterType::enable),
        PRINTER("SLEEP_ADCS", "Sleep ADCs", PrinterType::enable),
    }),
    CONSTRUCTOR_REGS(struct fmc250m_4ch)
{
    set_read_dest(regs);
}
Core::~Core() = default;

void Core::decode()
{
    add_general("RST_ADCS", rf_get_bit(regs.adc_ctl, WB_FMC_250M_4CH_CSR_ADC_CTL_RST_ADCS));
    add_general("RST_DIV_ADCS", rf_get_bit(regs.adc_ctl, WB_FMC_250M_4CH_CSR_ADC_CTL_RST_DIV_ADCS));
    add_general("SLEEP_ADCS", rf_get_bit(regs.adc_ctl, WB_FMC_250M_4CH_CSR_ADC_CTL_SLEEP_ADCS));
}

Controller::Controller(struct pcie_bars &bars):
    RegisterDecoderController(bars, ref_devinfo, &dec),
    CONSTRUCTOR_REGS(struct fmc250m_4ch),
    dec(bars)
{
    set_read_dest(regs);
}
Controller::~Controller() = default;

void Controller::unset_commands()
{
    regs.adc_ctl = 0;
}

} /* namespace fmc250m_4ch */
