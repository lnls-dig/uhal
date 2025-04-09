#ifndef SI57X_UTIL_H
#define SI57X_UTIL_H

#include <cstdint>

struct si57x_parameters {
    /** Nominal fxtal value, can be used when the device can't be reset to
     * determine the calibrated value. */
    double fxtal = 114285000;
    /** Output frequency after the device has been reset. Can be determined in
     * <https://tools.skyworksinc.com/TimingUtility/timing-part-number-search-results.aspx>
     * using the part number. */
    double fstartup;
    /** RFREQ integer value (38 bits max) */
    uint64_t rfreq;
    /** N1 in hardware (value - 1) */
    uint32_t n1;
    /** HS_DIV in hardware (value - 4) */
    uint32_t hs_div;

    /** Get #fxtal for the device. This function assumes the device was reset;
     * #fstartup has the correct value for that device; and #rfreq, #n1 and
     * #hs_div have been read from the device after the reset. */
    bool calc_fxtal();
    /** Set #rfreq, #n1 and #hs_div so the device outputs the desired frequency.
     * Assumes #fxtal for the device has already been determined. Returns false
     * on failure. */
    bool set_freq(double);
    /** Get the output frequency, based on #fxtal, #rfreq, #n1 and #hs_div. */
    [[nodiscard]]
    double get_freq() const;
};

#endif
