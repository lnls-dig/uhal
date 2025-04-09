#include <algorithm>
#include <stdexcept>

#include "pcie.h"
#include "printer.h"
#include "util.h"
#include "modules/fofb_shaper_filt.h"

namespace fofb_shaper_filt {

#include "hw/wb_fofb_shaper_filt_regs.h"

static_assert(WB_FOFB_SHAPER_FILT_REGS_CH_SIZE + WB_FOFB_SHAPER_FILT_REGS_CH == offsetof(wb_fofb_shaper_filt_regs, ch[1]));
static_assert(WB_FOFB_SHAPER_FILT_REGS_NUM_BIQUADS == offsetof(wb_fofb_shaper_filt_regs, num_biquads));

namespace {
    const size_t
        NUM_CHANNELS = 12,
        NUM_COEFFS =  sizeof(wb_fofb_shaper_filt_regs::ch::coeffs) / sizeof(wb_fofb_shaper_filt_regs::ch::coeffs::val),
        NUM_BIQUADS = 10,
        COEFFS_PER_BIQUAD = 5,
        UNUSED_PER_BIQUAD = 3,
        TOTAL_PER_BIQUAD = COEFFS_PER_BIQUAD + UNUSED_PER_BIQUAD;
    static_assert(NUM_COEFFS == NUM_BIQUADS * TOTAL_PER_BIQUAD);

    constexpr unsigned FOFB_SHAPER_FILT_DEVID = 0xf65559b2;
    struct sdb_device_info ref_devinfo = {
        .vendor_id = LNLS_VENDORID,
        .device_id = FOFB_SHAPER_FILT_DEVID,
        .abi_ver_major = 1
    };
}

filter_coefficients::filter_coefficients():
  values(NUM_CHANNELS)
{
}

void filter_coefficients::set_num_biquads(size_t num_biquads)
{
    for (auto &c: values) c.resize(num_biquads * COEFFS_PER_BIQUAD);
}

Core::Core(struct pcie_bars &bars):
    RegisterDecoder(bars, ref_devinfo, { }),
    CONSTRUCTOR_REGS(struct wb_fofb_shaper_filt_regs)
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
    add_general("NUM_BIQUADS", regs.num_biquads);
    unsigned num_biquads = regs.num_biquads;

    t = regs.coeffs_fp_repr;
    uint32_t int_width = extract_value<uint8_t>(t, WB_FOFB_SHAPER_FILT_REGS_COEFFS_FP_REPR_INT_WIDTH_MASK);
    add_general("INT_WIDTH", int_width);
    add_general("FRAC_WIDTH", extract_value<uint8_t>(t, WB_FOFB_SHAPER_FILT_REGS_COEFFS_FP_REPR_FRAC_WIDTH_MASK));

    uint32_t fixed_point_coeff = 32 - int_width;

    coefficients.set_num_biquads(num_biquads);

    for (unsigned i = 0; i < NUM_CHANNELS; i++) {
        for (unsigned j = 0; j < num_biquads; j++) {
            for (unsigned k = 0; k < COEFFS_PER_BIQUAD; k++) {
                coefficients.values[i][k + (COEFFS_PER_BIQUAD * j)] =
                    fixed2float(regs.ch[i].coeffs[k + (TOTAL_PER_BIQUAD * j)].val, fixed_point_coeff);
            }
        }
    }
}

void Core::print(FILE *f, bool verbose) const
{
    RegisterDecoder::print(f, verbose);

    if (channel) {
        fprintf(f, "channel %u filter coefficients: ", *channel);
        for (auto c: coefficients.values[*channel]) {
            fprintf(f, "%lf, ", c);
        }
        fputs("\n", f);

        if (verbose) {
            fprintf(f, "channel %u raw filter coefficients: ", *channel);
            for (auto c: regs.ch[*channel].coeffs) {
                fprintf(f, "%#08x, ", (unsigned)c.val);
            }
            fputs("\n", f);
        }
    }
}

Controller::Controller(struct pcie_bars &bars):
    RegisterController(bars, ref_devinfo),
    CONSTRUCTOR_REGS(struct wb_fofb_shaper_filt_regs)
{
    set_read_dest(regs);

    coefficients.set_num_biquads(NUM_BIQUADS);
}
Controller::~Controller() = default;

void Controller::set_devinfo_callback()
{
    regs.coeffs_fp_repr = bar4_read(&bars, addr + WB_FOFB_SHAPER_FILT_REGS_COEFFS_FP_REPR);
    fixed_point_coeff = 32 - extract_value<uint8_t>(regs.coeffs_fp_repr, WB_FOFB_SHAPER_FILT_REGS_COEFFS_FP_REPR_INT_WIDTH_MASK);

    regs.num_biquads = bar4_read(&bars, addr + WB_FOFB_SHAPER_FILT_REGS_NUM_BIQUADS);
    num_biquads = regs.num_biquads;
}

void Controller::encode_params()
{
    for (unsigned i = 0; i < NUM_CHANNELS; i++) {
        for (unsigned j = 0; j < num_biquads; j++) {
            for (unsigned k = 0; k < COEFFS_PER_BIQUAD; k++) {
                regs.ch[i].coeffs[k + (TOTAL_PER_BIQUAD * j)].val =
                    float2fixed(coefficients.values[i][k + (COEFFS_PER_BIQUAD * j)], fixed_point_coeff);
            }
        }
    }
}

void Controller::write_params()
{
    encode_params();

    for (unsigned i = 0; i < NUM_CHANNELS; i++) {
        for (unsigned j = 0; j < num_biquads; j++) {
            bar4_write_v(
                &bars,
                addr + WB_FOFB_SHAPER_FILT_REGS_CH_COEFFS + (i * sizeof(regs.ch[0])) + (TOTAL_PER_BIQUAD * j * sizeof(uint32_t)),
                &regs.ch[i].coeffs[TOTAL_PER_BIQUAD * j].val,
                COEFFS_PER_BIQUAD * sizeof(uint32_t));
        }
    }
}

} /* namespace fofb_shaper_filt */
