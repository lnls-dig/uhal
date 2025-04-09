#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#if defined(__x86_64__) && defined(__SSE4_1__)
# define INCLUDE_IMMINTRIN
# define USE_SSE41
#endif

#ifdef INCLUDE_IMMINTRIN
# include <immintrin.h>
#endif

#include "pcie.h"

/* PCIe Page constants */
/* SDRAM is accesses via 32-bit BAR (32-bit addressing) */
#define PCIE_SDRAM_PG_SHIFT                 0           /* bits */
#define PCIE_SDRAM_PG_MAX                   20          /* bits */
#define PCIE_SDRAM_PG_SIZE                  (1<<PCIE_SDRAM_PG_MAX)  /* in Bytes (8-bit) */
#define PCIE_SDRAM_PG_MASK                  ((PCIE_SDRAM_PG_SIZE-1) << PCIE_SDRAM_PG_SHIFT)

/* Wishbone is accessed via 64-bit BAR (64-bit addressed) */
#define PCIE_WB_PG_SHIFT                    0           /* bits */
#define PCIE_WB_PG_MAX                      16          /* bits */
#define PCIE_WB_PG_SIZE                     (1<<PCIE_WB_PG_MAX)  /* in Bytes (8-bit) */
#define PCIE_WB_PG_MASK                     ((PCIE_WB_PG_SIZE-1) << PCIE_WB_PG_SHIFT)

/* PCIe SDRAM Address Page number and offset extractor */
#define PCIE_ADDR_SDRAM_PG_OFFS(addr)       (((addr) & PCIE_SDRAM_PG_MASK) >> PCIE_SDRAM_PG_SHIFT)
#define PCIE_ADDR_SDRAM_PG(addr)            (((addr) & ~PCIE_SDRAM_PG_MASK) >> PCIE_SDRAM_PG_MAX)

/* PCIe WB Address Page number and offset extractor */
#define PCIE_ADDR_WB_PG_OFFS(addr)          (((addr) & PCIE_WB_PG_MASK) >> PCIE_WB_PG_SHIFT)
#define PCIE_ADDR_WB_PG(addr)               (((addr) & ~PCIE_WB_PG_MASK) >> PCIE_WB_PG_MAX)

#define WB_QWORD_ACC                        3           /* 64-bit addressing */
#define WB_DWORD_ACC                        2           /* 32-bit addressing */
#define WB_WORD_ACC                         1           /* 16-bit addressing */
#define WB_BYTE_ACC                         0           /* 8-bit addressing */

/* FPGA PCIe registers. These are inside bar0 and must match
 * the FPGA firmware */
#define PCIE_CFG_REG_SDRAM_PG               (7 << WB_DWORD_ACC)
#define PCIE_CFG_REG_WB_PG                  (9 << WB_DWORD_ACC)

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
#define PCIE_CFG_DMA_STA_DONE (1 << 0)
#define PCIE_CFG_DMA_STA_TIMEOUT (1 << 4)

/* Other registers and values */
#define PCIE_CFG_REG_TX_CTRL                (30 << WB_DWORD_ACC)
#define PCIE_CFG_REG_EB_STACON              (36 << WB_DWORD_ACC)
#define PCIE_CFG_TX_CTRL_CHANNEL_RST 0x0A

static void bar_generic_read_v(struct pcie_bars *bars, size_t addr, void *dest, size_t n,
    uint32_t (*fn)(struct pcie_bars *bars, size_t addr))
{
    /* we receive n in bytes, it's our job to turn it into uint32_t accesses;
     * we begin by asserting the alignment */
    assert((addr & 0x3) == 0);
    assert((n & 0x3) == 0);

    __attribute__((__may_alias__)) uint32_t *destp = dest;
    for (size_t i = 0; i < n; i += 4)
        destp[i/4] = fn(bars, addr + i);
}

static volatile uint32_t *bar0_get_u32p(const struct pcie_bars *bars, size_t addr)
{
    return (volatile void *)((unsigned char *)bars->bar0 + addr);
}

static void bar0_write(const struct pcie_bars *bars, size_t addr, uint32_t value)
{
    *bar0_get_u32p(bars, addr) = value;

    /* generate a read to flush writes; see bar4_write */
    *bar0_get_u32p(bars, addr);
}

