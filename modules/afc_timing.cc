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
        .abi_ver_major = 2
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
        PRINTER("LINK", "Fiber link", PrinterType::enable),
        PRINTER("RXEN", "RX Enable", PrinterType::enable),
        PRINTER("EVREN", "Event receiver enable", PrinterType::boolean),
        PRINTER("LOCKED_AFC_FREQ", "AFC PLL frequency lock status", PrinterType::boolean),
        PRINTER("LOCKED_AFC_PHASE", "AFC PLL phase lock status", PrinterType::boolean),
        PRINTER("LOCKED_RTM_FREQ", "RTM PLL frequency lock status", PrinterType::boolean),
        PRINTER("LOCKED_RTM_PHASE", "RTM PLL phase lock status", PrinterType::boolean),
        PRINTER("LOCKED_GT0", "GT0 PLL lock status", PrinterType::boolean),
        PRINTER("LOCKED_AFC_FREQ_LTC", "Latched AFC PLL frequency lock status", PrinterType::boolean),
        PRINTER("LOCKED_AFC_PHASE_LTC", "Latched AFC PLL phase lock status", PrinterType::boolean),
        PRINTER("LOCKED_RTM_FREQ_LTC", "Latched RTM PLL frequency lock status", PrinterType::boolean),
        PRINTER("LOCKED_RTM_PHASE_LTC", "Latched RTM PLL phase lock status", PrinterType::boolean),
        PRINTER("LOCKED_GT0_LTC", "Latched GT0 PLL lock status", PrinterType::boolean),
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
    uint32_t *pt = &regs.stat;
    add_general("LINK", get_bit(*pt, TIMING_STAT_LINK));
    add_general("RXEN", get_bit(*pt, TIMING_STAT_RXEN));
    add_general("EVREN", rf_get_bit(*pt, TIMING_STAT_EVREN));

    add_general("LOCKED_AFC_FREQ", get_bit(*pt, TIMING_STAT_LOCKED_AFC_FREQ));
    add_general("LOCKED_AFC_PHASE", get_bit(*pt, TIMING_STAT_LOCKED_AFC_PHASE));
    add_general("LOCKED_RTM_FREQ", get_bit(*pt, TIMING_STAT_LOCKED_RTM_FREQ));
    add_general("LOCKED_RTM_PHASE", get_bit(*pt, TIMING_STAT_LOCKED_RTM_PHASE));
    add_general("LOCKED_GT0", get_bit(*pt, TIMING_STAT_LOCKED_GT0));
    add_general("LOCKED_AFC_FREQ_LTC", get_bit(*pt, TIMING_STAT_LOCKED_AFC_FREQ_LTC));
    add_general("LOCKED_AFC_PHASE_LTC", get_bit(*pt, TIMING_STAT_LOCKED_AFC_PHASE_LTC));
    add_general("LOCKED_RTM_FREQ_LTC", get_bit(*pt, TIMING_STAT_LOCKED_RTM_FREQ_LTC));
    add_general("LOCKED_RTM_PHASE_LTC", get_bit(*pt, TIMING_STAT_LOCKED_RTM_PHASE_LTC));
    add_general("LOCKED_GT0_LTC", get_bit(*pt, TIMING_STAT_LOCKED_GT0_LTC));
    add_general("RST_LOCKED_LTCS", rf_get_bit(*pt, TIMING_STAT_RST_LOCKED_LTCS));

    add_general("ALIVE", regs.alive);

    size_t i = 0;
    for (auto clockp: {&regs.rtm_clock, &regs.afc_clock}) {
        add_channel("RFREQ_HI", i, clockp->rfreq_hi);
        add_channel("RFREQ_LO", i, clockp->rfreq_lo);
        add_channel("N1", i, extract_value<uint8_t>(clockp->n1_hs_div, TIMING_RTM_N1_MASK));
        add_channel("HS_DIV", i, extract_value<uint8_t>(clockp->n1_hs_div, TIMING_RTM_HS_DIV_MASK));
        add_channel("FREQ_KP", i, rf_extract_value(clockp->freq_control_loop, TIMING_RTM_FREQ_KP_MASK, true));
        add_channel("FREQ_KI", i, rf_extract_value(clockp->freq_control_loop, TIMING_RTM_FREQ_KI_MASK, true));
        add_channel("PHASE_KP", i, rf_extract_value(clockp->phase_control_loop, TIMING_RTM_PHASE_KP_MASK, true));
        add_channel("PHASE_KI", i, rf_extract_value(clockp->phase_control_loop, TIMING_RTM_PHASE_KI_MASK, true));
        add_channel("MAF_NAVG", i, rf_extract_value(clockp->maf, TIMING_RTM_MAF_NAVG_MASK));
        add_channel("MAF_DIV_EXP", i, rf_extract_value(clockp->maf, TIMING_RTM_MAF_DIV_EXP_MASK));
        i++;
    }

    for (unsigned i = 0; i < *number_of_channels; i++) {
        auto &trigger = regs.trigger[i];

        pt = &trigger.config;
        add_channel("CH_EN", i, rf_get_bit(*pt, TIMING_AMC0_EN));
        add_channel("CH_POL", i, rf_get_bit(*pt, TIMING_AMC0_POL));
        add_channel("CH_LOG", i, rf_get_bit(*pt, TIMING_AMC0_LOG));
        add_channel("CH_ITL", i, rf_get_bit(*pt, TIMING_AMC0_ITL));

        auto ch_src = rf_extract_value(*pt, TIMING_AMC0_SRC_MASK);
        assert(std::get<int32_t>(ch_src.value) < (ssize_t)Controller::sources_list.size());
        add_channel("CH_SRC", i, ch_src);

        add_channel("CH_DIR", i, rf_get_bit(*pt, TIMING_AMC0_DIR));

        add_channel("CH_COUNT_RST", i, rf_get_bit(*pt, TIMING_AMC0_COUNT_RST));

        add_channel("CH_PULSES", i, rf_whole_register(trigger.pulses));
        add_channel("CH_COUNT", i, trigger.count);
        add_channel("CH_EVT", i, rf_whole_register(trigger.evt));
        add_channel("CH_DLY", i, rf_whole_register(trigger.dly));
        add_channel("CH_WDT", i, rf_whole_register(trigger.wdt));
    }
}

Controller::Controller(struct pcie_bars &bars):
    RegisterDecoderController(bars, ref_devinfo, &dec),
    CONSTRUCTOR_REGS(struct afc_timing),
    dec(bars)
{
    set_read_dest(regs);
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
    auto setup_clock = [](const auto &parameters, auto &registers) {
        registers.rfreq_hi = parameters.rfreq >> 20;
        registers.rfreq_lo = parameters.rfreq & 0xfffff;

        clear_and_insert(registers.n1_hs_div, parameters.n1, TIMING_RTM_N1_MASK);
        clear_and_insert(registers.n1_hs_div, parameters.hs_div, TIMING_RTM_HS_DIV_MASK);
    };
    setup_clock(rtm_clock, regs.rtm_clock);
    setup_clock(afc_clock, regs.afc_clock);
}

void Controller::unset_commands()
{
    write_general("RST_LOCKED_LTCS", 0);
    for (unsigned i = 0; i < NUM_CHANNELS; i++)
        write_channel("CH_COUNT_RST", i, 0);
}

} /* namespace afc_timing */
