#include <tuple>

#include <catch2/catch_test_macros.hpp>

#include "util.h"

TEST_CASE("Roundtrip conversions - no integer part", "[fixed]")
{
    const unsigned point_pos = 31;
    for (auto v: {.5, .25, .125, .0625, -.5, -1.})
      CHECK(fixed2float(float2fixed(v, point_pos), point_pos) == v);
}

TEST_CASE("Roundtrip conversions - 3 bits integer", "[fixed]")
{
    const unsigned point_pos = 28;
    for (auto v: {1., 2., 7.5, -0.125, -1., -8.})
      CHECK(fixed2float(float2fixed(v, point_pos), point_pos) == v);
}

TEST_CASE("Saturation", "[fixed]")
{
    const unsigned point_pos = 31;
    const double divisor = 1UL << 31, max = 0x7fffffff / divisor, min = -1.;
    std::tuple<double, double> values[] = {{.5, .5}, {1, max}, {1.1, max}, {157, max}, {-1, min}, {-1.005, min}, {-8, min}};
    for (auto &&[v, s]: values)
        CHECK(fixed2float(float2fixed(v, point_pos, true), point_pos) == s);
}

TEST_CASE("Exception with saturation disabled", "[fixed]")
{
    const unsigned point_pos = 31;
    std::tuple<double, bool> values[] = {{.5, false}, {1, /* this case is always allowed to saturate */ false}, {1.1, true}, {157, true}, {-1, false}, {-1.005, true}, {-8, true}};
    for (auto &&[v, t]: values) {
        if (t) {
            CHECK_THROWS_AS(float2fixed(v, point_pos, false), std::runtime_error);
        } else {
            CHECK_NOTHROW(float2fixed(v, point_pos, false));
        }
    }
}
