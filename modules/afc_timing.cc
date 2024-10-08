#include <cmath>
#include <stdexcept>

#include "printer.h"
#include "util.h"
#include "si57x_util.h"
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

    constexpr unsigned AFC_TIMING_DEVID = 0xbe10be10;
    struct sdb_device_info ref_devinfo = {
        .vendor_id = LNLS_VENDORID,
        .device_id = AFC_TIMING_DEVID,
        .abi_ver_major = 1
    };
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
    RegisterDecoder(bars, ref_devinfo, {
        PRINTER("STA_LINK", "Fiber link", PrinterType::enable),
        PRINTER("STA_RXEN", "RX Enable", PrinterType::enable),
        PRINTER("STA_REFCLKLOCK", "AFC clock locked", PrinterType::boolean),
        PRINTER("STA_EVREN", "Event receiver enable", PrinterType::boolean),
        PRINTER("STA_LOCKED_RTM", "RTM clock locked", PrinterType::boolean),
        PRINTER("STA_LOCKED_GT0", "GT0 clock locked", PrinterType::boolean),
        PRINTER("STA_LOCKED_AFC_LATCH", "AFC clock locked (latch)", PrinterType::boolean),
        PRINTER("STA_LOCKED_RTM_LATCH", "RTM clock locked (latch)", PrinterType::boolean),
        PRINTER("STA_LOCKED_GT0_LATCH", "GT0 clock locked (latch)", PrinterType::boolean),
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
    CONSTRUCTOR_REGS(struct afc_timing)
{
    set_read_dest(regs);

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
    add_general("STA_LOCKED_RTM", get_bit(t, TIMING_STAT_LOCKED_RTM));
    add_general("STA_LOCKED_GT0", get_bit(t, TIMING_STAT_LOCKED_GT0));
    add_general("STA_LOCKED_AFC_LATCH", get_bit(t, TIMING_STAT_LOCKED_AFC_LATCH));
    add_general("STA_LOCKED_RTM_LATCH", get_bit(t, TIMING_STAT_LOCKED_RTM_LATCH));
    add_general("STA_LOCKED_GT0_LATCH", get_bit(t, TIMING_STAT_LOCKED_GT0_LATCH));

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
    }

    for (unsigned i = 0; i < *number_of_channels; i++) {
        auto const &trigger = regs.trigger[i];

        t = trigger.config;
        add_channel("CH_EN", i, get_bit(t, TIMING_AMC0_EN));
        add_channel("CH_POL", i, get_bit(t, TIMING_AMC0_POL));
        add_channel("CH_LOG", i, get_bit(t, TIMING_AMC0_LOG));
        add_channel("CH_ITL", i, get_bit(t, TIMING_AMC0_ITL));

        auto ch_src = extract_value<uint8_t>(t, TIMING_AMC0_SRC_MASK);
        assert(ch_src < Controller::sources_list.size());
        add_channel("CH_SRC", i, ch_src);

        add_channel("CH_DIR", i, get_bit(t, TIMING_AMC0_DIR));

        add_channel("CH_PULSES", i, trigger.pulses);
        add_channel("CH_COUNT", i, trigger.count);
        add_channel("CH_EVT", i, trigger.evt);
        add_channel("CH_DLY", i, trigger.dly);
        add_channel("CH_WDT", i, trigger.wdt);
    }
}

Controller::Controller(struct pcie_bars &bars):
    RegisterController(bars, ref_devinfo),
    CONSTRUCTOR_REGS(struct afc_timing)
{
    set_read_dest(regs);
    parameters.resize(NUM_CHANNELS);
}
Controller::~Controller() = default;

const std::vector<std::string> Controller::sources_list = {
    "Trigger",
    "Clock0",
    "Clock1",
    "Clock2",
    "Clock3",
    "Clock4",
    "Clock5",
    "Clock6",
    "Clock7",
};

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
    si57x_parameters si57x;
    if (!si57x.set_freq(freq))
        return false;

    clock.n1 = si57x.n1;
    clock.hs_div = si57x.hs_div;
    clock.rfreq = si57x.rfreq;

    return true;
}

void Controller::encode_params()
{
    insert_bit(regs.stat, event_receiver_enable, TIMING_STAT_EVREN);
    insert_bit(regs.stat, reset_latches, TIMING_STAT_RST_LATCH);

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

    auto setup_trigger = [](const auto &parameters, auto &registers) {
        insert_bit(registers.config, parameters.enable, TIMING_AMC0_EN);
        insert_bit(registers.config, parameters.polarity, TIMING_AMC0_POL);
        insert_bit(registers.config, parameters.log, TIMING_AMC0_LOG);
        insert_bit(registers.config, parameters.interlock, TIMING_AMC0_ITL);

        clear_and_insert_index(registers.config, TIMING_AMC0_SRC_MASK, parameters.source, sources_list);

        insert_bit(registers.config, parameters.direction, TIMING_AMC0_DIR);
        insert_bit(registers.config, parameters.count_reset, TIMING_AMC0_COUNT_RST);

        registers.pulses = parameters.pulses;
        registers.evt = parameters.event_code;
        registers.dly = parameters.delay;
        registers.wdt = parameters.width;
    };

    for (unsigned i = 0; i < NUM_CHANNELS; i++)
        setup_trigger(parameters[i], regs.trigger[i]);
}

void Controller::write_params()
{
    RegisterController::write_params();

    reset_latches = false;
    for (auto &p: parameters)
        p.count_reset = false;
}

} /* namespace afc_timing */
