/*
 * Copyright (C) 2022 CNPEM (cnpem.br)
 * Author: Érico Nogueira <erico.rolim@lnls.br>
 *
 * Released according to the GNU GPL, version 3 or any later version.
 */

#include <bit>
#include <numeric>
#include <ranges>
#include <stdexcept>
#include <type_traits>

#include <cassert>
#include <strings.h>

#include "util.h"

static_assert(__cpp_lib_bitops >= 201907L);

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
    uint32_t max = ((uint64_t)1 << std::popcount(mask)) - 1;
    unsigned shift = std::countr_zero(mask);

    clear_and_insert(dest, value, mask, shift, max, 0);
}

void clear_and_insert_index(uint32_t &dest, uint32_t mask, std::string_view value, std::vector<std::string> value_list)
{
    auto it = std::ranges::find(value_list, value);
    if (it != value_list.end())
        clear_and_insert(dest, it - value_list.begin(), mask);
    else
        throw std::runtime_error("option must be one of " + list_of_keys(value_list));
}

void insert_bit(uint32_t &dest, bool value, uint32_t mask)
{
    assert(std::popcount(mask) == 1);
    if (value)
        dest |= mask;
    else
        dest &= ~mask;
}

bool get_bit(uint32_t value, uint32_t mask)
{
    assert(std::popcount(mask) == 1);
    return value & mask;
}

template<typename T>
T extract_value(uint32_t value, uint32_t mask)
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
template int32_t extract_value(uint32_t, uint32_t);
template int16_t extract_value(uint32_t, uint32_t);
template int8_t extract_value(uint32_t, uint32_t);
template uint32_t extract_value(uint32_t, uint32_t);
template uint16_t extract_value(uint32_t, uint32_t);
template uint8_t extract_value(uint32_t, uint32_t);

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

uint32_t float2fixed(double v, unsigned point_pos, bool saturate)
{
    const uint32_t max_value_rep = 0x7fffffff, min_value_rep = 0x80000000;

    /* we know a 32-bit value fits inside the significand of a double, so these are exact */
    const double max_value = fixed2float(max_value_rep, point_pos);
    const double min_value = fixed2float(min_value_rep, point_pos);

    if (saturate) {
        if (v >= max_value) return max_value_rep;
        if (v <= min_value) return min_value_rep;
    } else {
        if (v > max_value) {
            /* if we are setting to one bit beyond the maximum, allow it */
            if (v == -min_value) return max_value_rep;

            throw std::runtime_error(std::string("value greater than max representable value of ") + std::to_string(max_value));
        }
        if (v < min_value) {
            throw std::runtime_error(std::string("value less than min representable value of ") + std::to_string(min_value));
        }
    }

    /* used for values inside the valid range */
    return v * ((uint64_t)1 << point_pos);
}

double fixed2float(uint32_t v, unsigned point_pos)
{
    return (double)(int32_t)v / ((uint64_t)1 << point_pos);
}

std::string list_of_keys(const std::unordered_map<std::string_view, int> &m)
{
    auto begin = m.begin();
    return std::accumulate(std::next(begin), m.end(), std::string(begin->first),
        [](std::string a, auto b) { return a.append(", ").append(b.first); });
}

std::string list_of_keys(const std::vector<std::string> &v)
{
    auto b = v.begin();
    return std::accumulate(std::next(b), v.end(), *b,
        [](std::string a, std::string b) { return a + ", " + b ; });
}
