#include <catch2/catch_test_macros.hpp>

#include "util-bits.h"

TEST_CASE("template extract_value unsigned", "[bits-test]")
{
    CHECK(extract_value<uint8_t>(0xfe00, 0xff00) == 254);
    CHECK(extract_value<uint16_t>(0x7fe00, 0xffff0) == 0x7fe0);
    CHECK(extract_value<uint32_t>(0x7fe00, 0xffff0) == 0x7fe0);
}

TEST_CASE("template extract_value signed", "[bits-test]")
{
    CHECK(extract_value<int8_t>(0xfe00, 0xff00) == -2);
    CHECK(extract_value<int16_t>(0x7fe00, 0xffff0) == 0x7fe0);
    CHECK(extract_value<int16_t>(0xffe00, 0xffff0) == -32);
}