static void set_sdram_pg(const struct pcie_bars *bars, int num)
{
    bar0_write(bars, PCIE_CFG_REG_SDRAM_PG, num);
}

static void set_wb_pg(const struct pcie_bars *bars, int num)
{
    bar0_write(bars, PCIE_CFG_REG_WB_PG, num);
}

static inline volatile uint32_t *bar2_get_u32p_small(const struct pcie_bars *bars, size_t addr, unsigned index)
{
    return (volatile uint32_t *)((unsigned char *)bars->bar2 + addr) + index;
}

void bar2_read_v(struct pcie_bars *bars, size_t addr, void *dest, size_t n)
{
    pthread_mutex_lock(&bars->locks[BAR2]);

    size_t sz = bars->sizes[1];

#ifdef USE_SSE41
    __m128i scratch[256] __attribute__((aligned(64)));
#endif

    __attribute__((__may_alias__)) uint32_t *destp = dest;

    while (n) {
        const size_t addr_now = PCIE_ADDR_SDRAM_PG_OFFS(addr);
        const size_t can_read = sz - addr_now;
        const size_t to_read = can_read < n ? can_read : n;

        set_sdram_pg(bars, PCIE_ADDR_SDRAM_PG(addr));

        /* stores number of uint32_t's written */
        size_t i = 0;

#ifdef USE_SSE41
        const size_t alignment = 64;
        const size_t read_size = 1024;

        size_t head = addr_now % alignment;
        head = head ? alignment - head : 0;
        for (size_t j = head; i < to_read/4 && j; i++, j-=4)
            destp[i] = *bar2_get_u32p_small(bars, addr_now, i);

        for (; i + (read_size-1) < to_read/4; i += read_size) {
            __m128i a;
            __m128i b;
            __m128i c;
            __m128i d;

            _mm_mfence();
            for (size_t j = 0; j < 64; j++) {
                size_t base = i + (j * 16);
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
                _mm_stream_si128((__m128i *)(destp + base), a);
                _mm_stream_si128((__m128i *)(destp + base+4), b);
                _mm_stream_si128((__m128i *)(destp + base+8), c);
                _mm_stream_si128((__m128i *)(destp + base+12), d);
            }
        }
#endif
        for (; i < to_read/4; i++)
            destp[i] = *bar2_get_u32p_small(bars, addr_now, i);

        n -= to_read;
        addr += to_read;
        assert((to_read & 0x3) == 0);
        destp = destp + to_read/4;
    }

    pthread_mutex_unlock(&bars->locks[BAR2]);
}

static size_t bar4_access_offset(struct pcie_bars *bars, size_t addr)
{
    uint32_t pg_num = PCIE_ADDR_WB_PG (addr);

    /* set memory page in bar0, all accesses need this set to the correct value.
     * since access to each bar is exclusive, save on writes to bar0 by checking
     * the value that was previously set */
    if (pg_num != bars->last_bar4_page) {
        set_wb_pg(bars, pg_num);
        bars->last_bar4_page = pg_num;
    }

    /* the offset defined in the SBC is aligned to 8 (3 LSB are 0),
     * but for encoding purposes the value is shifted right,
     * so we have to shift them back left to access the values */
    return PCIE_ADDR_WB_PG_OFFS (addr) << 3;
}

static volatile uint32_t *bar4_get_u32p(struct pcie_bars *bars, size_t addr)
{
    return (volatile void *)((unsigned char *)bars->bar4 + bar4_access_offset(bars, addr));
}

void bar4_write(struct pcie_bars *bars, size_t addr, uint32_t value)
{
    pthread_mutex_lock(&bars->locks[BAR4]);

    if (bars->fserport) {
        fprintf(bars->fserport, "W%08zX%08X\n", addr, value);
        char *line = NULL;
        size_t buff_size = 0;
        ssize_t ans_str_size = getline(&line, &buff_size, bars->fserport);
        assert(ans_str_size == 2);
        if (line[0] == 'O') {
        }
        else if (line[0] == 'E') {
            fprintf(stderr, "Write error\n");
        } else if (line[0] == 'T') {
            fprintf(stderr, "Wishbone timeout\n");
        } else {
            fprintf(stderr, "Unknown response: %c\n", line[0]);
        }
        if (line) free(line);

        goto unlock;
    }

    *bar4_get_u32p(bars, addr) = value;

    /* generate a read so the write is flushed;
     * corruption / wishbone timeouts have been observed when writing too many
     * words at once into bar4.
     * if desired, we can add a check for the read being equal to 0xffffffff to detect
     * timeouts in this layer. */
    *bar4_get_u32p(bars, addr);

  unlock:
    pthread_mutex_unlock(&bars->locks[BAR4]);
}

