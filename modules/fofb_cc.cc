/*
 * Copyright (C) 2022 CNPEM (cnpem.br)
 * Author: Érico Nogueira <erico.rolim@lnls.br>
 *
 * Released according to the GNU GPL, version 3 or any later version.
 */

#include <cmath>
#include <stdexcept>

#include "pcie.h"
#include "printer.h"
#include "util.h"
#include "modules/fofb_cc.h"

namespace fofb_cc {

#include "hw/wb_fofb_cc_regs.h"

static_assert(FOFB_CC_REGS_RAM_REG == offsetof(fofb_cc_regs, ram_reg));

#define FOFB_CC_OFFS_SHIFT 8

/* Read/Write RAM-based registers */
#define FOFB_CC_REGS_CTL_OFFS    (0x0 << FOFB_CC_OFFS_SHIFT)
#define BPM_ID                   (FOFB_CC_REGS_CTL_OFFS + 0x00)
#define TIME_FRAME_LEN           (FOFB_CC_REGS_CTL_OFFS + 0x01)
#define MGT_POWERDOWN            (FOFB_CC_REGS_CTL_OFFS + 0x02)
#define MGT_LOOPBACK             (FOFB_CC_REGS_CTL_OFFS + 0x03)
#define TIME_FRAME_DLY           (FOFB_CC_REGS_CTL_OFFS + 0x04)
#define RX_POLARITY              (FOFB_CC_REGS_CTL_OFFS + 0x08)
#define PAYLOAD_SEL              (FOFB_CC_REGS_CTL_OFFS + 0x09)
#define FOFB_DATA_SEL            (FOFB_CC_REGS_CTL_OFFS + 0x0A)

/* Read-Only RAM-based registers */
#define FOFB_CC_REGS_STA_OFFS    (0x3 << FOFB_CC_OFFS_SHIFT)
#define FIRMWARE_VER             (FOFB_CC_REGS_STA_OFFS + 0xFF)
#define SYS_STATUS               (FOFB_CC_REGS_STA_OFFS + 0x00)
#define LINK_PARTNER_1           (FOFB_CC_REGS_STA_OFFS + 0x01)
#define LINK_PARTNER_2           (FOFB_CC_REGS_STA_OFFS + 0x02)
#define LINK_PARTNER_3           (FOFB_CC_REGS_STA_OFFS + 0x03)
#define LINK_PARTNER_4           (FOFB_CC_REGS_STA_OFFS + 0x04)
#define LINK_PARTNER_5           (FOFB_CC_REGS_STA_OFFS + 0x05)
#define LINK_PARTNER_6           (FOFB_CC_REGS_STA_OFFS + 0x06)
#define LINK_PARTNER_7           (FOFB_CC_REGS_STA_OFFS + 0x07)
#define LINK_PARTNER_8           (FOFB_CC_REGS_STA_OFFS + 0x08)
#define LINK_UP                  (FOFB_CC_REGS_STA_OFFS + 0x09)
#define TIME_FRAME_CNT           (FOFB_CC_REGS_STA_OFFS + 0x0A)
#define HARD_ERR_CNT_1           (FOFB_CC_REGS_STA_OFFS + 0x0B)
/* ... goes until 8 */
#define SOFT_ERR_CNT_1           (FOFB_CC_REGS_STA_OFFS + 0x13)
/* ... goes until 8 */
#define FRAME_ERR_CNT_1          (FOFB_CC_REGS_STA_OFFS + 0x1B)
/* ... goes until 8 */
#define RX_PCK_CNT_1             (FOFB_CC_REGS_STA_OFFS + 0x23)
/* ... goes until 8 */
#define TX_PCK_CNT_1             (FOFB_CC_REGS_STA_OFFS + 0x2B)
/* ... goes until 8 */
#define FOD_PROCESS_TIME         (FOFB_CC_REGS_STA_OFFS + 0x33)
#define BPM_CNT                  (FOFB_CC_REGS_STA_OFFS + 0x34)
#define RX_MAXCOUNT_1            (FOFB_CC_REGS_STA_OFFS + 0x3B)
#define RX_MAXCOUNT_2            (FOFB_CC_REGS_STA_OFFS + 0x3C)
#define TX_MAXCOUNT_1            (FOFB_CC_REGS_STA_OFFS + 0x3D)
#define TX_MAXCOUNT_2            (FOFB_CC_REGS_STA_OFFS + 0x3E)

#define BPM_ID_MASK              0x1ff /* 9 bits */

namespace {
    const unsigned NUMBER_OF_CHANS = 8;

