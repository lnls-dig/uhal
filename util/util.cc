/*
 * Copyright (C) 2022 CNPEM (cnpem.br)
 * Author: Érico Nogueira <erico.rolim@lnls.br>
 *
 * Released according to the GNU GPL, version 3 or any later version.
 */

#include <numeric>
#include <stdexcept>

#include "util.h"

void clear_and_insert(uint32_t &dest, unsigned value, uint32_t mask, unsigned shift)
{
    dest &= UINT32_MAX & ~mask;
    dest |= (value << shift) & mask;
}

void insert_bit(uint32_t &dest, bool value, uint32_t mask)
{
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

template<typename Signed, typename Unsigned>
int32_t sign_extend(uint32_t v)
{
    return ((Signed)(Unsigned)v);
}

static sign_extension_fn sign_extend_8 = sign_extend<int8_t, uint8_t>;
static sign_extension_fn sign_extend_16 = sign_extend<int16_t, uint16_t>;
static sign_extension_fn sign_extend_32 = sign_extend<int32_t, uint32_t>;

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
