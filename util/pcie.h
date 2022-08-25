/*
 * Copyright (C) 2022 CNPEM (cnpem.br)
 * Author: Érico Nogueira <erico.rolim@lnls.br>
 *
 * Released according to the GNU GPL, version 3 or any later version.
 */

#ifndef PCIE_H
#define PCIE_H

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
};

#ifdef __cplusplus
extern "C" {
#endif
void bar2_read_v(struct pcie_bars *bars, size_t addr, void *dest, size_t n);
void bar2_read_dma(const struct pcie_bars *bars, size_t addr, unsigned bar, unsigned long physical_address, size_t n);
void bar4_write(struct pcie_bars *bars, size_t addr, uint32_t value);
void bar4_write_v(struct pcie_bars *bars, size_t addr, const void *src, size_t n);
uint32_t bar4_read(struct pcie_bars *bars, size_t addr);
void bar4_read_v(struct pcie_bars *bars, size_t addr, void *dest, size_t n);
#ifdef __cplusplus
}
#endif

#endif
