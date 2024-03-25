/*
 * Copyright (C) 2023 CNPEM (cnpem.br)
 * Author: Érico Nogueira <erico.rolim@lnls.br>
 *
 * Released according to the GNU GPL, version 3 or any later version.
 */

#include <algorithm>
#include <stdexcept>

#include "pcie.h"
#include "printer.h"
#include "util.h"
#include "modules/fofb_shaper_filt.h"

namespace fofb_shaper_filt {

#include "hw/wb_fofb_shaper_filt_regs.h"

static_assert(WB_IIR_FILT_REGS_CH_SIZE + WB_IIR_FILT_REGS_CH == offsetof(wb_iir_filt_regs, ch[1]));
static_assert(WB_IIR_FILT_REGS_MAX_FILT_ORDER == offsetof(wb_iir_filt_regs, max_filt_order));

namespace {
    const size_t NUM_CHANNELS = 12, NUM_COEFFS =  sizeof(wb_iir_filt_regs::ch::coeffs) / sizeof(wb_iir_filt_regs::ch::coeffs::val);
    static_assert(NUM_COEFFS == 50);

    constexpr unsigned FOFB_SHAPER_FILT_DEVID = 0xf65559b2;
    struct sdb_device_info ref_devinfo = {
        .vendor_id = LNLS_VENDORID,
        .device_id = FOFB_SHAPER_FILT_DEVID,
        .abi_ver_major = 0
    };
}

filter_coefficients::filter_coefficients():
  values(NUM_CHANNELS)
{
    for (auto &c: values) c.resize(NUM_COEFFS);
}

Core::Core(struct pcie_bars &bars):
    RegisterDecoder(bars, ref_devinfo, {
    }),
    CONSTRUCTOR_REGS(struct wb_iir_filt_regs)
{
    set_read_dest(regs);
    number_of_channels = NUM_CHANNELS;
}
Core::~Core() = default;

/* no monitored values */
void Core::read_monitors()
{
}
void Core::decode_monitors()
{
}

void Core::decode()
{
    uint32_t t;
    add_general("MAX_FILT_ORDER", regs.max_filt_order);

    t = regs.coeffs_fp_repr;
    uint32_t int_width = extract_value<uint8_t>(t, WB_IIR_FILT_REGS_COEFFS_FP_REPR_INT_WIDTH_MASK);
    add_general("INT_WIDTH", int_width);
    add_general("FRAC_WIDTH", extract_value<uint8_t>(t, WB_IIR_FILT_REGS_COEFFS_FP_REPR_FRAC_WIDTH_MASK));

    uint32_t fixed_point_coeff = 32 - int_width;

    for (unsigned i = 0; i < NUM_CHANNELS; i++) {
        size_t u = 0;
        std::generate(coefficients.values[i].begin(), coefficients.values[i].end(),
            [&](){ return fixed2float(regs.ch[i].coeffs[u++].val, fixed_point_coeff); });
    }
}

Controller::Controller(struct pcie_bars &bars):
    RegisterController(bars, ref_devinfo),
    CONSTRUCTOR_REGS(struct wb_iir_filt_regs)
{
    set_read_dest(regs);
}
Controller::~Controller() = default;

void Controller::set_devinfo_callback()
{
    read();
    fixed_point_coeff = 32 - extract_value<uint8_t>(regs.coeffs_fp_repr, WB_IIR_FILT_REGS_COEFFS_FP_REPR_INT_WIDTH_MASK);
}

void Controller::encode_params()
{
    for (unsigned i = 0; i < NUM_CHANNELS; i++)
        for (unsigned j = 0; j < NUM_COEFFS; j++)
            regs.ch[i].coeffs[j].val = float2fixed(coefficients.values[i][j], fixed_point_coeff);
}

void Controller::write_params()
{
    RegisterController::write_params();

    insert_bit(regs.ctl, true, WB_IIR_FILT_REGS_CTL_EFF_COEFFS);
    bar4_write(&bars, addr + WB_IIR_FILT_REGS_CTL, regs.ctl);
    insert_bit(regs.ctl, false, WB_IIR_FILT_REGS_CTL_EFF_COEFFS);
}

} /* namespace fofb_shaper_filt */
