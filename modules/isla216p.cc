#include "printer.h"
#include "util.h"
#include "modules/isla216p.h"

namespace isla216p {

Controller::Controller(struct pcie_bars &bars):
    spi_regs(bars),
    f250_regs(bars)
{
}

bool Controller::match_devinfo(const struct sdb_device_info &match)
{
    /* the third SPI core after an FMC250M_4CH core is responsible for
     * controlling that FMC's ISLA216P chip, and that's how we locate it */
    if (f250_devinfo && spi_first_devinfo && spi_second_devinfo && spi_regs.match_devinfo_lambda(match)) {
        /* XXX: should we include a check for the offset between FMC250M_4CH
         * and SPI cores? */
        f250_devinfo = std::nullopt;
        spi_first_devinfo = std::nullopt;
        spi_second_devinfo = std::nullopt;
        return true;
    }

    if (f250_devinfo && spi_first_devinfo && spi_regs.match_devinfo_lambda(match)) {
        spi_second_devinfo = match;
        return false;
    }

    if (f250_devinfo && spi_regs.match_devinfo_lambda(match)) {
        spi_first_devinfo = match;
        return false;
    }

    if (f250_regs.match_devinfo_lambda(match)) {
        f250_devinfo = match;
        return false;
    }

    return false;
}

void Controller::set_devinfo(const struct sdb_device_info &new_devinfo)
{
    spi_regs.set_devinfo(new_devinfo);
}

bool Controller::get_reg(uint8_t addr, uint8_t &rdata, spi::Channel channel)
{
    const unsigned char wdata[] = {
        0x80, /* read 1 byte */
        addr,
    };

    return spi_regs.write_read_data(wdata, sizeof wdata, &rdata, sizeof rdata, channel.channel);
}

bool Controller::set_reg(uint8_t addr, uint8_t rdata, spi::Channel channel)
{
    const unsigned char wdata[] = {
        0x00, /* write 1 byte */
        addr,
    };

    return spi_regs.write_read_data(wdata, sizeof wdata, &rdata, sizeof rdata, channel.channel);
}

bool Controller::set_defaults(spi::Channel)
{
    (void)this;
    return false;
}

} /* namespace isla216p */
