#include "printer.h"
#include "util.h"
#include "modules/fmc_active_clk.h"

namespace fmc_active_clk {

#include "hw/wb_fmc_active_clk_regs.h"

struct fmc_active_clk {
    uint32_t clk_distrib;
    /* there is a dummy member after clk_distrib, but we can simply omit it,
     * since it doesn't need to be read from or written to */
};

namespace {
    constexpr unsigned FMC_ACTIVE_CLK_DEVID = 0x88c67d9c;
    struct sdb_device_info ref_devinfo = {
        .vendor_id = LNLS_VENDORID,
        .device_id = FMC_ACTIVE_CLK_DEVID,
        .abi_ver_major = 1
    };
}

Core::Core(struct pcie_bars &bars):
    RegisterDecoder(bars, ref_devinfo, {
        PRINTER("SI571_OE", "Si571 Output Enable", PrinterType::enable), /* enable is valid because OE is Active High for this chip */
        PRINTER("PLL_FUNCTION", "AD9510 PLL Function", PrinterType::value),
        PRINTER("PLL_STATUS", "AD9510 PLL Status", "not locked", "locked"),
        PRINTER("CLK_SEL", "Reference Clock Selection", "clock from external source (MMCX J4)", "clock from FPGA (FMC_CLK line)"),
    }),
    CONSTRUCTOR_REGS(struct fmc_active_clk)
{
    set_read_dest(regs);
}
Core::~Core() = default;

void Core::decode()
{
    add_general("SI571_OE", rf_get_bit(regs.clk_distrib, WB_FMC_ACTIVE_CLK_CSR_CLK_DISTRIB_SI571_OE));
    add_general("PLL_FUNCTION", rf_get_bit(regs.clk_distrib, WB_FMC_ACTIVE_CLK_CSR_CLK_DISTRIB_PLL_FUNCTION));
    add_general("PLL_STATUS", get_bit(regs.clk_distrib, WB_FMC_ACTIVE_CLK_CSR_CLK_DISTRIB_PLL_STATUS));
    add_general("CLK_SEL", rf_get_bit(regs.clk_distrib, WB_FMC_ACTIVE_CLK_CSR_CLK_DISTRIB_CLK_SEL));
}

Controller::Controller(struct pcie_bars &bars):
    RegisterDecoderController(bars, ref_devinfo, &dec),
    CONSTRUCTOR_REGS(struct fmc_active_clk),
    dec(bars)
{
    set_read_dest(regs);
}
Controller::~Controller() = default;

} /* namespace fmc_active_clk */
