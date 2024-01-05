#include <memory>

#include <catch2/catch_test_macros.hpp>
#include <catch2/benchmark/catch_benchmark_all.hpp>

#include "decoders.h"
#include "pcie-defs.h"
#include "printer.h"
#include "util.h"

struct pcie_bars bars;
struct sdb_device_info ref_devinfo;

struct test_regs {
    uint32_t gen, fixed_point;
};

struct TestRegisterDecoder: public RegisterDecoder {
    std::unique_ptr<struct test_regs> regs_storage;
    struct test_regs &regs;

    TestRegisterDecoder():
        RegisterDecoder(bars, ref_devinfo, {
            PRINTER("INT", "integer", PrinterType::value),
            PRINTER("DOUBLE", "double", PrinterType::value_float),
            PRINTER("ENABLE", "bool", PrinterType::enable),
            PRINTER("C_INT", "integer", PrinterType::value),
            PRINTER("C_DOUBLE", "double", PrinterType::value_float),
            PRINTER("C_DOUBLE_CH1", "double", PrinterType::value_float),
        }),
        CONSTRUCTOR_REGS(struct test_regs)
    {
        set_read_dest(regs);

        regs.gen = 0x809200FE;
        regs.fixed_point = float2fixed(7.75, 23);
    }

    void decode() override
    {
        /* General data storage */
        add_general("INT", 1);
        add_general_double("DOUBLE", 1.5);

        /* RegisterField data storage */
        add_general("RF_UINT", rf_extract_value(regs.gen, 0xFF));
        add_general("RF_INT", rf_extract_value(regs.gen, 0xFF, true));
        add_general("RF_INT16", rf_extract_value(regs.gen, 0xFFFF0000, true));
    }

    /* Test try_boolean_value */
    void test_try_boolean_value()
    {
        number_of_channels = 3;
        add_channel("ENABLE", 0, 5);
        add_channel("ENABLE", 1, 0);
        add_channel("ENABLE", 2, 1);
    }

    /* Channel data storage */
    void channel_decode()
    {
        number_of_channels = 2;
        channel_add(*number_of_channels);
    }

    /* Channel data number_of_channels */
    void channel_add(unsigned channels, bool extra_channel = false)
    {
        unsigned i;
        for (i = 0; i < channels; i++) {
            add_channel("C_INT", i, 1 + i);
            add_channel_double("C_DOUBLE", i, .25 + i);
        }
        if (extra_channel) add_channel_double("C_DOUBLE", i+1, 1.);
    }

    /* Channel data single channel & Print */
    void add_ch1_only_double()
    {
        add_channel_double("C_DOUBLE_CH1", 1, 1.);
    }
};

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
