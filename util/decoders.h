/*
 * Copyright (C) 2022 CNPEM (cnpem.br)
 * Author: Érico Nogueira <erico.rolim@lnls.br>
 *
 * Released according to the GNU GPL, version 3 or any later version.
 */

#ifndef DECODERS_H
#define DECODERS_H

#include <cstdint>
#include <cstdio>

#include <sys/types.h>

#include "pcie.h"

class RegisterDecoder {
  protected:
    size_t read_size;
    void *read_dest;

  public:
    virtual ~RegisterDecoder() {};
    virtual void read(const struct pcie_bars *, size_t);
    virtual void print(FILE *, bool) = 0;
};

#endif
