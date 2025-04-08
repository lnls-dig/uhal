#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>

#include "defer.h"
#include "pcie-open.h"
#include "test-util.h"

int main()
{
    struct pcie_bars bars;
    dummy_dev_open(bars);
    defer _(nullptr, [&bars](...){dev_close(bars);});

    const size_t s = bars.sizes[1] / sizeof(uint32_t);
    uint32_t *p = reinterpret_cast<uint32_t *>(const_cast<void *>(bars.bar2));
    for (size_t i = 0; i < s; i++)
        p[i] = i;

    const size_t dest_s = bars.sizes[1] * 16;
    void *dest = malloc(dest_s);

    auto compare = [&bars, dest, dest_s, s](size_t bar_off, size_t dest_off, size_t n) {
        while (n) {
            size_t to_read;
            if (n < s*4 - bar_off) {
                to_read = n;
            } else {
                to_read = s*4 - bar_off;
            }

            printf("checking from %p and %p, %zu bytes\n",
                (void *)((unsigned char *)bars.bar2 + bar_off),
                (void *)((unsigned char *)dest + dest_off),
                to_read);
            if (memcmp((unsigned char *)bars.bar2 + bar_off, (unsigned char *)dest + dest_off, to_read)) {
                std::cerr << "memcmp failed" << std::endl;
                exit(1);
            }

            n -= to_read;
        }

        memset(dest, 0, dest_s);
    };

    bar2_read_v(&bars, 0, dest, bars.sizes[1]);
    compare(0, 0, bars.sizes[1]);

    const size_t offs[] = {4096, 2048, 1024, 768, 512, 256, 128, 64, 32, 16, /*FIXME: 8, 4, */ 0};
    for (auto off: offs) {
        bar2_read_v(&bars, off, dest, bars.sizes[1] - off);
        compare(off, 0, bars.sizes[1] - off);
    }

    for (auto off: offs) {
        bar2_read_v(&bars, off, dest, bars.sizes[1] * 2 + 128);
        compare(off, 0, bars.sizes[1] * 2 + 128);
    }

    free(dest);
}
