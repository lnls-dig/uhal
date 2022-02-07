/*
 * Copyright (C) 2022 CNPEM (cnpem.br)
 * Author: Érico Nogueira <erico.rolim@lnls.br>
 *
 * Released according to the GNU GPL, version 3 or any later version.
 */

#include <stdint.h>
#include <sys/types.h>

#include "wb_acq_core_regs.h"

/* PCIe Page constants */
/* SDRAM is accesses via 32-bit BAR (32-bit addressing) */
#define PCIE_SDRAM_PG_SHIFT                 0           /* bits */
#define PCIE_SDRAM_PG_MAX                   20          /* bits */
#define PCIE_SDRAM_PG_SIZE                  (1<<PCIE_SDRAM_PG_MAX)  /* in Bytes (8-bit) */
#define PCIE_SDRAM_PG_MASK                  ((PCIE_SDRAM_PG_SIZE-1) << \
                                                PCIE_SDRAM_PG_SHIFT)

/* Wishbone is accessed via 64-bit BAR (64-bit addressed) */
#define PCIE_WB_PG_SHIFT                    0           /* bits */
#define PCIE_WB_PG_MAX                      16          /* bits */
#define PCIE_WB_PG_SIZE                     (1<<PCIE_WB_PG_MAX)  /* in Bytes (8-bit) */
#define PCIE_WB_PG_MASK                     ((PCIE_WB_PG_SIZE-1) << \
                                                PCIE_WB_PG_SHIFT)

/* PCIe SDRAM Address Page number and offset extractor */
#define PCIE_ADDR_SDRAM_PG_OFFS(addr)       ((addr & PCIE_SDRAM_PG_MASK) >> \
                                                PCIE_SDRAM_PG_SHIFT)
#define PCIE_ADDR_SDRAM_PG(addr)            ((addr & ~PCIE_SDRAM_PG_MASK) >> \
                                                PCIE_SDRAM_PG_MAX)

/* PCIe WB Address Page number and offset extractor */
#define PCIE_ADDR_WB_PG_OFFS(addr)          ((addr & PCIE_WB_PG_MASK) >> \
                                                PCIE_WB_PG_SHIFT)
#define PCIE_ADDR_WB_PG(addr)               ((addr & ~PCIE_WB_PG_MASK) >> \
                                                PCIE_WB_PG_MAX)

#define WB_QWORD_ACC                        3           /* 64-bit addressing */
#define WB_DWORD_ACC                        2           /* 32-bit addressing */
#define WB_WORD_ACC                         1           /* 16-bit addressing */
#define WB_BYTE_ACC                         0           /* 8-bit addressing */

/* FPGA PCIe registers. These are inside bar0 and must match
 * the FPGA firmware */
#define PCIE_CFG_REG_SDRAM_PG               (7 << WB_DWORD_ACC)
#define PCIE_CFG_REG_WB_PG                  (9 << WB_DWORD_ACC)

/* Page Macros */
#define SET_PG(bar0, which, num)                                    \
    do {                                                            \
        bar0[which >> WB_DWORD_ACC] =                               \
                num;                                                \
    } while (0)

#define SET_WB_PG(bar0, num)                                        \
    SET_PG(bar0, PCIE_CFG_REG_WB_PG, num)

struct pcie_bars {
    void *bar0;
    void *bar4;
};

uint32_t bar4_read(const struct pcie_bars *bars, size_t addr);
void bar4_read_u32s(const struct pcie_bars *bars, size_t addr, void *dest, size_t n);

void decode_registers_print(struct acq_core *acq, FILE *f);
