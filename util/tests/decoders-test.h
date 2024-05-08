#include <memory>

#include "decoders.h"
#include "pcie-defs.h"
#include "printer.h"
#include "util.h"

static struct pcie_bars bars;
static struct sdb_device_info ref_devinfo;

struct test_regs {
    uint32_t gen, fixed_point;
};

struct TestRegisterDecoder: public RegisterDecoder {
    std::unique_ptr<struct test_regs> regs_storage;
    struct test_regs &regs;

    TestRegisterDecoder():
        RegisterDecoder(::bars, ref_devinfo, {
            PRINTER("INT", "integer", PrinterType::value),
            PRINTER("DOUBLE", "double", PrinterType::value_float),
            PRINTER("ENABLE", "bool", PrinterType::enable),
            PRINTER("C_INT", "integer", PrinterType::value),
            PRINTER("C_DOUBLE", "double", PrinterType::value_float),
            PRINTER("C_DOUBLE_CH1", "double", PrinterType::value_float),
            PRINTER("RF_DOUBLE", "double", PrinterType::value_float),
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

        add_general("RF_DOUBLE", rf_fixed2float(rf_whole_register(regs.fixed_point), 23));
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

    void write_general(const char *name, decoders::data_type value)
    {
        RegisterDecoder::write_general(name, value, read_dest);
    }
};
