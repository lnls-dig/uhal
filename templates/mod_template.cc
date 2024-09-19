#include "printer.h"
#include "util.h"
#include "mod_template.h"

namespace mod_template {

struct mod_template {
};

namespace {
    constexpr unsigned MOD_TEMPLATE_DEVID = 0xdeadbeef;
    struct sdb_device_info ref_devinfo = {
        .vendor_id = LNLS_VENDORID,
        .device_id = MOD_TEMPLATE_DEVID,
        .abi_ver_major = 0
    };
}

Core::Core(struct pcie_bars &bars):
    RegisterDecoder(bars, ref_devinfo, {
    }),
    CONSTRUCTOR_REGS(struct mod_template)
{
    set_read_dest(regs);
}
Core::~Core() = default;

void Core::decode()
{
}

Controller::Controller(struct pcie_bars &bars):
    RegisterDecoderController(bars, ref_devinfo, &dec),
    CONSTRUCTOR_REGS(struct mod_template),
    dec(bars)
{
    set_read_dest(regs);
}
Controller::~Controller() = default;

} /* namespace mod_template */
