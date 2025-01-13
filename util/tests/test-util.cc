#include <sys/mman.h>
#include <stdexcept>

#include "test-util.h"

void dummy_dev_open(struct pcie_bars &bars)
{
    bars.last_bar4_page = -1;

    pthread_mutexattr_t mattr;
    if (pthread_mutexattr_init(&mattr) ||
        pthread_mutexattr_settype(&mattr, PTHREAD_MUTEX_RECURSIVE))
        throw std::runtime_error("couldn't set mutexattr as recursive");

    for (auto &lock: bars.locks) {
        if (pthread_mutex_init(&lock, &mattr))
            throw std::runtime_error("couldn't initialize mutex");
    }

    auto allocate_bar = [](volatile void *&ptr, size_t &size, size_t desired_size) {
        size = desired_size;
        ptr = mmap(0, desired_size, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
    };

    /* same sizes as our devices */
    allocate_bar(bars.bar0, bars.sizes[0], 2048);
    allocate_bar(bars.bar2, bars.sizes[1], 1048576);
    allocate_bar(bars.bar4, bars.sizes[2], 524288);

    bars.fserport = nullptr;
}
