/*
 * Copyright (C) 2022 CNPEM (cnpem.br)
 * Author: Érico Nogueira <erico.rolim@lnls.br>
 *
 * Released according to the GNU GPL, version 3 or any later version.
 */

#include <cmath>
#include <stdexcept>
#include <unordered_map>

#include "printer.h"
#include "util.h"
#include "fofb_processing.h"

namespace fofb_processing {

#include "hw/wb_fofb_processing_regs.h"
#define FOFB_PROCESSING_RAM_BANK_SIZE (WB_FOFB_PROCESSING_REGS_RAM_BANK_1 - WB_FOFB_PROCESSING_REGS_RAM_BANK_0)

static constexpr unsigned MAX_NUM_CHAN = 12;

Core::Core(struct pcie_bars &bars):
    RegisterDecoder(bars, {
        I("FIXED_POINT_POS", "Position of point in fixed point representation", PrinterType::value),
    }),
    regs_storage(new struct wb_fofb_processing_regs),
    regs(*regs_storage)
{
    read_size = sizeof regs;
    read_dest = &regs;

    device_match = device_match_fofb_processing;
}
Core::~Core() = default;

void Core::decode()
{
    auto add_general = [this](const char *name, auto value) {
        general_data[name] = value;
        if (!data_order_done)
            general_data_order.push_back(name);
    };

    fixed_point = extract_value<uint32_t>(regs.fixed_point_pos, WB_FOFB_PROCESSING_REGS_FIXED_POINT_POS_VAL_MASK);
    add_general("FIXED_POINT_POS", fixed_point);

    data_order_done = true;

    /* we don't use number_of_channels here,
     * because there aren't 'normal' per channel variables */
    coefficients.resize(MAX_NUM_CHAN);

    auto convert_fixed_point = [this](uint32_t value) -> float {
        return (double)(int32_t)value / (1 << fixed_point);
    };

    /* finish conversion */
    unsigned i;
    auto add_bank = [this, &i, &convert_fixed_point](const auto &ram_bank) {
        const size_t elements = sizeof ram_bank/sizeof *ram_bank;
        coefficients[i].resize(elements);

        size_t u = 0;
        std::generate(coefficients[i].begin(), coefficients[i].end(),
            [&](){ return convert_fixed_point(ram_bank[u++].data); });
    };

    for (i = 0; i < MAX_NUM_CHAN; i++) switch (i) {
        case 0: add_bank(regs.ram_bank_0); break;
        case 1: add_bank(regs.ram_bank_1); break;
        case 2: add_bank(regs.ram_bank_2); break;
        case 3: add_bank(regs.ram_bank_3); break;
        case 4: add_bank(regs.ram_bank_4); break;
        case 5: add_bank(regs.ram_bank_5); break;
        case 6: add_bank(regs.ram_bank_6); break;
        case 7: add_bank(regs.ram_bank_7); break;
        case 8: add_bank(regs.ram_bank_8); break;
        case 9: add_bank(regs.ram_bank_9); break;
        case 10: add_bank(regs.ram_bank_10); break;
        case 11: add_bank(regs.ram_bank_11); break;
        default: throw std::logic_error("there are only 12 RAM banks");
    }
}

void Core::print(FILE *f, bool verbose)
{
    const bool v = verbose;

    RegisterDecoder::print(f, v);

    if (channel) for (auto v: coefficients[*channel]) {
        fprintf(f, "%f\n", v);
    }
}

Controller::Controller(struct pcie_bars &bars):
    RegisterController(bars),
    regs_storage(new struct wb_fofb_processing_regs),
    regs(*regs_storage),
    ram_bank_values(FOFB_PROCESSING_RAM_BANK_SIZE / sizeof(float))
{
}
Controller::~Controller() = default;

void Controller::get_internal_values()
{
    bar4_read_v(&bars, addr, &regs, sizeof regs);
    fixed_point = extract_value<uint32_t>(regs.fixed_point_pos, WB_FOFB_PROCESSING_REGS_FIXED_POINT_POS_VAL_MASK);
}

void Controller::encode_config()
{
    get_internal_values();

    /* use a templated lambda here, because the type for each bank is different */
    auto set_bank = [fixed_point = fixed_point, &ram_bank_values = ram_bank_values](auto &ram_bank) {
        for (unsigned i = 0; i < ram_bank_values.size(); i++)
            ram_bank[i].data = round((double)ram_bank_values[i] * (1 << fixed_point));
    };

    switch (channel) {
        case 0: set_bank(regs.ram_bank_0); return;
        case 1: set_bank(regs.ram_bank_1); return;
        case 2: set_bank(regs.ram_bank_2); return;
        case 3: set_bank(regs.ram_bank_3); return;
        case 4: set_bank(regs.ram_bank_4); return;
        case 5: set_bank(regs.ram_bank_5); return;
        case 6: set_bank(regs.ram_bank_6); return;
        case 7: set_bank(regs.ram_bank_7); return;
        case 8: set_bank(regs.ram_bank_8); return;
        case 9: set_bank(regs.ram_bank_9); return;
        case 10: set_bank(regs.ram_bank_10); return;
        case 11: set_bank(regs.ram_bank_11); return;
        default: throw std::runtime_error("there are only 12 RAM banks");
    }
}

void Controller::write_params()
{
    encode_config();

    bar4_write_v(&bars, addr, &regs, sizeof regs);
}

} /* namespace fofb_processing */
