#include <catch2/catch_test_macros.hpp>

#include "util-bits.h"

TEST_CASE("clear_and_insert unsigned basic", "[bits-test]")
{
    uint32_t reg = UINT32_MAX;
    clear_and_insert(reg, 0x49U, 0x7f00);
    CHECK(reg == 0xffffc9ff);

    reg = 0;
    clear_and_insert(reg, 0x49U, 0x7f00);
    CHECK(reg == 0x4900);
}

TEST_CASE("clear_and_insert signed basic", "[bits-test]")
{
    uint32_t reg;

    /* using negative value */
    reg = UINT32_MAX;
    clear_and_insert(reg, -0x2a70, 0x00ffff00); /* equivalent to 0xd590 */
    CHECK(reg == 0xffd590ff);

    /* using cast to unsigned */
    reg = UINT32_MAX;
    clear_and_insert(reg, (uint16_t)-0x2a70, 0x00ffff00);
    CHECK(reg == 0xffd590ff);
}

const uint32_t range_mask = 0x1ff000;

TEST_CASE("clear_and_insert unsigned range", "[bits-test]")
{
    uint32_t reg = 0;
    CHECK_THROWS_AS(clear_and_insert(reg, 1000U, range_mask), std::runtime_error);
    CHECK_THROWS_AS(clear_and_insert(reg, -1000, range_mask), std::runtime_error);

    CHECK_THROWS_AS(clear_and_insert(reg, 512U, range_mask), std::runtime_error);
    CHECK_THROWS_AS(clear_and_insert(reg, -511, range_mask), std::runtime_error);

    CHECK_NOTHROW(clear_and_insert(reg, 511U, range_mask));
    CHECK_NOTHROW(clear_and_insert(reg, 255U, range_mask));
    CHECK_NOTHROW(clear_and_insert(reg, 0U, range_mask));
}

TEST_CASE("clear_and_insert signed range", "[bits-test]")
{
    uint32_t reg = 0;
    CHECK_THROWS_AS(clear_and_insert(reg, 1000, range_mask), std::runtime_error);
    CHECK_THROWS_AS(clear_and_insert(reg, -1000, range_mask), std::runtime_error);

    CHECK_THROWS_AS(clear_and_insert(reg, 256, range_mask), std::runtime_error);
    CHECK_THROWS_AS(clear_and_insert(reg, -257, range_mask), std::runtime_error);

    CHECK_NOTHROW(clear_and_insert(reg, 255, range_mask));
    CHECK_NOTHROW(clear_and_insert(reg, 0, range_mask));
    CHECK_NOTHROW(clear_and_insert(reg, -256, range_mask));
}

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
