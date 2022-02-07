/*
 * Copyright (C) 2022 CNPEM (cnpem.br)
 * Author: Érico Nogueira <erico.rolim@lnls.br>
 *
 * Released according to the GNU GPL, version 3 or any later version.
 */

#include <cstdint>
#include <cstdio>

#include <sys/types.h>

#include "pcie.h"
#include "wb_acq_core_regs.h"

class RegisterDecoder {
  protected:
    size_t read_size;
    void *read_dest;

  public:
    virtual ~RegisterDecoder() {};
    virtual void read(const struct pcie_bars *, size_t);
    virtual void print(FILE *) { /* dummy */ }
};

class LnlsBpmAcqCore: public RegisterDecoder {
    struct acq_core regs;

  public:
    LnlsBpmAcqCore();
    void print(FILE *);
};
