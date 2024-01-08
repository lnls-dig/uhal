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

TEST_CASE("extract_value unsigned", "[bits-test]")
{
    CHECK(extract_value(0xfe00, 0xff00, false) == 254);
    CHECK(extract_value(0xffe00, 0xffff0, false) == 0xffe0);
}

TEST_CASE("extract_value signed", "[bits-test]")
{
    CHECK(extract_value(0xfe00, 0xff00, true) == -2);
    CHECK(extract_value(0xffe00, 0xffff0, true) == -32);
}

TEST_CASE("extract_value signed unhandled width", "[bits-test]")
{
    CHECK_THROWS_AS(extract_value(0, 0x7f00, true), std::logic_error);
}

TEST_CASE("extract_value all bits set", "[bits-test]")
{
    CHECK(extract_value(0xffff, 0xffff, false) == 0xffff);
    CHECK(extract_value(0xffff, 0xffff, true) == -1);

    CHECK((uint32_t)extract_value(0xffffffff, 0xffffffff, false) == 0xffffffffU);
    CHECK(extract_value(0xffffffff, 0xffffffff, true) == -1);
}
