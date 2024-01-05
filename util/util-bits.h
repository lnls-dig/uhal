#ifndef UTIL_BITS_H
#define UTIL_BITS_H

#include <bit>
#include <cassert>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <type_traits>

static_assert(__cpp_lib_bitops >= 201907L);

static inline void clear_and_insert(uint32_t &dest, unsigned value, uint32_t mask, unsigned shift, uint32_t max, uint32_t min)
{
    if (value < min)
        throw std::runtime_error("value " + std::to_string(value) + " less than min (" + std::to_string(min) + ")");
    if (value > max)
        throw std::runtime_error("value " + std::to_string(value) + " greater than max (" + std::to_string(max) + ")");

    dest &= UINT32_MAX & ~mask;
    dest |= (value << shift) & mask;
}

/* TODO: template this as well */
static inline void clear_and_insert(uint32_t &dest, unsigned value, uint32_t mask)
{
    uint32_t max = ((uint64_t)1 << std::popcount(mask)) - 1;
    unsigned shift = std::countr_zero(mask);

    clear_and_insert(dest, value, mask, shift, max, 0);
}

static inline void insert_bit(uint32_t &dest, bool value, uint32_t mask)
{
    assert(std::popcount(mask) == 1);
    if (value)
        dest |= mask;
    else
        dest &= ~mask;
}

static inline bool get_bit(uint32_t value, uint32_t mask)
{
    assert(std::popcount(mask) == 1);
    return value & mask;
}

template<typename T>
static inline T extract_value(uint32_t value, uint32_t mask)
{
    unsigned shift = std::countr_zero(mask);
    uint32_t intermediary = (value & mask) >> shift;

    if constexpr (std::is_signed<T>()) {
        typedef typename std::make_unsigned<T>::type U;
        return (T)(U)intermediary;
    } else {
        return (T)intermediary;
    }
}

#endif