void bar4_write_v(struct pcie_bars *bars, size_t addr, const void *src, size_t n)
{
    pthread_mutex_lock(&bars->locks[BAR4]);

    const uint32_t *srcp = src;
    for (size_t i = 0; i < n; i += 4) {
        bar4_write(bars, addr + i, srcp[i/4]);
    }

    pthread_mutex_unlock(&bars->locks[BAR4]);
}

uint32_t bar4_read(struct pcie_bars *bars, size_t addr)
{
    pthread_mutex_lock(&bars->locks[BAR4]);

    uint32_t rv;
    if (bars->fserport) bar4_read_v(bars, addr, &rv, 4);
    else rv = *bar4_get_u32p(bars, addr);

    pthread_mutex_unlock(&bars->locks[BAR4]);

    return rv;
}

/** Max number of words that uart_read_cmd() can read at once */
static const size_t max_word_blk_size = 256;

/** Read 32 bits words, max num_words is #max_word_blk_size */
static ssize_t uart_read_cmd(struct pcie_bars *bars, size_t addr, void *dest, ssize_t num_words)
{
    assert(num_words <= (ssize_t)max_word_blk_size);

    uint32_t *dest_u32 = (uint32_t*)dest;
    ssize_t words_read = 0;
    if (bars->fserport && num_words > 0) {
        fprintf(bars->fserport, "R%08zX%02zX\n", addr, num_words - 1);
        char *line = NULL;
        size_t buff_size = 0;
        ssize_t ans_str_size = getline(&line, &buff_size, bars->fserport);
        if (ans_str_size > 0) {
            if (line[0] == 'O') {
                char *data = line + 1;
                for (ssize_t i = 0; i < ((ans_str_size - 2) / 8) && i < num_words; i++) {
                    int elements = sscanf(data + (i*8), "%8x", dest_u32);
                    dest_u32 += 1;
                    words_read += 1;
                    assert(elements == 1);
                }
            } else if (line[0] == 'E') {
                fprintf(stderr, "Read error\n");
                words_read = -EIO;
            } else if (line[0] == 'T') {
                fprintf(stderr, "Wishbone timeout\n");
                words_read = -ETIMEDOUT;
            } else {
                fprintf(stderr, "Unknown response: %c\n", line[0]);
                words_read = -EIO;
            }
        } else {
            fprintf(stderr, "UART Timeout\n");
            words_read = -ETIMEDOUT;
        }
        if (line) free(line);
    }
    return words_read;
}

void bar4_read_v(struct pcie_bars *bars, size_t addr, void *dest, size_t n)
{
    pthread_mutex_lock(&bars->locks[BAR4]);

    if (bars->fserport) {
        /* Assert that the address is 32 bits aligned and the size is a multiple
         * of 4 */
        assert((addr & 0x3) == 0);
        assert((n & 0x3) == 0);

        size_t words_left = n / 4;
        while (words_left > 0) {
            ssize_t words_to_read;
            if (words_left > max_word_blk_size) {
                words_to_read = max_word_blk_size;
                words_left -= max_word_blk_size;
            } else {
                words_to_read = words_left;
                words_left = 0;
            }
            ssize_t words_read = uart_read_cmd(bars, addr, dest, words_to_read);
            assert(words_read == words_to_read);
            addr += words_to_read * 4;
            dest = (unsigned char *)dest + words_to_read * 4;
        }

        goto unlock;
    }

    bar_generic_read_v(bars, addr, dest, n, bar4_read);

  unlock:
    pthread_mutex_unlock(&bars->locks[BAR4]);
}

static void bar4_reset(const struct pcie_bars *bars)
{
    bar0_write(bars, PCIE_CFG_REG_TX_CTRL, PCIE_CFG_TX_CTRL_CHANNEL_RST);
}

void device_reset(const struct pcie_bars *bars)
{
    bar0_write(bars, PCIE_CFG_REG_EB_STACON, PCIE_CFG_TX_CTRL_CHANNEL_RST);
    bar4_reset(bars);
}
