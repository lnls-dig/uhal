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
#include "hw/wb_fofb_processing_regs.h"

#define FOFB_PROCESSING_RAM_BANK_SIZE (WB_FOFB_PROCESSING_REGS_RAM_BANK_1 - WB_FOFB_PROCESSING_REGS_RAM_BANK_0)

LnlsFofbProcessing::LnlsFofbProcessing(struct pcie_bars *bars):
    RegisterDecoder(bars),
    regs_storage(new struct wb_fofb_processing_regs),
    regs(*regs_storage)
{
    read_size = sizeof regs;
    read_dest = &regs;

    device_match = device_match_fofb_processing;
}
LnlsFofbProcessing::~LnlsFofbProcessing() = default;

void LnlsFofbProcessing::print(FILE *f, bool verbose)
{
    static const std::unordered_map<const char *, Printer> printers ({
      I("FIXED_POINT_POS", "Position of point in fixed point representation", PrinterType::value),
    });

    bool v = verbose;
    unsigned indent = 0;
    uint32_t fixed_point;

    auto print = [f, v, &indent](const char *name, uint32_t value) {
        printers.at(name).print(f, v, indent, value);
    };

    fixed_point = extract_value<uint32_t>(regs.fixed_point_pos, WB_FOFB_PROCESSING_REGS_FIXED_POINT_POS_VAL_MASK);
    print("FIXED_POINT_POS", fixed_point);

    auto convert_fixed_point = [fixed_point](uint32_t value) -> float {
        return (double)(int32_t)value / (1 << fixed_point);
    };

    auto read_bank = [f, &convert_fixed_point](const auto &ram_bank) {
        for (auto v: ram_bank)
            fprintf(f, "%f\n", convert_fixed_point(v.data));
    };

    if (channel) switch (*channel) {
        case 0: read_bank(regs.ram_bank_0); return;
        case 1: read_bank(regs.ram_bank_1); return;
        case 2: read_bank(regs.ram_bank_2); return;
        case 3: read_bank(regs.ram_bank_3); return;
        case 4: read_bank(regs.ram_bank_4); return;
        case 5: read_bank(regs.ram_bank_5); return;
        case 6: read_bank(regs.ram_bank_6); return;
        case 7: read_bank(regs.ram_bank_7); return;
        case 8: read_bank(regs.ram_bank_8); return;
        case 9: read_bank(regs.ram_bank_9); return;
        case 10: read_bank(regs.ram_bank_10); return;
        case 11: read_bank(regs.ram_bank_11); return;
        default: throw std::runtime_error("there are only 12 RAM banks");
    }
}

LnlsFofbProcessingController::LnlsFofbProcessingController(struct pcie_bars *bars):
    RegisterController(bars),
    regs_storage(new struct wb_fofb_processing_regs),
    regs(*regs_storage),
    ram_bank_values(FOFB_PROCESSING_RAM_BANK_SIZE / sizeof(float))
{
}
LnlsFofbProcessingController::~LnlsFofbProcessingController() = default;

void LnlsFofbProcessingController::get_internal_values()
{
    bar4_read_v(bars, addr, &regs, sizeof regs);
    fixed_point = extract_value<uint32_t>(regs.fixed_point_pos, WB_FOFB_PROCESSING_REGS_FIXED_POINT_POS_VAL_MASK);
}

void LnlsFofbProcessingController::encode_config()
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

void LnlsFofbProcessingController::write_params()
{
    encode_config();

    bar4_write_v(bars, addr, &regs, sizeof regs);
}
