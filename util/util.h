/*
 * Copyright (C) 2022 CNPEM (cnpem.br)
 * Author: Érico Nogueira <erico.rolim@lnls.br>
 *
 * Released according to the GNU GPL, version 3 or any later version.
 */

#ifndef UTIL_H
#define UTIL_H

#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>

void clear_and_insert(uint32_t &dest, unsigned value, uint32_t mask);
void insert_bit(uint32_t &dest, bool value, uint32_t mask);
bool get_bit(uint32_t value, uint32_t mask);
unsigned align_extend(unsigned value, unsigned alignment);

template<typename T>
T extract_value(uint32_t value, uint32_t mask);

typedef std::function<int32_t(uint32_t)> sign_extension_fn;
sign_extension_fn &sign_extend_function(unsigned width);

uint32_t float2fixed(double v, unsigned point_pos, bool saturate=true);
double fixed2float(uint32_t v, unsigned point_pos);

std::string list_of_keys(const std::unordered_map<std::string_view, int> &m);

#endif
