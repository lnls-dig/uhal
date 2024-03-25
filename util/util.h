#ifndef UTIL_H
#define UTIL_H

#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <vector>

#include <tsl/ordered_map.h>

#include "util-bits.h"

size_t get_index(std::string_view value, const std::vector<std::string> &value_list);
void clear_and_insert_index(uint32_t &dest, uint32_t mask, std::string_view value, const std::vector<std::string> &value_list);

uint32_t float2fixed(double v, unsigned point_pos, bool saturate=true);
double fixed2float(uint32_t v, unsigned point_pos);

std::string list_of_keys(const tsl::ordered_map<std::string_view, int> &);
std::string list_of_keys(const std::vector<std::string> &);

#endif
