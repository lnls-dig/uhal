/*
 * Copyright (C) 2022 CNPEM (cnpem.br)
 * Author: Érico Nogueira <erico.rolim@lnls.br>
 *
 * Released according to the GNU GPL, version 3 or any later version.
 */

#include <assert.h>
#include <stdio.h>

#if defined(__x86_64__) && defined(__SSE4_1__)
# define INCLUDE_IMMINTRIN
# define USE_SSE41
#endif

#ifdef INCLUDE_IMMINTRIN
# include <immintrin.h>
#endif

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

static inline volatile uint32_t *bar2_get_u32p_small(const struct pcie_bars *bars, size_t addr, unsigned index)
{
    return (volatile uint32_t *)((unsigned char *)bars->bar2 + addr) + index;
}

void bar2_read_v(const struct pcie_bars *bars, size_t addr, void *dest, size_t n)
{
    size_t sz = bars->sizes[1];

#ifdef USE_SSE41
    __m128i scratch[256] __attribute__((aligned(64)));
#endif

    while (n) {
        const size_t addr_now = PCIE_ADDR_SDRAM_PG_OFFS(addr);
        const size_t can_read = sz - addr_now;
        const size_t to_read = can_read < n ? can_read : n;

        set_sdram_pg(bars, PCIE_ADDR_SDRAM_PG(addr));

#ifdef USE_SSE41
        /* stores number of uint32_t's written */
        size_t i = 0;
        const size_t alignment = 64, read_size = 1024;

        size_t head = addr_now % alignment;
        head = head ? alignment - head : 0;
        for (size_t j = head; i < to_read/4 && j; i++, j-=4)
            ((uint32_t *)dest)[i] = *bar2_get_u32p_small(bars, addr_now, i);

        for (; i + (read_size-1) < to_read/4; i += read_size) {
            __m128i a, b, c, d;

            _mm_mfence();
            for (size_t j = 0; j < 64; j++) {
                size_t base = i + j * 16;
                a = _mm_stream_load_si128((__m128i *)bar2_get_u32p_small(bars, addr_now, base));
                b = _mm_stream_load_si128((__m128i *)bar2_get_u32p_small(bars, addr_now, base+4));
                c = _mm_stream_load_si128((__m128i *)bar2_get_u32p_small(bars, addr_now, base+8));
                d = _mm_stream_load_si128((__m128i *)bar2_get_u32p_small(bars, addr_now, base+12));

                base = j * 4;
                _mm_store_si128(scratch + base, a);
                _mm_store_si128(scratch + base + 1, b);
                _mm_store_si128(scratch + base + 2, c);
                _mm_store_si128(scratch + base + 3, d);
            }

            _mm_mfence();
            for (size_t j = 0; j < 64; j++) {
                size_t base = j * 4;
                a = _mm_load_si128(scratch + base);
                b = _mm_load_si128(scratch + base + 1);
                c = _mm_load_si128(scratch + base + 2);
                d = _mm_load_si128(scratch + base + 3);

                base = i + j * 16;
                _mm_stream_si128((__m128i *)((uint32_t *)dest + base), a);
                _mm_stream_si128((__m128i *)((uint32_t *)dest + base+4), b);
                _mm_stream_si128((__m128i *)((uint32_t *)dest + base+8), c);
                _mm_stream_si128((__m128i *)((uint32_t *)dest + base+12), d);
            }
        }

        for (; i < to_read/4; i++)
            ((uint32_t *)dest)[i] = *bar2_get_u32p_small(bars, addr_now, i);
#else
        for (size_t i = 0; i < to_read/4; i++)
            ((uint32_t *)dest)[i] = *bar2_get_u32p_small(bars, addr_now, i);
#endif

        n -= to_read;
        addr += to_read;
        dest = (unsigned char *)dest + to_read;
    }
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
