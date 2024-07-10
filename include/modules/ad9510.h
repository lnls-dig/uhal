#ifndef AD9510_H
#define AD9510_H

#include <modules/fmc_active_clk.h>
#include <modules/spi.h>

namespace ad9510 {

class Controller {
    spi::Controller spi_regs;

    fmc_active_clk::Core fac_regs;
    mutable std::optional<struct sdb_device_info> fac_devinfo;

    bool set_reg_ll(uint8_t, uint8_t);
    bool update_parameters();

  public:
    Controller(struct pcie_bars &);

    bool match_devinfo(const struct sdb_device_info &);
    void set_devinfo(const struct sdb_device_info &);

    bool get_reg(uint8_t, uint8_t &, spi::Channel = {0});
    bool set_reg(uint8_t, uint8_t, spi::Channel = {0});

    bool set_a_div(uint8_t);
    bool set_b_div(uint16_t);
    bool set_r_div(uint16_t);
    bool set_defaults(spi::Channel = {0});
};

} /* namespace ad9510 */

#endif
