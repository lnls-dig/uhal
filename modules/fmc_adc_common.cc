#include "printer.h"
#include "util.h"
#include "modules/fmc_adc_common.h"

namespace fmc_adc_common {

#include "hw/wb_fmc_adc_common_regs.h"

struct fmc_adc_common {
    uint32_t fmc_status;
    uint32_t trigger;
    uint32_t monitor;
};

namespace {
    constexpr unsigned FMC_ADC_COMMON_DEVID = 0x2403f569;
    struct sdb_device_info ref_devinfo = {
        .vendor_id = LNLS_VENDORID,
        .device_id = FMC_ADC_COMMON_DEVID,
        .abi_ver_major = 1
    };
}

Core::Core(struct pcie_bars &bars):
    RegisterDecoder(bars, ref_devinfo, {
        PRINTER("MMCM_LOCKED", "MMCM locked status", PrinterType::boolean),
        PRINTER("PWR_GOOD", "FMC power good status", PrinterType::boolean),
        PRINTER("PRST", "FMC board present status", PrinterType::boolean),
        PRINTER("DIR", "Trigger direction", "output", "input"),
        PRINTER("TERM", "Trigger termination with 50 ohms", PrinterType::enable),
        PRINTER("TRIG_VAL", "Trigger value when used in output mode", PrinterType::boolean),
        PRINTER("TEST_DATA_EN", "Enable test data", PrinterType::enable),
        PRINTER("LED1", "LED 1 (blue, configuration in progress)", PrinterType::enable),
        PRINTER("LED2", "LED 2 (red, data acquisition in progress)", PrinterType::enable),
        PRINTER("LED3", "LED 3 (green, trigger status)", PrinterType::enable),
        PRINTER("MMCM_RST", "MMCM Reset", PrinterType::enable),
    }),
    CONSTRUCTOR_REGS(struct fmc_adc_common)
{
    set_read_dest(regs);
}
Core::~Core() = default;

void Core::decode()
{
    add_general("MMCM_LOCKED", get_bit(regs.fmc_status, WB_FMC_ADC_COMMON_CSR_FMC_STATUS_MMCM_LOCKED));
    add_general("PWR_GOOD", get_bit(regs.fmc_status, WB_FMC_ADC_COMMON_CSR_FMC_STATUS_PWR_GOOD));
    add_general("PRST", get_bit(regs.fmc_status, WB_FMC_ADC_COMMON_CSR_FMC_STATUS_PRST));

    add_general("DIR", rf_get_bit(regs.trigger, WB_FMC_ADC_COMMON_CSR_TRIGGER_DIR));
    add_general("TERM", rf_get_bit(regs.trigger, WB_FMC_ADC_COMMON_CSR_TRIGGER_TERM));
    add_general("TRIG_VAL", rf_get_bit(regs.trigger, WB_FMC_ADC_COMMON_CSR_TRIGGER_TRIG_VAL));

    add_general("TEST_DATA_EN", rf_get_bit(regs.monitor, WB_FMC_ADC_COMMON_CSR_MONITOR_TEST_DATA_EN));
    add_general("LED1", rf_get_bit(regs.monitor, WB_FMC_ADC_COMMON_CSR_MONITOR_LED1));
    add_general("LED2", rf_get_bit(regs.monitor, WB_FMC_ADC_COMMON_CSR_MONITOR_LED2));
    add_general("LED3", rf_get_bit(regs.monitor, WB_FMC_ADC_COMMON_CSR_MONITOR_LED3));
    add_general("MMCM_RST", rf_get_bit(regs.monitor, WB_FMC_ADC_COMMON_CSR_MONITOR_MMCM_RST));
}

Controller::Controller(struct pcie_bars &bars):
    RegisterDecoderController(bars, ref_devinfo, &dec),
    CONSTRUCTOR_REGS(struct fmc_adc_common),
    dec(bars)
{
    set_read_dest(regs);
}
Controller::~Controller() = default;

} /* namespace fmc_adc_common */
