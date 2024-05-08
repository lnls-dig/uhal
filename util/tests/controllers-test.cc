#include <catch2/catch_test_macros.hpp>
#include <catch2/benchmark/catch_benchmark_all.hpp>

#include "controllers.h"

#include "decoders-test.h"

struct TestRegisterDecoderController: public RegisterDecoderController {
    std::unique_ptr<struct test_regs> regs_storage;
    struct test_regs &regs;
    TestRegisterDecoder dec;

    TestRegisterDecoderController():
        RegisterDecoderController(::bars, ref_devinfo, &dec),
        CONSTRUCTOR_REGS(struct test_regs)
    {
        set_read_dest(regs);

        dec.decode();
    }

    void copy_regs_and_decode(TestRegisterDecoder &other_dec)
    {
        memcpy(&other_dec.regs, &regs, read_size);
        other_dec.decode();
    }
};

TEST_CASE("RegisterDecoderController write_general", "[controllers-test]")
{
    TestRegisterDecoderController ctl{};
    TestRegisterDecoder dec{};

    ctl.write_general("RF_UINT", 0x43);
    ctl.copy_regs_and_decode(dec);
    CHECK(dec.get_general_data<int32_t>("RF_UINT") == 0x43);

    ctl.write_general("RF_INT", 0x54);
    ctl.copy_regs_and_decode(dec);
    CHECK(dec.get_general_data<int32_t>("RF_INT") == 0x54);

    ctl.write_general("RF_INT", -0x11);
    ctl.copy_regs_and_decode(dec);
    CHECK(dec.get_general_data<int32_t>("RF_INT") == -0x11);
}

TEST_CASE("RegisterDecoderController write_general fixed-point", "[controllers-test][!shouldfail]")
{
    TestRegisterDecoderController ctl{};
    TestRegisterDecoder dec{};

    CHECK_NOTHROW(ctl.write_general("RF_DOUBLE", -5.125));
    ctl.copy_regs_and_decode(dec);
    CHECK(dec.get_general_data<double>("RF_DOUBLE") == -5.125);

    CHECK_NOTHROW(ctl.write_general("RF_DOUBLE", 3.5));
    ctl.copy_regs_and_decode(dec);
    CHECK(dec.get_general_data<double>("RF_DOUBLE") == 3.5);
}
