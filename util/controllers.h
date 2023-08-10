/*
 * Copyright (C) 2022 CNPEM (cnpem.br)
 * Author: Érico Nogueira <erico.rolim@lnls.br>
 *
 * Released according to the GNU GPL, version 3 or any later version.
 */

#ifndef CONTROLLERS_H
#define CONTROLLERS_H

#include "decoders.h"
#include "sdb-defs.h"

class RegisterController: public RegisterDecoderBase {
  protected:
    RegisterController(struct pcie_bars &bars, const struct sdb_device_info &);

    /** Child classes can implement this function to capture one-time values
     * from hardware and perform any other necessary initialization */
    virtual void set_devinfo_callback() { }

    /** Child classes must implement this function to encode their
     * configuration parameters into hardware registers */
    virtual void encode_params() = 0;

  public:
    void set_devinfo(const struct sdb_device_info &) override;

    /** Child classes can implement this function when their write procedures
     * require more than simply writing the regs structure */
    virtual void write_params();
};

#endif
