/*
 * Copyright (C) 2022 CNPEM (cnpem.br)
 * Author: Érico Nogueira <erico.rolim@lnls.br>
 *
 * Released according to the GNU GPL, version 3 or any later version.
 */

#include <numeric>
#include <ranges>
#include <stdexcept>

#include <cassert>
#include <strings.h>

#include "util.h"

size_t get_index(std::string_view value, const std::vector<std::string> &value_list)
{
    auto it = std::ranges::find(value_list, value);
    if (it != value_list.end())
        return it - value_list.begin();
    else
        throw std::runtime_error("option must be one of " + list_of_keys(value_list));
}

void clear_and_insert_index(uint32_t &dest, uint32_t mask, std::string_view value, const std::vector<std::string> &value_list)
{
    clear_and_insert(dest, get_index(value, value_list), mask);
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

/* XXX: replace with C++23's std::ranges::fold_left_first */

std::string list_of_keys(const tsl::ordered_map<std::string_view, int> &m)
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