    constexpr uint64_t DLS_VENDORID = 0x1000000000000d15;
    constexpr unsigned FOFB_CC_DEVID = 0x4a1df147;
    struct sdb_device_info ref_devinfo = {
        .vendor_id = DLS_VENDORID,
        .device_id = FOFB_CC_DEVID,
        .abi_ver_major = 1
    };
}

Core::Core(struct pcie_bars &bars):
    RegisterDecoder(bars, ref_devinfo, {
        /* normal wishbone registers */
        PRINTER("CC_ENABLE", "Enable CC module", PrinterType::enable),
        PRINTER("TFS_OVERRIDE", "Timeframe start override", PrinterType::boolean, "override, use external signal", "normal, use internal signal"),
        PRINTER("TOA_RD_EN", "Enable Time of Arrival module for reading", PrinterType::enable),
        PRINTER("TOA_DATA", "Time of Arrival data", PrinterType::value),
        PRINTER("RCB_RD_EN", "Enable Received Buffer module for reading", PrinterType::enable),
        PRINTER("RCB_DATA", "Received Buffer data", PrinterType::value),
        /* RAM registers (r/w) */
        PRINTER("BPM_ID", "BPM ID for sending packets", PrinterType::value),
        PRINTER("TIME_FRAME_LEN", "Time frame length in clock cycles", PrinterType::value),
        PRINTER("MGT_POWERDOWN", "Transceiver powerdown", PrinterType::enable),
        PRINTER("MGT_LOOPBACK", "Transceiver loopback", PrinterType::enable),
        PRINTER("TIME_FRAME_DLY", "Time frame delay", PrinterType::value),
        PRINTER("RX_POLARITY", "Receiver polarity", PrinterType::value),
        PRINTER("PAYLOAD_SEL", "Payload selection", PrinterType::value),
        PRINTER("FOFB_DATA_SEL", "FOFB data selection", PrinterType::value),
        /* RAM registers (ro) */
        PRINTER("FIRMWARE_VER", "Firmware version", PrinterType::value_hex),
        PRINTER("SYS_STATUS", "System status", PrinterType::value),
        PRINTER("LINK_UP", "Link status", PrinterType::enable),
        PRINTER("TIME_FRAME_CNT", "Total time frame count", PrinterType::value),
        PRINTER("FOD_PROCESS_TIME", "Forward or Discard process time", PrinterType::value),
        PRINTER("BPM_CNT", "BPM devices count", PrinterType::value),
        /* RAM registers (ro) - channels */
        PRINTER("LINK_PARTNER", "Link partner ID", PrinterType::value),
        PRINTER("HARD_ERR_CNT", "Hard error count", PrinterType::value),
        PRINTER("SOFT_ERR_CNT", "Soft error count", PrinterType::value),
        PRINTER("FRAME_ERR_CNT", "Frame error count", PrinterType::value),
        PRINTER("RX_PCK_CNT", "Received packet count", PrinterType::value),
        PRINTER("TX_PCK_CNT", "Transmitted packet count", PrinterType::value),
    }),
    CONSTRUCTOR_REGS(struct fofb_cc_regs)
{
    set_read_dest(regs);
}
Core::~Core() = default;

void Core::read()
{
    bar4_read_v(&bars, devinfo.start_addr, &regs, offsetof(fofb_cc_regs, __padding_0));
    bar4_read_v(&bars, devinfo.start_addr + offsetof(fofb_cc_regs, ram_reg), &regs.ram_reg, (FOFB_DATA_SEL+1) * 4);
    bar4_read_v(
        &bars,
        devinfo.start_addr + offsetof(fofb_cc_regs, ram_reg[FOFB_CC_REGS_STA_OFFS]),
        &regs.ram_reg[FOFB_CC_REGS_STA_OFFS],
        (FIRMWARE_VER + 1) * 4);
}

void Core::decode()
{
    uint32_t t;

    /* TODO: use get_bit() */
    t = regs.cfg_val;
    add_general("CC_ENABLE", t & FOFB_CC_REGS_CFG_VAL_CC_ENABLE);
    add_general("TFS_OVERRIDE", t & FOFB_CC_REGS_CFG_VAL_TFS_OVERRIDE);

    t = regs.toa_ctl;
    add_general("TOA_RD_EN", t & FOFB_CC_REGS_TOA_CTL_RD_EN);
    add_general("TOA_DATA", regs.toa_data);

    t = regs.rcb_ctl;
    add_general("RCB_RD_EN", t & FOFB_CC_REGS_RCB_CTL_RD_EN);
    add_general("RCB_DATA", regs.rcb_data);

    auto ram_reg = [this](size_t offset) -> uint32_t {
        return regs.ram_reg[offset].data;
    };

    add_general("BPM_ID", ram_reg(BPM_ID) & BPM_ID_MASK);
    add_general("TIME_FRAME_LEN", ram_reg(TIME_FRAME_LEN));
    add_general("MGT_POWERDOWN", ram_reg(MGT_POWERDOWN));
    add_general("MGT_LOOPBACK", ram_reg(MGT_LOOPBACK));
    add_general("TIME_FRAME_DLY", ram_reg(TIME_FRAME_DLY));
    add_general("RX_POLARITY", ram_reg(RX_POLARITY));
    add_general("PAYLOAD_SEL", ram_reg(PAYLOAD_SEL));
    add_general("FOFB_DATA_SEL", ram_reg(FOFB_DATA_SEL));

    add_general("FIRMWARE_VER", ram_reg(FIRMWARE_VER));
    add_general("SYS_STATUS", ram_reg(SYS_STATUS));
    add_general("LINK_UP", ram_reg(LINK_UP));
    add_general("TIME_FRAME_CNT", ram_reg(TIME_FRAME_CNT));
    add_general("FOD_PROCESS_TIME", ram_reg(FOD_PROCESS_TIME));
    add_general("BPM_CNT", ram_reg(BPM_CNT));

    number_of_channels = NUMBER_OF_CHANS;

    for (unsigned i = 0; i < *number_of_channels; i++) {
        add_channel("LINK_PARTNER", i, ram_reg(LINK_PARTNER_1 + i));
        add_channel("HARD_ERR_CNT", i, ram_reg(HARD_ERR_CNT_1 + i));
        add_channel("SOFT_ERR_CNT", i, ram_reg(SOFT_ERR_CNT_1 + i));
        add_channel("FRAME_ERR_CNT", i, ram_reg(FRAME_ERR_CNT_1 + i));
        add_channel("RX_PCK_CNT", i, ram_reg(RX_PCK_CNT_1 + i));
        add_channel("TX_PCK_CNT", i, ram_reg(TX_PCK_CNT_1 + i));

        data_order_done = true;
    }
}

Controller::Controller(struct pcie_bars &bars):
    RegisterController(bars, ref_devinfo),
    CONSTRUCTOR_REGS(struct fofb_cc_regs)
{
    set_read_dest(regs);
}
Controller::~Controller() = default;

void Controller::get_internal_values()
{
    bar4_read_v(&bars, addr, &regs, sizeof regs);
}

void Controller::encode_params()
{
    get_internal_values();

    uint32_t *target_reg;
    auto insert_cfg = [&target_reg](auto option, uint32_t mask) {
        if (option)
            insert_bit(*target_reg, *option, mask);
    };

    target_reg = &regs.cfg_val;
    insert_cfg(err_clr, FOFB_CC_REGS_CFG_VAL_ERR_CLR);
    insert_cfg(cc_enable, FOFB_CC_REGS_CFG_VAL_CC_ENABLE);
    insert_cfg(tfs_override, FOFB_CC_REGS_CFG_VAL_TFS_OVERRIDE);

    target_reg = &regs.toa_ctl;
    insert_cfg(toa_rd_en, FOFB_CC_REGS_TOA_CTL_RD_EN);
    insert_cfg(toa_rd_str, FOFB_CC_REGS_TOA_CTL_RD_STR);

    target_reg = &regs.rcb_ctl;
    insert_cfg(rcb_rd_en, FOFB_CC_REGS_RCB_CTL_RD_EN);
    insert_cfg(rcb_rd_str, FOFB_CC_REGS_RCB_CTL_RD_STR);

    auto ram_reg = [this](size_t offset) -> uint32_t & {
        return regs.ram_reg[offset].data;
    };

    if (bpm_id)
        clear_and_insert(ram_reg(BPM_ID), *bpm_id, BPM_ID_MASK);

    auto insert_val = [&ram_reg](auto option, size_t offset) {
        if (option)
            clear_and_insert(ram_reg(offset), *option, UINT32_MAX);
    };

    insert_val(time_frame_len, TIME_FRAME_LEN);
    insert_val(mgt_powerdown, MGT_POWERDOWN);
    insert_val(mgt_loopback, MGT_LOOPBACK);
    insert_val(time_frame_delay, TIME_FRAME_DLY);
    insert_val(rx_polarity, RX_POLARITY);
    insert_val(payload_sel, PAYLOAD_SEL);
    insert_val(fofb_data_sel, FOFB_DATA_SEL);
}

void Controller::write_params()
{
    encode_params();

    bar4_write_v(&bars, addr, &regs, sizeof regs);

    /* clear these values after writing so they aren't copied into the next write */
    err_clr = toa_rd_str = rcb_rd_str = std::nullopt;

    /* apply any changes */
    bar4_write(
        &bars, addr + FOFB_CC_REGS_CFG_VAL,
        regs.cfg_val | FOFB_CC_REGS_CFG_VAL_ACT_PART);
}

} /* namespace fofb_cc */
