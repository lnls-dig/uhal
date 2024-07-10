#include <arpa/inet.h>
#include <cstring>

#include "printer.h"
#include "util.h"
#include "modules/spi.h"

namespace spi {

#include "hw/wb_spi_regs.h"

struct spi {
    uint32_t x0;
    uint32_t x1;
    uint32_t x2;
    uint32_t x3;
    uint32_t ctrl;
    uint32_t divider;
    uint32_t ss;
    uint32_t cfg_bidir;
    uint32_t rx0_single;
    uint32_t rx1_single;
    uint32_t rx2_single;
    uint32_t rx3_single;
};

static_assert(SPI_PROTO_REG_RX3_SINGLE == offsetof(struct spi, rx3_single));

namespace {
    constexpr uint64_t CERN_VENDORID = 0x000000000000ce42;
    constexpr unsigned SPI_DEVID = 0xe503947e;
    struct sdb_device_info ref_devinfo = {
        .vendor_id = CERN_VENDORID,
        .device_id = SPI_DEVID,
        .abi_ver_major = 1
    };
}

Core::Core(struct pcie_bars &bars):
    RegisterDecoder(bars, ref_devinfo, {
        PRINTER("X", "RX/TX registers", PrinterType::value_hex),
        PRINTER("CHARLEN", "size of message in bits (the max of 128 bits is encoded as 0)", PrinterType::value),
        PRINTER("BSY", "Busy flag", PrinterType::boolean),
        PRINTER("RXNEG", "", PrinterType::boolean),
        PRINTER("TXNEG", "", PrinterType::boolean),
        PRINTER("LSB", "", PrinterType::boolean),
        PRINTER("IE", "", PrinterType::boolean),
        PRINTER("ASS", "", PrinterType::boolean),
        PRINTER("DIVIDER", "", PrinterType::value),
        PRINTER("SS", "Slave select", PrinterType::value_hex),
        PRINTER("BIDIR_CHARLEN", "", PrinterType::value),
        PRINTER("BIDIR_EN", "", PrinterType::enable),
        PRINTER("RX_SINGLE", "", PrinterType::value_hex),
    }),
    CONSTRUCTOR_REGS(struct spi)
{
    set_read_dest(regs);

    number_of_channels = 4;
}
Core::~Core() = default;

void Core::decode()
{
    add_channel("X", 0, rf_whole_register(regs.x0));
    add_channel("X", 1, rf_whole_register(regs.x1));
    add_channel("X", 2, rf_whole_register(regs.x2));
    add_channel("X", 3, rf_whole_register(regs.x3));

    add_general("CHARLEN", rf_extract_value(regs.ctrl, SPI_PROTO_CTRL_CHARLEN_MASK));
    add_general("BSY", rf_get_bit(regs.ctrl, SPI_PROTO_CTRL_BSY));
    add_general("RXNEG", rf_get_bit(regs.ctrl, SPI_PROTO_CTRL_RXNEG));
    add_general("TXNEG", rf_get_bit(regs.ctrl, SPI_PROTO_CTRL_TXNEG));
    add_general("LSB", rf_get_bit(regs.ctrl, SPI_PROTO_CTRL_LSB));
    add_general("IE", rf_get_bit(regs.ctrl, SPI_PROTO_CTRL_IE));
    add_general("ASS", rf_get_bit(regs.ctrl, SPI_PROTO_CTRL_ASS));

    add_general("DIVIDER", rf_extract_value(regs.divider, SPI_PROTO_DIVIDER_MASK));

    add_general("SS", rf_extract_value(regs.ss, SPI_PROTO_SS_MASK));

    add_general("BIDIR_CHARLEN", rf_extract_value(regs.cfg_bidir, SPI_PROTO_CFG_BIDIR_CHARLEN_MASK));
    add_general("BIDIR_EN", rf_get_bit(regs.cfg_bidir, SPI_PROTO_CFG_BIDIR_EN));

    add_channel("RX_SINGLE", 0, rf_whole_register(regs.rx0_single));
    add_channel("RX_SINGLE", 1, rf_whole_register(regs.rx1_single));
    add_channel("RX_SINGLE", 2, rf_whole_register(regs.rx2_single));
    add_channel("RX_SINGLE", 3, rf_whole_register(regs.rx3_single));
}

Controller::Controller(struct pcie_bars &bars):
    RegisterDecoderController(bars, ref_devinfo, &dec),
    CONSTRUCTOR_REGS(struct spi),
    dec(bars)
{
    set_read_dest(regs);
}
Controller::~Controller() = default;

void Controller::set_devinfo_callback()
{
    set_defaults();
}

int32_t Controller::get_divider(int32_t sys_freq, int32_t spi_freq)
{
    return sys_freq / (2 * spi_freq) - 1;
}

void Controller::set_defaults()
{
    write_general("DIVIDER", get_divider(100'000'000, 100'000));
    write_general("TXNEG", 1);
    write_general("ASS", 1);

    write_params();
}

bool Controller::write_read_data(const unsigned char *wdata, size_t wsize, unsigned char *rdata, size_t rsize, Channel slave)
{
    if (wsize + rsize > 16)
        throw std::logic_error("unsupported wsize + rsize");
    if (wsize + rsize > 4)
        throw std::logic_error("TODO: deal with commands spanning more than one register");
    if (rsize > 1)
        throw std::logic_error("TODO: deal with reading more than one byte");

    int32_t charlen = (wsize + rsize) * 8;
    if (charlen == 128)
        charlen = 0;
    write_general("CHARLEN", charlen);

    write_general("SS", 1 << slave.channel);

    /* TODO: deal with commands spanning more than one register */
    regs.x0 = 0;
    memcpy(&regs.x0, wdata, wsize);
    /* the bytes were in network order (MSB first), and were copied into x0, so
     * we need to revert that */
    regs.x0 = ntohl(regs.x0);
    /* now the bytes are at the register's MSB, so we need to shift them to LSB */
    regs.x0 >>= (4 - wsize) * 8;
    /* finally, we need to make room for the response bytes */
    regs.x0 <<= rsize * 8;

    write_params();

    write_general("BSY", 1);
    write_params();
    write_general("BSY", 0);

    do {
        dec.get_data();
    } while (dec.get_general_data<int32_t>("BSY"));

    /* TODO: deal with reading more than one byte */
    if (rsize)
        *rdata = (uint32_t)dec.get_channel_data<int32_t>("RX_SINGLE", 0);

    return true;
}

} /* namespace spi */
