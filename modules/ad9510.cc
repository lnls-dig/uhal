#include "printer.h"
#include "util.h"
#include "modules/ad9510.h"

namespace ad9510 {

Controller::Controller(struct pcie_bars &bars):
    spi_regs(bars),
    fac_regs(bars)
{
}

bool Controller::match_devinfo(const struct sdb_device_info &match)
{
    /* the SPI core after an FMC_ACTIVE_CLK core is responsible for controlling
     * that clock's AD9510 chip, and that's how we locate it on the SDB */
    if (fac_regs.match_devinfo_lambda(match)) {
        fac_devinfo = match;
        return false;
    }

    if (fac_devinfo && spi_regs.match_devinfo_lambda(match)) {
        /* XXX: should we include a check for the offset between FMC_ACTIVE_CLK
         * and SPI cores? */
        fac_devinfo = std::nullopt;
        return true;
    }

    return false;
}

void Controller::set_devinfo(const struct sdb_device_info &new_devinfo)
{
    spi_regs.set_devinfo(new_devinfo);
}

bool Controller::get_reg(uint8_t addr, uint8_t &rdata, spi::Channel)
{
    const unsigned char wdata[] = {
        0x80, /* read 1 byte */
        addr,
    };

    return spi_regs.write_read_data(wdata, sizeof wdata, &rdata, sizeof rdata);
}

bool Controller::set_reg_ll(uint8_t addr, uint8_t value)
{
    const unsigned char wdata[] = {
        0x00, /* write 1 byte */
        addr,
        value,
    };

    return spi_regs.write_read_data(wdata, sizeof wdata, nullptr, 0);
}

bool Controller::update_parameters()
{
    if (!set_reg_ll(0x5a, 1))
        return false;

    /* wait for update to finish */
    uint8_t reg;
    do {
        if (!get_reg(0x5a, reg))
            return false;
    } while (reg);

    return true;
}

bool Controller::set_reg(const uint8_t addr, uint8_t value, spi::Channel)
{
    if (!set_reg_ll(addr, value))
        return false;

    uint8_t reg_read;
    if (!get_reg(addr, reg_read))
        return false;

    return reg_read == value;
}

bool Controller::set_a_div(uint8_t value)
{
    if (!set_reg(0x04, value))
        return false;

    return update_parameters();
}

bool Controller::set_b_div(uint16_t value)
{
    if (!set_reg(0x06, value & 0xff))
        return false;
    if (!set_reg(0x05, value >> 8))
        return false;

    return update_parameters();
}

bool Controller::set_r_div(uint16_t value)
{
    if (!set_reg(0x0c, value & 0xff))
        return false;
    if (!set_reg(0x0b, value >> 8))
        return false;

    return update_parameters();
}

bool Controller::set_defaults(spi::Channel)
{
    bool rv = true;
    if (rv)
        /* PLL_2 = normal CP | digital lock detect active-high | positive PFD */
        rv = set_reg(0x08, 0x03 | 0x04 | 0x40);
    if (rv)
        /* PLL_3 = 600mA CP */
        rv = set_reg(0x09, 0);
    if (rv)
        /* PLL_4 = default */
        rv = set_reg(0x0a, 0);
    if (rv)
        /* LVPECL_OUT0 = power up | 810mV output */
        rv = set_reg(0x3c, 0x08);
    if (rv)
        /* LVPECL_OUT1 = power up | 810mV output */
        rv = set_reg(0x3d, 0x08);
    if (rv)
        /* LVPECL_OUT2 = power up | 810mV output */
        rv = set_reg(0x3e, 0x08);
    if (rv)
        /* LVPECL_OUT3 = power up | 810mV output */
        rv = set_reg(0x3f, 0x08);
    if (rv)
        /* CMOS_OUT4 = power up | 3.5mA output */
        rv = set_reg(0x40, 0x02);
    if (rv)
        /* CMOS_OUT5 = power down | 3.5mA output */
        rv = set_reg(0x41, 0x03);
    if (rv)
        /* CMOS_OUT6 = power down | 3.5mA output */
        rv = set_reg(0x42, 0x03);
    if (rv)
        /* CMOS_OUT7 = power down | 3.5mA output */
        rv = set_reg(0x43, 0x03);
    if (rv)
        /* CLK_SELECT = sel clk2 | CLK1 power down */
        rv = set_reg(0x45, 0x02);
    if (rv)
        /* DIV0_PHASE = bypass | start high */
        rv = set_reg(0x49, 0x90);
    if (rv)
        /* DIV1_PHASE = bypass | start high */
        rv = set_reg(0x4b, 0x90);
    if (rv)
        /* DIV2_PHASE = bypass | start high */
        rv = set_reg(0x4d, 0x90);
    if (rv)
        /* DIV3_PHASE = bypass | start high */
        rv = set_reg(0x4f, 0x90);
    if (rv)
        /* DIV4_PHASE = bypass | start high */
        rv = set_reg(0x51, 0x90);
    if (rv)
        /* FUNCTION = SYNCB */
        rv = set_reg(0x58, 0x20);
    if (rv)
        rv = update_parameters();

    /* force a software synchronization */
    if (rv)
        /* FUNCTION = SYNCB | SOFT_SYNC */
        rv = set_reg(0x58, 0x20 | 0x04);
    if (rv)
        rv = update_parameters();
    if (rv)
        /* FUNCTION = SYNCB */
        rv = set_reg(0x58, 0x20);
    if (rv)
        rv = update_parameters();

    return rv;
}

} /* namespace ad9510 */
