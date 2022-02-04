#include <stdint.h>
#include <sys/types.h>

/************************************************************/
/******************* PCIe Page constants ********************/
/************************************************************/
/* Some FPGA PCIe constants */
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

/************************************************************/
/******************* FPGA PCIe Registers ********************/
/************************************************************/
/* FPGA PCIe registers. These are inside bar0r These must match
 * the FPGA firmware */

#define PCIE_CFG_REG_SDRAM_PG               (7 << WB_DWORD_ACC)
#define PCIE_CFG_REG_WB_PG                  (9 << WB_DWORD_ACC)

/************************************************************/
/*********************** Page Macros ************************/
/************************************************************/
#define SET_PG(bar0, which, num)                                    \
    do {                                                            \
        bar0[which >> WB_DWORD_ACC] =                               \
                num;                                                \
    } while (0)

#define SET_SDRAM_PG(bar0, num)                                     \
    SET_PG(bar0, PCIE_CFG_REG_SDRAM_PG, num)

#define SET_WB_PG(bar0, num)                                        \
    SET_PG(bar0, PCIE_CFG_REG_WB_PG, num)

#define BAR_RW_TYPE                         uint32_t

/* Read or write to BAR */
#define BAR_RW_8(barp, addr, datap, rw)                             \
    do {                                                            \
        (rw) ?                                                      \
        (*(datap) = *(BAR_RW_TYPE *)(((uint8_t *)barp) + (addr))) : \
        (*(BAR_RW_TYPE *)(((uint8_t *)barp) + (addr)) = *(datap));  \
    } while (0)

/* BAR0 is BYTE addressed for the user */
#define BAR0_RW(barp, addr, datap, rw)                              \
    BAR_RW_8(barp, addr, datap, rw)

/* BAR2 is BYTE addressed for the user */
#define BAR2_RW(barp, addr, datap, rw)                              \
    BAR_RW_8(barp, addr, datap, rw)

/* BAR4 is BYTE addresses for the user */
/* On PCIe Core FPGA firmware the wishbone address is provided with
 * only 29 bits, with the LSB zeroed:
 *
 *  bit 31        . . .      bit 3   bit 2   bit 1   bit 0
 *   A31          . . .        A3     '0'     '0'    '0'
 *
 * This is done as the BAR4 is 64-bit addressed. But, the output of the
 * PCIe wrapper are right shifted to avoid dealing with this particularity:
 *
 *  bit 31   bit 30   bit 29  bit 28    . . .      bit 3   bit 2   bit 1   bit 0
 *   '0'      '0'     '0'       A31     . . .        A6     A5      A4      A3
 *
 * */
#define BAR4_RW(barp, addr, datap, rw)                              \
    do {                                                            \
        (rw) ?                                                      \
        (*(datap) = *((barp) + (addr))) :                           \
        (*((barp) + (addr)) = *(datap));                            \
    } while (0)

#include "wb_acq_core_regs.h"

struct pcie_bars {
    void *bar0;
    void *bar4;
};

uint32_t bar4_read(const struct pcie_bars *bars, size_t addr);
void bar4_read_u32s(const struct pcie_bars *bars, size_t addr, void *dest, size_t n);

void decode_registers_print(struct acq_core *acq, FILE *f);
