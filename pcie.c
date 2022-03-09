/*
 * Copyright (C) 2022 CNPEM (cnpem.br)
 * Author: Érico Nogueira <erico.rolim@lnls.br>
 *
 * Released according to the GNU GPL, version 3 or any later version.
 */

#include <assert.h>
#include <stdio.h>

#include "pcie.h"

static void bar_generic_read_v(const struct pcie_bars *bars, size_t addr, void *dest, size_t n,
    uint32_t (*fn)(const struct pcie_bars *bars, size_t addr))
{
    /* we receive n in bytes, it's our job to turn it into uint32_t accesses;
     * we begin by asserting the alignment */
    assert((addr & 0x3) == 0);
    assert((n & 0x3) == 0);

    uint32_t *destp = dest;
    for (size_t i = 0; i < n; i += 4)
        destp[i/4] = fn(bars, addr + i);
}

static volatile uint32_t *bar0_get_u32p(const struct pcie_bars *bars, size_t addr)
{
    return (volatile void *)((unsigned char *)bars->bar0 + addr);
}

static uint32_t bar0_read(const struct pcie_bars *bars, size_t addr)
{
    return *bar0_get_u32p(bars, addr);
}

static void bar0_write(const struct pcie_bars *bars, size_t addr, uint32_t value)
{
    *bar0_get_u32p(bars, addr) = value;
}

static void set_sdram_pg(const struct pcie_bars *bars, int num)
{
    volatile uint32_t *bar0 = bars->bar0;
    SET_PG(bar0, PCIE_CFG_REG_SDRAM_PG, num);
}

static void set_wb_pg(const struct pcie_bars *bars, int num)
{
    volatile uint32_t *bar0 = bars->bar0;
    SET_PG(bar0, PCIE_CFG_REG_WB_PG, num);
}

static size_t bar2_access_offset(const struct pcie_bars *bars, size_t addr)
{
    uint32_t pg_num = PCIE_ADDR_SDRAM_PG (addr);

    /* set sdram page in bar0, all accesses need to do this */
    set_sdram_pg(bars, pg_num);

    return PCIE_ADDR_SDRAM_PG_OFFS (addr);
}

static volatile uint32_t *bar2_get_u32p(const struct pcie_bars *bars, size_t addr)
{
    return (volatile void *)((unsigned char *)bars->bar2 + bar2_access_offset(bars, addr));
}

static uint32_t bar2_read(const struct pcie_bars *bars, size_t addr)
{
    return *bar2_get_u32p(bars, addr);
}

void bar2_read_v(const struct pcie_bars *bars, size_t addr, void *dest, size_t n)
{
    bar_generic_read_v(bars, addr, dest, n, bar2_read);
}

void bar2_read_dma(const struct pcie_bars *bars, size_t addr, unsigned bar, unsigned long physical_address, size_t n)
{
    /* uses upstream DMA */
    size_t dma_base_address = PCIE_CFG_REG_DMA_US_BASE;

    /* FIXME: check for ongoing transfer */

    /* reset dma */
    bar0_write(bars, dma_base_address + PCIE_CFG_REG_DMA_CTRL, PCIE_CFG_DMA_CTRL_VALID | PCIE_CFG_TX_CTRL_CHANNEL_RST);
    /* set dma */
    bar0_write(bars, dma_base_address + PCIE_CFG_REG_DMA_PAH, ((uint64_t)addr) >> 32);
    bar0_write(bars, dma_base_address + PCIE_CFG_REG_DMA_PAL, addr);
    bar0_write(bars, dma_base_address + PCIE_CFG_REG_DMA_HAH, ((uint64_t)physical_address) >> 32);
    bar0_write(bars, dma_base_address + PCIE_CFG_REG_DMA_HAL, physical_address);
    bar0_write(bars, dma_base_address + PCIE_CFG_REG_DMA_BDAH, 0);
    bar0_write(bars, dma_base_address + PCIE_CFG_REG_DMA_BDAL, 0);
    bar0_write(bars, dma_base_address + PCIE_CFG_REG_DMA_BDAH, 0);
    bar0_write(bars, dma_base_address + PCIE_CFG_REG_DMA_LENG, n);

    /* start transfer */
    bar0_write(bars, dma_base_address + PCIE_CFG_REG_DMA_CTRL,
        PCIE_CFG_DMA_CTRL_VALID | PCIE_CFG_DMA_CTRL_LAST | PCIE_CFG_DMA_CTRL_AINC | (bar << PCIE_CFG_DMA_CTRL_BAR_SHIFT));

    /* wait for completion */
    uint32_t status_reg;
    do {
        status_reg = bar0_read(bars, dma_base_address + PCIE_CFG_REG_DMA_STA);
    } while(!(status_reg & PCIE_CFG_DMA_STA_DONE));
}

static size_t bar4_access_offset(const struct pcie_bars *bars, size_t addr)
{
    uint32_t pg_num = PCIE_ADDR_WB_PG (addr);

    /* set memory page in bar0, all accesses need to do this */
    set_wb_pg(bars, pg_num);

    /* the offset defined in the SBC is aligned to 8 (3 LSB are 0),
     * but for encoding purposes the value is shifted right,
     * so we have to shift them back left to access the values */
    return PCIE_ADDR_WB_PG_OFFS (addr) << 3;
}

static volatile uint32_t *bar4_get_u32p(const struct pcie_bars *bars, size_t addr)
{
    return (volatile void *)((unsigned char *)bars->bar4 + bar4_access_offset(bars, addr));
}

void bar4_write(const struct pcie_bars *bars, size_t addr, uint32_t value)
{
    *bar4_get_u32p(bars, addr) = value;
}

void bar4_write_v(const struct pcie_bars *bars, size_t addr, const void *src, size_t n)
{
    const uint32_t *srcp = src;
    for (size_t i = 0; i < n; i += 4) {
        bar4_write(bars, addr + i, srcp[i/4]);
    }
}

uint32_t bar4_read(const struct pcie_bars *bars, size_t addr)
{
    return *bar4_get_u32p(bars, addr);
}

void bar4_read_v(const struct pcie_bars *bars, size_t addr, void *dest, size_t n)
{
    bar_generic_read_v(bars, addr, dest, n, bar4_read);
}
