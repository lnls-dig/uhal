/*
 * Copyright (C) 2022 CNPEM (cnpem.br)
 * Author: Érico Nogueira <erico.rolim@lnls.br>
 *
 * Released according to the GNU GPL, version 3 or any later version.
 */

#include <assert.h>
#include <stdio.h>

#include "pcie.h"

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
    /* we receive n in bytes, it's our job to turn it into uint32_t accesses;
     * we begin by asserting the alignment */
    assert((addr & 0x3) == 0);
    assert((n & 0x3) == 0);

    uint32_t *destp = dest;
    for (size_t i = 0; i < n; i += 4)
        destp[i/4] = bar2_read(bars, addr + i);
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

void bar4_write_u32s(const struct pcie_bars *bars, size_t addr, const void *src, size_t n)
{
    const uint32_t *srcp = src;
    for (size_t i = 0; i < n; i++) {
        bar4_write(bars, addr + i*4, srcp[i]);
    }
}

uint32_t bar4_read(const struct pcie_bars *bars, size_t addr)
{
    return *bar4_get_u32p(bars, addr);
}

void bar4_read_u32s(const struct pcie_bars *bars, size_t addr, void *dest, size_t n)
{
    uint32_t *destp = dest;

    /* registers are offset in multiples of 4 */
    for (size_t i = 0; i < n; i++)
        destp[i] = bar4_read(bars, addr + i*4);
}
