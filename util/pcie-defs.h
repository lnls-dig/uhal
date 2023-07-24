/*
 * Copyright (C) 2023 CNPEM (cnpem.br)
 * Author: Érico Nogueira <erico.rolim@lnls.br>
 *
 * Released according to the GNU GPL, version 3 or any later version.
 */

/** \file
 * This file contains the definition of pcie_bars, so that library users can
 * use the struct without also having access to the functions from pcie.h. */

#ifndef PCIE_STRUCT_H
#define PCIE_STRUCT_H

#include <pthread.h>
#include <stdint.h>
#include <sys/types.h>

enum bar_lock {
    BAR2,
    BAR4,
    NUM_LOCKS,
};

struct pcie_bars {
    volatile void *bar0;
    volatile void *bar2;
    volatile void *bar4;

    size_t sizes[3];

    /* private fields */
    uint32_t last_bar4_page; /* protected by locks[BAR4] */

    /* we only need locking for bar2 and bar4, since they are paged.
     * these mutexes MUST be initialized as recursive */
    pthread_mutex_t locks[NUM_LOCKS];

    int fd;
};

#endif
