#include <catch2/catch_test_macros.hpp>
#include <catch2/benchmark/catch_benchmark_all.hpp>

#include "decoders-test.h"

TEST_CASE("Benchmark", "[decoders-benchmark]")
{
    TestRegisterDecoder dec{};

    dec.decode();
    BENCHMARK("Set general values") {
        return dec.decode();
    };

    BENCHMARK("Get general values - integer") {
        return dec.get_general_data<int32_t>("INT");
    };
    BENCHMARK("Get general values - double") {
        return dec.get_general_data<double>("DOUBLE");
    };

    dec.channel_decode();
    BENCHMARK("Set channel values") {
        return dec.channel_decode();
    };

    BENCHMARK("Get channel values - integer") {
        return dec.get_channel_data<int32_t>("C_INT", 1);
    };
    BENCHMARK("Get channel values - double") {
        return dec.get_channel_data<double>("C_DOUBLE", 1);
    };
}

TEST_CASE("General data storage", "[decoders-test]")
{
    TestRegisterDecoder dec{};
    dec.decode();
    CHECK(dec.get_general_data<int32_t>("INT") == 1);
    CHECK(dec.get_general_data<double>("DOUBLE") == 1.5);
}

TEST_CASE("General data storage - bad variant", "[decoders-test]")
{
    TestRegisterDecoder dec{};
    dec.decode();
    CHECK_THROWS_AS(dec.get_general_data<double>("INT"), std::bad_variant_access);
    CHECK_THROWS_AS(dec.get_general_data<int32_t>("DOUBLE"), std::bad_variant_access);
}

TEST_CASE("Test try_boolean_value", "[decoders-test]")
{
    TestRegisterDecoder dec{};
    dec.test_try_boolean_value();

    CHECK(dec.get_channel_data<int32_t>("ENABLE", 0) == 1);
    CHECK(dec.get_channel_data<int32_t>("ENABLE", 1) == 0);
    CHECK(dec.get_channel_data<int32_t>("ENABLE", 2) == 1);
}

TEST_CASE("Channel data storage", "[decoders-test]")
{
    TestRegisterDecoder dec{};
    dec.channel_decode();
    CHECK(dec.get_channel_data<int32_t>("C_INT", 0) == 1);
    CHECK(dec.get_channel_data<double>("C_DOUBLE", 1) == 1.25);
}

TEST_CASE("Channel data storage - bad variant", "[decoders-test]")
{
    TestRegisterDecoder dec{};
    dec.channel_decode();
    CHECK_THROWS_AS(dec.get_channel_data<double>("C_INT", 0), std::bad_variant_access);
}

TEST_CASE("Channel data number_of_channels", "[decoders-test]")
{
    TestRegisterDecoder dec{};
    /* uninitialized number_of_channels throws */
    CHECK_THROWS_AS(dec.channel_add(1), std::logic_error);
    dec.channel_decode();
    /* adding beyond number_of_channels throws */
    CHECK_THROWS_AS(dec.channel_add(2, true), std::out_of_range);
    /* reading beyond number_of_channels throws */
    CHECK_THROWS_AS(dec.get_channel_data<int32_t>("C_INT", 2), std::out_of_range);
}

TEST_CASE("Channel data single channel", "[decoders-test]")
{
    TestRegisterDecoder dec{};
    dec.channel_decode();
    dec.add_ch1_only_double();

    CHECK(dec.get_channel_data<double>("C_DOUBLE_CH1", 1) == 1.);
    CHECK_THROWS_AS(dec.get_channel_data<double>("C_DOUBLE_CH1", 0), std::out_of_range);
}

TEST_CASE("Generic data API", "[decoders-test]")
{
    TestRegisterDecoder dec{};
    dec.decode();
    dec.channel_decode();

    CHECK(std::get<int32_t>(dec.get_generic_data("INT")) == 1);
    CHECK(std::get<double>(dec.get_generic_data("C_DOUBLE", 1)) == 1.25);
}

TEST_CASE("Print", "[decoders-test]")
{
    TestRegisterDecoder dec{};
    dec.decode();
    dec.channel_decode();
    dec.add_ch1_only_double();

    char *fc;
    size_t fs;
    FILE *f = open_memstream(&fc, &fs);
    dec.print(f, false);
    fclose(f);

    std::string res(fc, fs);
    free(fc);
    std::string eres =
        "INT: 1\n"
        "DOUBLE: 1.500000\n"
        "RF_DOUBLE: 7.750000\n"
        "channel 0:\n"
        "    C_INT: 1\n"
        "    C_DOUBLE: 0.250000\n"
        "channel 1:\n"
        "    C_INT: 2\n"
        "    C_DOUBLE: 1.250000\n"
        "    C_DOUBLE_CH1: 1.000000\n"
    ;
    CHECK(res == eres);
}

TEST_CASE("RegisterField data storage", "[decoders-test]")
{
    TestRegisterDecoder dec{};
    dec.decode();
    CHECK(dec.get_general_data<int32_t>("RF_UINT") == 0xFE);
    CHECK(dec.get_general_data<int32_t>("RF_INT") == -2);
    CHECK(dec.get_general_data<int32_t>("RF_INT16") == -32622);
}

TEST_CASE("RegisterField write_general", "[decoders-test]")
{
    TestRegisterDecoder dec{};
    dec.decode();

    dec.write_general("RF_UINT", 0x43);
    dec.decode();
    CHECK(dec.get_general_data<int32_t>("RF_UINT") == 0x43);

    dec.write_general("RF_INT", 0x54);
    dec.decode();
    CHECK(dec.get_general_data<int32_t>("RF_INT") == 0x54);

    dec.write_general("RF_INT", -0x11);
    dec.decode();
    CHECK(dec.get_general_data<int32_t>("RF_INT") == -0x11);
}

TEST_CASE("RegisterDecoder write_general unsigned", "[decoders-test]")
{
    TestRegisterDecoder dec{};
    dec.decode();

    dec.write_general("RF_UINT", 127);
    dec.decode();
    CHECK(dec.get_general_data<int32_t>("RF_UINT") == 127);

    CHECK_NOTHROW(dec.write_general("RF_UINT", 255));
    dec.decode();
    CHECK(dec.get_general_data<int32_t>("RF_UINT") == 255);

    CHECK_THROWS_AS(dec.write_general("RF_UINT", 256), std::runtime_error);
    CHECK_THROWS_AS(dec.write_general("RF_UINT", -5), std::runtime_error);
}

TEST_CASE("RegisterDecoder write_general signed", "[decoders-test]")
{
    TestRegisterDecoder dec{};
    dec.decode();

    dec.write_general("RF_INT", -128);
    dec.decode();
    CHECK(dec.get_general_data<int32_t>("RF_INT") == -128);

    dec.write_general("RF_INT", 127);
    dec.decode();
    CHECK(dec.get_general_data<int32_t>("RF_INT") == 127);

    CHECK_THROWS_AS(dec.write_general("RF_INT", -129), std::runtime_error);
    CHECK_THROWS_AS(dec.write_general("RF_INT", 128), std::runtime_error);
}

TEST_CASE("RegisterDecoder write_general fixed-point", "[decoders-test]")
{
    TestRegisterDecoder dec{};
    dec.decode();

    dec.write_general("RF_DOUBLE", -5.125);
    dec.decode();
    CHECK(dec.get_general_data<double>("RF_DOUBLE") == -5.125);

    dec.write_general("RF_DOUBLE", 3.5);
    dec.decode();
    CHECK(dec.get_general_data<double>("RF_DOUBLE") == 3.5);
}
