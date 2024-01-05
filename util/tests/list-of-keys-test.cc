#include <catch2/catch_test_macros.hpp>

#include "util.h"

TEST_CASE("tsl::ordered_map with multiple entries", "[list-of-keys]")
{
    tsl::ordered_map<std::string_view, int> map = {
        {"first", 0},
        {"second", 0},
        {"third", 0},
    };

    CHECK(list_of_keys(map) == "first, second, third");
}

TEST_CASE("tsl::ordered_map with single entry", "[list-of-keys]")
{
    tsl::ordered_map<std::string_view, int> map = {
        {"first", 0},
    };

    CHECK(list_of_keys(map) == "first");
}

TEST_CASE("std::vector with multiple entries", "[list-of-keys]")
{
    std::vector<std::string> vec = {
        "first",
        "second",
        "third",
    };

    CHECK(list_of_keys(vec) == "first, second, third");
}

TEST_CASE("std::vector with single entry", "[list-of-keys]")
{
    std::vector<std::string> vec = {
        "first",
    };

    CHECK(list_of_keys(vec) == "first");
}
