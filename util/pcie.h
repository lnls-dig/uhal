/*
 * Copyright (C) 2022 CNPEM (cnpem.br)
 * Author: Érico Nogueira <erico.rolim@lnls.br>
 *
 * Released according to the GNU GPL, version 3 or any later version.
 */

#ifndef PCIE_H
#define PCIE_H

#include <stdint.h>
#include <sys/types.h>

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

/* First register of the block*/
#define PCIE_CFG_REG_DMA_US_BASE            (11 << WB_DWORD_ACC)

/* Offset for DMA registers */
#define PCIE_CFG_REG_DMA_PAH                (0 << WB_DWORD_ACC)
#define PCIE_CFG_REG_DMA_PAL                (1 << WB_DWORD_ACC)
#define PCIE_CFG_REG_DMA_HAH                (2 << WB_DWORD_ACC)
#define PCIE_CFG_REG_DMA_HAL                (3 << WB_DWORD_ACC)
#define PCIE_CFG_REG_DMA_BDAH               (4 << WB_DWORD_ACC)
#define PCIE_CFG_REG_DMA_BDAL               (5 << WB_DWORD_ACC)
#define PCIE_CFG_REG_DMA_LENG               (6 << WB_DWORD_ACC)
#define PCIE_CFG_REG_DMA_CTRL               (7 << WB_DWORD_ACC)
#define PCIE_CFG_REG_DMA_STA                (8 << WB_DWORD_ACC)

/* Relevant values for DMA registers */
#define PCIE_CFG_DMA_CTRL_AINC (1 << 15)
#define PCIE_CFG_DMA_CTRL_BAR_SHIFT 16
#define PCIE_CFG_DMA_CTRL_LAST (1 << 24)
#define PCIE_CFG_DMA_CTRL_VALID (1 << 25)
#define PCIE_CFG_TX_CTRL_CHANNEL_RST 0x0A
#define PCIE_CFG_DMA_STA_DONE (1 << 0)
#define PCIE_CFG_DMA_STA_TIMEOUT (1 << 4)

struct pcie_bars {
    volatile void *bar0;
    volatile void *bar2;
    volatile void *bar4;

    size_t sizes[3];
};

#ifdef __cplusplus
extern "C" {
#endif
void bar2_read_v(const struct pcie_bars *bars, size_t addr, void *dest, size_t n);
void bar2_read_dma(const struct pcie_bars *bars, size_t addr, unsigned bar, unsigned long physical_address, size_t n);
void bar4_write(const struct pcie_bars *bars, size_t addr, uint32_t value);
void bar4_write_v(const struct pcie_bars *bars, size_t addr, const void *src, size_t n);
uint32_t bar4_read(const struct pcie_bars *bars, size_t addr);
void bar4_read_v(const struct pcie_bars *bars, size_t addr, void *dest, size_t n);
#ifdef __cplusplus
}
#endif

#endif
