#ifndef ISLA216P_H
#define ISLA216P_H

#include <modules/fmc250m_4ch.h>
#include <modules/spi.h>

namespace isla216p {

class Controller {
    spi::Controller spi_regs;

    fmc250m_4ch::Core f250_regs;
    mutable std::optional<struct sdb_device_info> f250_devinfo;
    mutable std::optional<struct sdb_device_info> spi_first_devinfo, spi_second_devinfo;

  public:
    Controller(struct pcie_bars &);

    bool match_devinfo(const struct sdb_device_info &);
    void set_devinfo(const struct sdb_device_info &);

    bool get_reg(uint8_t, uint8_t &, spi::Channel);
    bool set_reg(uint8_t, uint8_t, spi::Channel);
    bool set_defaults(spi::Channel);
};

} /* namespace isla216p */

#endif
