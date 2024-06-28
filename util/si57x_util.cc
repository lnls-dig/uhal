#include <array>
#include <cmath>
#include <ranges>

#include "si57x_util.h"

namespace {
    const double fdco_min = 4850000000;
    const double fdco_max = 5670000000;

    const uint32_t n1_max_val = 128;
    const uint32_t n1_min_val = 2;
    /* FIXME: n1 can be 1 */
    const uint32_t n1_step = 2;
    const auto hs_div_opts = std::to_array<uint32_t>({11, 9, 7, 6, 5, 4});
    const double rfreq_factor = 1 << 28;

    uint32_t hw2val_n1(uint32_t n1)
    {
        return n1 + 1;
    }
    uint32_t val2hw_n1(uint32_t n1)
    {
        return n1 - 1;
    }

    uint32_t hw2val_hs_div(uint32_t hs_div)
    {
        auto rv = hs_div + 4;
        if (std::ranges::find(hs_div_opts, rv) == hs_div_opts.end())
            throw std::runtime_error("unknown hs_div value in hw.");
        return rv;
    }
    uint32_t val2hw_hs_div(uint32_t hs_div)
    {
        return hs_div - 4;
    }

    double hw2val_rfreq(uint64_t rfreq)
    {
        return (double)rfreq / rfreq_factor;
    }
}

bool si57x_parameters::calc_fxtal()
{
    fxtal = (fstartup * hw2val_hs_div(hs_div) * hw2val_n1(n1)) / hw2val_rfreq(rfreq);
    return true;
}

bool si57x_parameters::set_freq(double freq)
{
    uint32_t n1_best, hs_div_best;
    uint64_t rfreq_best;
    double freq_err_best = 1e9;
    bool best_is_set = false;
    for (auto hs_div_opt: hs_div_opts) {
        for (uint32_t n1 = n1_min_val; n1 <= n1_max_val; n1 += n1_step) {
            /* take the precision of RFREQ into account, using its hardware
             * representation to calculate fdco */
            uint64_t rfreq_val = llrint(freq * hs_div_opt * n1 * rfreq_factor / fxtal);
            double fdco = (rfreq_val * fxtal) / rfreq_factor;

            if ((fdco >= fdco_min) && (fdco <= fdco_max)) {
                double freq_err = fabs(freq - (fdco / (hs_div_opt * n1)));
                if (freq_err < freq_err_best) {
                    freq_err_best = freq_err;
                    n1_best = n1;
                    hs_div_best = hs_div_opt;
                    rfreq_best = rfreq_val;

                    best_is_set = true;
                }
            }
        }
    }
    if (!best_is_set) return false;

    /* values to actually write into registers */
    n1 = val2hw_n1(n1_best);
    hs_div = val2hw_hs_div(hs_div_best);
    rfreq = rfreq_best;

    return true;
}

double si57x_parameters::get_freq()
{
    return fxtal * hw2val_rfreq(rfreq) / (hw2val_hs_div(hs_div) * hw2val_n1(n1));
}
