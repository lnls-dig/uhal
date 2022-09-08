/*
 * Copyright (C) 2022 CNPEM (cnpem.br)
 * Author: Érico Nogueira <erico.rolim@lnls.br>
 *
 * Released according to the GNU GPL, version 3 or any later version.
 */

#ifndef CONTROLLERS_H
#define CONTROLLERS_H

#include "util_sdb.h"

class RegisterController {
  protected:
    struct pcie_bars &bars;
    struct sdb_device_info devinfo;
    size_t addr;

    RegisterController(struct pcie_bars &bars);

  public:
    void set_devinfo(const struct sdb_device_info &);
};

#endif
