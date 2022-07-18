/*
 * Copyright (C) 2022 CNPEM (cnpem.br)
 * Author: Érico Nogueira <erico.rolim@lnls.br>
 *
 * Released according to the GNU GPL, version 3 or any later version.
 */

#include <numeric>
#include <stdexcept>
#include <type_traits>

#include <cassert>
#include <strings.h>

#include "util.h"

/* XXX: replace with C++20's <bit> */

static inline unsigned bit_popcount(uint32_t x)
{
#ifdef __GNUC__
    return __builtin_popcount(x);
#else
    /* From: https://graphics.stanford.edu/~seander/bithacks.html */
    uint32_t v = x;
    unsigned c;
    for (c = 0; v; c++)
        v &= v - 1; /* clear the least significant bit set */
    return c;
#endif
}

static inline unsigned bit_countr_zero(uint32_t x)
{
    return ffs(x) - 1;
}

static void clear_and_insert(uint32_t &dest, unsigned value, uint32_t mask, unsigned shift, uint32_t max, uint32_t min)
{
    if (value < min)
        throw std::runtime_error("value " + std::to_string(value) + " less than min (" + std::to_string(min) + ")");
    if (value > max)
        throw std::runtime_error("value " + std::to_string(value) + " greater than max (" + std::to_string(max) + ")");

    dest &= UINT32_MAX & ~mask;
    dest |= (value << shift) & mask;
}

/* TODO: template this as well */
void clear_and_insert(uint32_t &dest, unsigned value, uint32_t mask)
{
    uint32_t max = (1 << bit_popcount(mask)) - 1;
    unsigned shift = bit_countr_zero(mask);

    clear_and_insert(dest, value, mask, shift, max, 0);
}

void insert_bit(uint32_t &dest, bool value, uint32_t mask)
{
    assert(bit_popcount(mask) == 1);
    if (value)
        dest |= mask;
    else
        dest &= ~mask;
}

unsigned align_extend(unsigned value, unsigned alignment)
{
    unsigned extra = value % alignment;
    if (extra)
        return value + (alignment - extra);
    else
        return value;
}

template<typename T>
T extract_value(uint32_t value, uint32_t mask)
{
    unsigned shift = bit_countr_zero(mask);
    uint32_t intermediary = (value & mask) >> shift;

    if constexpr (std::is_signed<T>()) {
        typedef typename std::make_unsigned<T>::type U;
        return (T)(U)intermediary;
    } else {
        return (T)intermediary;
    }
}
template int32_t extract_value(uint32_t, uint32_t);
template int16_t extract_value(uint32_t, uint32_t);
template uint32_t extract_value(uint32_t, uint32_t);
template uint16_t extract_value(uint32_t, uint32_t);

template<typename Signed>
int32_t sign_extend(uint32_t v)
{
    typedef typename std::make_unsigned<Signed>::type Unsigned;
    return (Signed)(Unsigned)v;
}

static sign_extension_fn sign_extend_8 = sign_extend<int8_t>;
static sign_extension_fn sign_extend_16 = sign_extend<int16_t>;
static sign_extension_fn sign_extend_32 = sign_extend<int32_t>;

sign_extension_fn &sign_extend_function(unsigned width)
{
    switch (width) {
        case 8:
            return sign_extend_8;
        case 16:
            return sign_extend_16;
        case 32:
            return sign_extend_32;
        default:
            throw std::logic_error("invalid width should have been caught elsewhere");
    }
}

std::string list_of_keys(const std::unordered_map<std::string, int> &m)
{
    auto b = m.begin();
    return std::accumulate(std::next(b), m.end(), b->first,
        [](std::string a, std::pair<std::string, int> b) {return a + ", " + b.first;});
}
