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
#include "modules/afc_timing.h"

namespace afc_timing {

#include "hw/wb_slave_afc_timing_regs.h"

static_assert(TIMING_RTM_N1_MASK == TIMING_AFC_N1_MASK);
static_assert(TIMING_RTM_HS_DIV_MASK == TIMING_AFC_HS_DIV_MASK);

static_assert(TIMING_RTM_FREQ_KP_MASK == TIMING_RTM_PHASE_KP_MASK);
static_assert(TIMING_RTM_FREQ_KI_MASK == TIMING_RTM_PHASE_KI_MASK);
static_assert(TIMING_RTM_PHASE_KP_MASK == TIMING_AFC_PHASE_KP_MASK);
static_assert(TIMING_RTM_PHASE_KI_MASK == TIMING_AFC_PHASE_KI_MASK);

static_assert(TIMING_RTM_MAF_NAVG_MASK == TIMING_AFC_MAF_NAVG_MASK);
static_assert(TIMING_RTM_MAF_DIV_EXP_MASK == TIMING_AFC_MAF_DIV_EXP_MASK);

static_assert(TIMING_AMC0_EN == TIMING_FMC2CH4_EN);
static_assert(TIMING_AMC0_POL == TIMING_FMC2CH4_POL);
static_assert(TIMING_AMC0_LOG == TIMING_FMC2CH4_LOG);
static_assert(TIMING_AMC0_ITL == TIMING_FMC2CH4_ITL);
static_assert(TIMING_AMC0_SRC_MASK == TIMING_FMC2CH4_SRC_MASK);
static_assert(TIMING_AMC0_DIR == TIMING_FMC2CH4_DIR);
static_assert(TIMING_AMC0_COUNT_RST == TIMING_FMC2CH4_COUNT_RST);

namespace {
    constexpr unsigned NUM_CHANNELS = 18;
}

/* Make sure we have the right number of channels by using:
 * - the size of registers for a given channel
 * - the number of channels
 * - the size of registers for all channels */
static_assert((TIMING_REG_AMC1 - TIMING_REG_AMC0) * NUM_CHANNELS == TIMING_REG_FMC2CH4_WDT - TIMING_REG_AMC0 + 4);

struct afc_timing {
    uint32_t stat, alive;
    struct {
        uint32_t rfreq_hi, rfreq_lo, n1_hs_div, freq_control_loop, phase_control_loop, maf;
    } rtm_clock, afc_clock;
    struct {
        uint32_t config, pulses, count, evt, dly, wdt;
    } trigger[NUM_CHANNELS];
};

Core::Core(struct pcie_bars &bars):
    RegisterDecoder(bars, {
        PRINTER("STA_LINK", "Fiber link", PrinterType::enable),
        PRINTER("STA_RXEN", "RX Enable", PrinterType::enable),
        PRINTER("STA_REFCLKLOCK", "Reference clock locked", PrinterType::boolean),
        PRINTER("STA_EVREN", "Event receiver enable", PrinterType::boolean),
        PRINTER("ALIVE", "Alive counter", PrinterType::value),

        PRINTER("RFREQ_HI", "Si57x RFREQ MSB", PrinterType::value),
        PRINTER("RFREQ_LO", "Si57x RFREQ LSB", PrinterType::value),
        PRINTER("N1", "Si57x N1", PrinterType::value),
        PRINTER("HS_DIV", "Si57x HS_DIV", PrinterType::value),
        PRINTER("FREQ_KP", "Frequency proportional gain", PrinterType::value),
        PRINTER("FREQ_KI", "Frequency integral gain", PrinterType::value),
        PRINTER("PHASE_KP", "Phase proportional gain", PrinterType::value),
        PRINTER("PHASE_KI", "Phase integral gain", PrinterType::value),
        PRINTER("MAF_NAVG", "DDMTD average number", PrinterType::value),
        PRINTER("MAF_DIV_EXP", "DDMTD divider exponent (2^N)", PrinterType::value),

        PRINTER("CH_EN", "Channel enable", PrinterType::enable),
        PRINTER("CH_POL", "Channel polarity", PrinterType::boolean),
        PRINTER("CH_LOG", "Channel time log", PrinterType::enable),
        PRINTER("CH_ITL", "Channel interlock", PrinterType::enable),
        PRINTER("CH_SRC", "Channel output source", PrinterType::value),
        PRINTER("CH_DIR", "Channel output direction", PrinterType::boolean),
        PRINTER("CH_PULSES", "Channel pulses", PrinterType::value),
        PRINTER("CH_COUNT", "Channel count", PrinterType::value),
        PRINTER("CH_EVT", "Channel event code", PrinterType::value),
        PRINTER("CH_DLY", "Channel delay to trigger output", PrinterType::value),
        PRINTER("CH_WDT", "Channel trigger output width", PrinterType::value),
    }),
    regs_storage(new struct afc_timing),
    regs(*regs_storage)
{
    read_size = sizeof regs;
    read_dest = &regs;

    device_match = device_match_afc_timing;

    number_of_channels = NUM_CHANNELS;
}
Core::~Core() = default;

void Core::decode()
{
    uint32_t t = regs.stat;
    add_general("STA_LINK", get_bit(t, TIMING_STAT_LINK));
    add_general("STA_RXEN", get_bit(t, TIMING_STAT_RXEN));
    add_general("STA_REFCLKLOCK", get_bit(t, TIMING_STAT_REFCLKLOCK));
    add_general("STA_EVREN", get_bit(t, TIMING_STAT_EVREN));

    add_general("ALIVE", regs.alive);

    size_t i = 0;
    for (auto clockp: {&regs.rtm_clock, &regs.afc_clock}) {
        add_channel("RFREQ_HI", i, clockp->rfreq_hi);
        add_channel("RFREQ_LO", i, clockp->rfreq_lo);
        add_channel("N1", i, extract_value<uint8_t>(clockp->n1_hs_div, TIMING_RTM_N1_MASK));
        add_channel("HS_DIV", i, extract_value<uint8_t>(clockp->n1_hs_div, TIMING_RTM_HS_DIV_MASK));
        add_channel("FREQ_KP", i, extract_value<int16_t>(clockp->freq_control_loop, TIMING_RTM_FREQ_KP_MASK));
        add_channel("FREQ_KI", i, extract_value<int16_t>(clockp->freq_control_loop, TIMING_RTM_FREQ_KI_MASK));
        add_channel("PHASE_KP", i, extract_value<int16_t>(clockp->phase_control_loop, TIMING_RTM_PHASE_KP_MASK));
        add_channel("PHASE_KI", i, extract_value<int16_t>(clockp->phase_control_loop, TIMING_RTM_PHASE_KI_MASK));
        add_channel("MAF_NAVG", i, extract_value<uint16_t>(clockp->maf, TIMING_RTM_MAF_NAVG_MASK));
        add_channel("MAF_DIV_EXP", i, extract_value<uint16_t>(clockp->maf, TIMING_RTM_MAF_DIV_EXP_MASK));
        i++;

        data_order_done = true;
    }

    data_order_done = false;
    for (unsigned i = 0; i < *number_of_channels; i++) {
        auto const &trigger = regs.trigger[i];

        t = trigger.config;
        add_channel("CH_EN", i, get_bit(t, TIMING_AMC0_EN));
        add_channel("CH_POL", i, get_bit(t, TIMING_AMC0_POL));
        add_channel("CH_LOG", i, get_bit(t, TIMING_AMC0_LOG));
        add_channel("CH_ITL", i, get_bit(t, TIMING_AMC0_ITL));
        add_channel("CH_SRC", i, extract_value<uint8_t>(t, TIMING_AMC0_SRC_MASK));
        add_channel("CH_DIR", i, get_bit(t, TIMING_AMC0_DIR));

        add_channel("CH_PULSES", i, trigger.pulses);
        add_channel("CH_COUNT", i, trigger.count);
        add_channel("CH_EVT", i, trigger.evt);
        add_channel("CH_DLY", i, trigger.dly);
        add_channel("CH_WDT", i, trigger.wdt);

        data_order_done = true;
    }
}

Controller::Controller(struct pcie_bars &bars):
    RegisterController(bars),
    regs_storage(new struct afc_timing()),
    regs(*regs_storage)
{
}
Controller::~Controller() = default;

void Controller::set_devinfo_callback()
{
}

bool Controller::set_rtm_freq(double freq)
{
    return set_freq(freq, rtm_clock);
}
bool Controller::set_afc_freq(double freq)
{
    return set_freq(freq, afc_clock);
}

bool Controller::set_freq(double freq, struct clock &clock)
{
    /* from Si57x datasheet */
    const double fxtal = 114285000;
    const double fdco_min = 4850000000;
    const double fdco_max = 5670000000;

    const uint32_t n1_max_val = 128;
    const uint32_t n1_min_val = 2;
    const uint32_t n1_step = 2;
    const uint32_t hs_div_opts[] = {11, 9, 7, 6, 5, 4};

    const uint32_t rfreq_factor = 1 << 28;
    const double freq_err_initial = 1e9;

    uint32_t n1_best, hs_div_best;
    uint64_t rfreq_best;
    double freq_err_best = freq_err_initial;
    for (auto hs_div_opt: hs_div_opts) {
        for (uint32_t n1 = n1_min_val; n1 <= n1_max_val; n1 += n1_step) {
            uint64_t rfreq = (freq * hs_div_opt * n1 * rfreq_factor / fxtal);
            double fdco = (rfreq * fxtal) / rfreq_factor;

            if ((fdco >= fdco_min) && (fdco <= fdco_max)) {
                double freq_err = fabs(freq - (fdco / (hs_div_opt * n1)));
                if (freq_err < freq_err_best) {
                    freq_err_best = freq_err;
                    n1_best = n1;
                    hs_div_best = hs_div_opt;
                    rfreq_best = rfreq;
                }
            }
        }
    }
    if (freq_err_best == freq_err_initial) return false;

    /* values to actually write into registers */
    clock.n1 = n1_best - 1;
    clock.hs_div = hs_div_best - 4;
    clock.rfreq = rfreq_best;

    return true;
}

void Controller::encode_config()
{
    insert_bit(regs.stat, event_receiver_enable, TIMING_STAT_EVREN);

    auto setup_clock = [](const auto &parameters, auto &registers) {
        registers.rfreq_hi = parameters.rfreq >> 20;
        registers.rfreq_lo = parameters.rfreq & 0xfffff;

        clear_and_insert(registers.n1_hs_div, parameters.n1, TIMING_RTM_N1_MASK);
        clear_and_insert(registers.n1_hs_div, parameters.hs_div, TIMING_RTM_HS_DIV_MASK);

        auto setup_loop = [](const auto &p, auto &r) {
            clear_and_insert(r, p.kp, TIMING_RTM_FREQ_KP_MASK);
            clear_and_insert(r, p.ki, TIMING_RTM_FREQ_KI_MASK);
        };
        setup_loop(parameters.freq_loop, registers.freq_control_loop);
        setup_loop(parameters.phase_loop, registers.phase_control_loop);

        clear_and_insert(registers.maf, parameters.ddmtd_config.div_exp, TIMING_RTM_MAF_DIV_EXP_MASK);
        clear_and_insert(registers.maf, parameters.ddmtd_config.navg, TIMING_RTM_MAF_NAVG_MASK);
    };
    setup_clock(rtm_clock, regs.rtm_clock);
    setup_clock(afc_clock, regs.afc_clock);
}

void Controller::write_params()
{
    encode_config();

    bar4_write_v(&bars, addr, &regs, sizeof regs);
}

} /* namespace afc_timing */
