/*
 * Copyright (C) 2023 CNPEM (cnpem.br)
 * Author: Érico Nogueira <erico.rolim@lnls.br>
 *
 * Released according to the GNU GPL, version 3 or any later version.
 */

#include <cmath>
#include <stdexcept>

#include "pcie.h"
#include "printer.h"
#include "util.h"
#include "modules/bpm_swap.h"

namespace bpm_swap {

#include "hw/wb_bpm_swap_regs.h"

namespace {
    constexpr unsigned BPM_SWAP_DEVID = 0x12897592;
    struct sdb_device_info ref_devinfo = {
        .vendor_id = LNLS_VENDORID,
        .device_id = BPM_SWAP_DEVID,
        .abi_ver_major = 1
    };
}

struct bpm_swap_regs {
    uint32_t ctrl, dly;
};

Core::Core(struct pcie_bars &bars):
    RegisterDecoder(bars, ref_devinfo, {
        PRINTER("MODE", "Operation mode of first pair", PrinterType::value),
        PRINTER("DIV_F_CNT_EN", "Swap phase sync enable", PrinterType::enable),
        PRINTER("DIV_F", "Swap divisor", PrinterType::value),
        PRINTER("DLY", "Swap delay", PrinterType::value),
    }),
    CONSTRUCTOR_REGS(struct bpm_swap_regs)
{
    set_read_dest(regs);
}
Core::~Core() = default;

void Core::decode()
{
    uint32_t t;

    t = regs.ctrl;
    add_general("MODE", extract_value<uint8_t>(t, BPM_SWAP_CTRL_MODE_MASK));
    add_general("DIV_F_CNT_EN", get_bit(t, BPM_SWAP_CTRL_SWAP_DIV_F_CNT_EN));
    add_general("DIV_F", extract_value<uint16_t>(t, BPM_SWAP_CTRL_SWAP_DIV_F_MASK));

    t = regs.dly;
    add_general("DLY", extract_value<uint16_t>(t, BPM_SWAP_DLY_DESWAP_MASK));
}

Controller::Controller(struct pcie_bars &bars):
    RegisterController(bars, ref_devinfo),
    CONSTRUCTOR_REGS(struct bpm_swap_regs)
{
    set_read_dest(regs);
}
Controller::~Controller() = default;

const std::vector<std::string> Controller::mode_list = {
    "rffe_switching",
    "direct",
    "inverted",
    "switching",
};

void Controller::encode_params()
{
    insert_bit(regs.ctrl, reset, BPM_SWAP_CTRL_RST);

    auto mode_it = std::find(mode_list.begin(), mode_list.end(), mode);
    if (mode_it != mode_list.end())
        clear_and_insert(regs.ctrl, mode_it - mode_list.begin(), BPM_SWAP_CTRL_MODE_MASK);
    else
        throw std::runtime_error("mode must be one of " + list_of_keys(mode_list));

    insert_bit(regs.ctrl, swap_div_f_cnt_en, BPM_SWAP_CTRL_SWAP_DIV_F_CNT_EN);
    clear_and_insert(regs.ctrl, swap_div_f, BPM_SWAP_CTRL_SWAP_DIV_F_MASK);

    clear_and_insert(regs.dly, deswap_delay, BPM_SWAP_DLY_DESWAP_MASK);
}

void Controller::write_params()
{
    RegisterController::write_params();

    reset = false;
}

} /* namespace bpm_swap */
