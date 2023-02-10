/*
 * Copyright (C) 2022 CNPEM (cnpem.br)
 * Author: Érico Nogueira <erico.rolim@lnls.br>
 *
 * Released according to the GNU GPL, version 3 or any later version.
 */

#include <algorithm>
#include <array>
#include <charconv>
#include <cstring>
#include <stdexcept>
#include <type_traits>
#include <unordered_map>

#include "decoders.h"
#include "printer.h"
#include "util.h"
#include "acq.h"

namespace acq {

#include "hw/wb_acq_core_regs.h"

#define MAX_NUM_CHAN 24
#define REGISTERS_PER_CHAN 2

static const unsigned ddr3_payload_size = 32;

Core::Core(struct pcie_bars &bars):
    RegisterDecoder(bars, {
        PRINTER("FSQ_ACQ_NOW", "Acquire data immediately and don't wait for any trigger", PrinterType::boolean, "acquire immediately", "wait on trigger"),
        PRINTER("FSM_STATE", "State machine status", PrinterType::custom_function,
            [](FILE *f, bool v, uint32_t value){
                (void)v;
                static const char *fsm_states[] = {"IDLE", "PRE_TRIG", "WAIT_TRIG", "POST_TRIG", "DECR_SHOT"};
                switch (value) {
                    case 0: case 6: case 7:
                        fprintf(f, "illegal (%u)", value);
                        break;
                    case 1: case 2: case 3: case 4: case 5:
                        fprintf(f, "%s", fsm_states[value-1]);
                        break;
                }
                fputc('\n', f);
            }
        ),
        PRINTER("FSM_ACQ_DONE", "FSM acquisition status", PrinterType::progress),
        PRINTER("FC_TRANS_DONE", "External flow control transfer status", PrinterType::boolean),
        PRINTER("FC_FULL", "External flow control FIFO full status", PrinterType::boolean, "full (data may be lost)", "full"),
        PRINTER("DDR3_TRANS_DONE", "DDR3 transfer status", PrinterType::progress),
        PRINTER("HW_TRIG_SEL", "Hardware trigger selection", PrinterType::boolean, "external", "internal"),
        PRINTER("HW_TRIG_POL", "Hardware trigger polarity", PrinterType::boolean, "negative edge", "positive edge"),
        PRINTER("HW_TRIG_EN", "Hardware trigger enable", PrinterType::enable),
        PRINTER("SW_TRIG_EN", "Software trigger enable", PrinterType::enable),
        PRINTER("INT_TRIG_SEL", "Atom selection for internal trigger", PrinterType::value),
        PRINTER("THRES_FILT", "Internal trigger threshold glitch filter", PrinterType::value),
        PRINTER("TRIG_DATA_THRES", "Threshold for internal trigger", PrinterType::value),
        PRINTER("TRIG_DLY", "Trigger delay value", PrinterType::value),
        PRINTER("NB", "Number of shots required in multi-shot mode, one if in single-shot mode", PrinterType::value),
        PRINTER("MULTISHOT_RAM_SIZE_IMPL", "MultiShot RAM size reg implemented", PrinterType::boolean),
        PRINTER("MULTISHOT_RAM_SIZE", "MultiShot RAM size", PrinterType::value),
        PRINTER("TRIG_POS", "Trigger address in DDR memory", PrinterType::value_hex),
        PRINTER("PRE_SAMPLES", "Number of requested pre-trigger samples", PrinterType::value),
        PRINTER("POST_SAMPLES", "Number of requested post-trigger samples", PrinterType::value),
        PRINTER("SAMPLES_CNT", "Samples counter", PrinterType::value),
        PRINTER("DDR3_START_ADDR", "Start address in DDR3 memory for the next acquisition", PrinterType::value_hex),
        PRINTER("DDR3_END_ADDR", "End address in DDR3 memory for the next acquisition", PrinterType::value_hex),
        PRINTER("WHICH", "Acquisition channel selection", PrinterType::value),
        PRINTER("DTRIG_WHICH", "Data-driven channel selection", PrinterType::value),
        PRINTER("NUM_CHAN", "Number of acquisition channels", PrinterType::value),
        /* per channel info */
        PRINTER("INT_WIDTH", "Internal Channel Width", PrinterType::value),
        PRINTER("NUM_COALESCE", "Number of coalescing words", PrinterType::value),
        PRINTER("NUM_ATOMS", "Number of atoms inside the complete data word", PrinterType::value),
        PRINTER("ATOM_WIDTH", "Atom width in bits", PrinterType::value),
    }),
    regs_storage(new struct acq_core),
    regs(*regs_storage)
{
    read_size = sizeof regs;
    read_dest = &regs;

    device_match = device_match_acq;
}
Core::~Core() = default;

void Core::decode()
{
    uint32_t t;

    /* control register */
    t = regs.ctl;
    add_general("FSQ_ACQ_NOW", t & ACQ_CORE_CTL_FSM_ACQ_NOW);

    /* status register */
    t = regs.sta;

    add_general("FSM_STATE", (t & ACQ_CORE_STA_FSM_STATE_MASK) >> ACQ_CORE_STA_FSM_STATE_SHIFT);
    add_general("FSM_ACQ_DONE", t & ACQ_CORE_STA_FSM_ACQ_DONE);
    add_general("FC_TRANS_DONE", t & ACQ_CORE_STA_FC_TRANS_DONE);
    add_general("FC_FULL", t & ACQ_CORE_STA_FC_FULL);
    add_general("DDR3_TRANS_DONE", t & ACQ_CORE_STA_DDR3_TRANS_DONE);

    /* trigger configuration */
    t = regs.trig_cfg;
    add_general("HW_TRIG_SEL", t & ACQ_CORE_TRIG_CFG_HW_TRIG_SEL);
    add_general("HW_TRIG_POL", t & ACQ_CORE_TRIG_CFG_HW_TRIG_POL);
    add_general("HW_TRIG_EN", t & ACQ_CORE_TRIG_CFG_HW_TRIG_EN);
    add_general("SW_TRIG_EN", t & ACQ_CORE_TRIG_CFG_SW_TRIG_EN);
    add_general("INT_TRIG_SEL", ((t & ACQ_CORE_TRIG_CFG_INT_TRIG_SEL_MASK) >> ACQ_CORE_TRIG_CFG_INT_TRIG_SEL_SHIFT) + 1);

    /* trigger data config thresold */
    t = regs.trig_data_cfg;
    add_general("THRES_FILT", (t & ACQ_CORE_TRIG_DATA_CFG_THRES_FILT_MASK) >> ACQ_CORE_TRIG_DATA_CFG_THRES_FILT_SHIFT);

    /* trigger */
    add_general("TRIG_DATA_THRES", (int32_t)regs.trig_data_thres);
    add_general("TRIG_DLY", regs.trig_dly);

    /* number of shots */
    t = regs.shots;
    add_general("NB", (t & ACQ_CORE_SHOTS_NB_MASK) >> ACQ_CORE_SHOTS_NB_SHIFT);
    add_general("MULTISHOT_RAM_SIZE_IMPL", t & ACQ_CORE_SHOTS_MULTISHOT_RAM_SIZE_IMPL);
    add_general("MULTISHOT_RAM_SIZE", (t & ACQ_CORE_SHOTS_MULTISHOT_RAM_SIZE_MASK) >> ACQ_CORE_SHOTS_MULTISHOT_RAM_SIZE_SHIFT);

    /* trigger address register */
    add_general("TRIG_POS", regs.trig_pos);

    /* samples */
    add_general("PRE_SAMPLES", regs.pre_samples);
    add_general("POST_SAMPLES", regs.post_samples);
    add_general("SAMPLES_CNT", regs.samples_cnt);

    /* ddr3 addresses */
    add_general("DDR3_START_ADDR", regs.ddr3_start_addr);
    add_general("DDR3_END_ADDR", regs.ddr3_end_addr);

    /* acquisition channel control */
    t = regs.acq_chan_ctl;
    /* will be used to determine how many channels to show in the next block */
    unsigned num_chan = (t & ACQ_CORE_ACQ_CHAN_CTL_NUM_CHAN_MASK) >> ACQ_CORE_ACQ_CHAN_CTL_NUM_CHAN_SHIFT;
    add_general("WHICH", (t & ACQ_CORE_ACQ_CHAN_CTL_WHICH_MASK) >> ACQ_CORE_ACQ_CHAN_CTL_WHICH_SHIFT);
    add_general("DTRIG_WHICH", (t & ACQ_CORE_ACQ_CHAN_CTL_DTRIG_WHICH_MASK) >> ACQ_CORE_ACQ_CHAN_CTL_DTRIG_WHICH_SHIFT);
    add_general("NUM_CHAN", num_chan);

    if (num_chan > MAX_NUM_CHAN) {
        throw std::runtime_error("max number of supported channels is 12, received " + std::to_string(num_chan) + "\n");
    }
    number_of_channels = num_chan;

    /* channel description and atom description - 24 channels maximum (0-23).
     * the loop being used depends on the struct having no padding and elements in a set order */
    uint32_t p[MAX_NUM_CHAN * REGISTERS_PER_CHAN];
    memcpy(p, &regs.ch0_desc, sizeof p);

    unsigned i;
    auto add_channel = [this, &i](const char *name, auto value) {
        channel_data[name].resize(*number_of_channels);
        add_channel_impl(name, i, value);
    };

    for (i = 0; i < num_chan; i++) {
        uint32_t desc = p[i*REGISTERS_PER_CHAN], adesc = p[i*REGISTERS_PER_CHAN + 1];
        add_channel("INT_WIDTH", (desc & ACQ_CORE_CH0_DESC_INT_WIDTH_MASK) >> ACQ_CORE_CH0_DESC_INT_WIDTH_SHIFT);
        add_channel("NUM_COALESCE", (desc & ACQ_CORE_CH0_DESC_NUM_COALESCE_MASK) >> ACQ_CORE_CH0_DESC_NUM_COALESCE_SHIFT);
        add_channel("NUM_ATOMS", (adesc & ACQ_CORE_CH0_ATOM_DESC_NUM_ATOMS_MASK) >> ACQ_CORE_CH0_ATOM_DESC_NUM_ATOMS_SHIFT);
        add_channel("ATOM_WIDTH", (adesc & ACQ_CORE_CH0_ATOM_DESC_ATOM_WIDTH_MASK) >> ACQ_CORE_CH0_ATOM_DESC_ATOM_WIDTH_SHIFT);

        data_order_done = true;
    }
}

Controller::Controller(struct pcie_bars &bars):
    RegisterController(bars),
    regs_storage(new struct acq_core),
    regs(*regs_storage)
{
}
Controller::~Controller() = default;

void Controller::get_internal_values()
{
    uint32_t channel_desc = bar4_read(&bars, addr + ACQ_CORE_CH0_DESC + 8*channel);
    uint32_t num_coalesce = (channel_desc & ACQ_CORE_CH0_DESC_NUM_COALESCE_MASK) >> ACQ_CORE_CH0_DESC_NUM_COALESCE_SHIFT;
    uint32_t int_width = (channel_desc & ACQ_CORE_CH0_DESC_INT_WIDTH_MASK) >> ACQ_CORE_CH0_DESC_INT_WIDTH_SHIFT;

    /* taken from halcs:
     *  - int_width is a width in bits, so we need to divide by 8 to get a number of bytes
     */
    sample_size = (int_width / 8) * num_coalesce;

    alignment = (ddr3_payload_size > sample_size) ? ddr3_payload_size / sample_size : 1;

    uint32_t channel_atom_desc = bar4_read(&bars, addr + ACQ_CORE_CH0_ATOM_DESC + 8*channel);
    channel_atom_width = (channel_atom_desc & ACQ_CORE_CH0_ATOM_DESC_ATOM_WIDTH_MASK) >> ACQ_CORE_CH0_ATOM_DESC_ATOM_WIDTH_SHIFT;
    channel_num_atoms = (channel_atom_desc & ACQ_CORE_CH0_ATOM_DESC_NUM_ATOMS_MASK) >> ACQ_CORE_CH0_ATOM_DESC_NUM_ATOMS_SHIFT;
}

void Controller::encode_config()
{
    get_internal_values();

    clear_and_insert(regs.acq_chan_ctl, channel, ACQ_CORE_ACQ_CHAN_CTL_WHICH_MASK);

    regs.pre_samples = align_extend(pre_samples, alignment);
    regs.post_samples = align_extend(post_samples, alignment);

    clear_and_insert(regs.shots, number_shots, ACQ_CORE_SHOTS_NB_MASK);

    const static std::unordered_map<std::string_view, std::array<bool, 4>> trigger_types({
        /* CTL_FSM_ACQ_NOW, TRIG_CFG_HW_TRIG_EN, TRIG_CFG_SW_TRIG_EN, TRIG_CFG_HW_TRIG_SEL */
        {"immediate", {true, false, false, false}},
        {"external", {false, true, false, true}},
        {"data-driven", {false, true, false, false}},
        {"software", {false, false, true, false}},
    });
    auto &trigger_setting = trigger_types.at(trigger_type);
    insert_bit(regs.ctl, trigger_setting[0], ACQ_CORE_CTL_FSM_ACQ_NOW);
    insert_bit(regs.trig_cfg, trigger_setting[1], ACQ_CORE_TRIG_CFG_HW_TRIG_EN);
    insert_bit(regs.trig_cfg, trigger_setting[2], ACQ_CORE_TRIG_CFG_SW_TRIG_EN);
    insert_bit(regs.trig_cfg, trigger_setting[3], ACQ_CORE_TRIG_CFG_HW_TRIG_SEL);

    regs.trig_data_thres = data_trigger_threshold;
    insert_bit(regs.trig_cfg, data_trigger_polarity_neg, ACQ_CORE_TRIG_CFG_HW_TRIG_POL);
    clear_and_insert(regs.trig_cfg, data_trigger_sel, ACQ_CORE_TRIG_CFG_INT_TRIG_SEL_MASK);
    clear_and_insert(regs.trig_data_cfg, data_trigger_filt, ACQ_CORE_TRIG_DATA_CFG_THRES_FILT_MASK);
    clear_and_insert(regs.ctl, data_trigger_channel, ACQ_CORE_ACQ_CHAN_CTL_DTRIG_WHICH_MASK);
    regs.trig_dly = trigger_delay;
}

void Controller::write_config()
{
    encode_config();

    /* FIXME: before writing we should make sure nothing is currently running */
    bar4_write_v(&bars, addr, &regs, sizeof regs);
}

void Controller::start_acquisition()
{
    /* FIXME: hardcoded memory size */ bar4_write(&bars, addr + ACQ_CORE_DDR3_END_ADDR, 0x0FFFFFE0);
    insert_bit(regs.ctl, true, ACQ_CORE_CTL_FSM_START_ACQ);
    bar4_write(&bars, addr + ACQ_CORE_CTL, regs.ctl);
}

#define ACQ_CORE_STA_FSM_IDLE (1 << ACQ_CORE_STA_FSM_STATE_SHIFT)
#define COMPLETE_MASK  (ACQ_CORE_STA_FSM_STATE_MASK | ACQ_CORE_STA_FSM_ACQ_DONE | ACQ_CORE_STA_FC_TRANS_DONE | ACQ_CORE_STA_DDR3_TRANS_DONE)
#define COMPLETE_VALUE (ACQ_CORE_STA_FSM_IDLE       | ACQ_CORE_STA_FSM_ACQ_DONE | ACQ_CORE_STA_FC_TRANS_DONE | ACQ_CORE_STA_DDR3_TRANS_DONE)

bool Controller::acquisition_ready()
{
    regs.sta = bar4_read(&bars, addr + ACQ_CORE_STA);
    return (regs.sta & COMPLETE_MASK) == COMPLETE_VALUE;
}

template <class Data>
std::vector<Data> Controller::get_result()
{
    /* intermediate vectors for any size atoms */
    constexpr bool is_signed = std::is_signed_v<Data>;
    std::vector<std::conditional_t<is_signed, int8_t, uint8_t>> v8;
    std::vector<std::conditional_t<is_signed, int16_t, uint16_t>> v16;
    std::vector<std::conditional_t<is_signed, int32_t, uint32_t>> v32;

    /* total number of elements (samples*atoms) */
    size_t elements = (pre_samples + post_samples) * channel_num_atoms;

    size_t trigger_pos = bar4_read(&bars, addr + ACQ_CORE_TRIG_POS);

    /* how we interpret the contents of FPGA memory depends on atom width */
    void *data_pointer;
    size_t data_size;
    auto set_data = [&data_pointer, &data_size, elements](auto &v) {
        v.resize(elements);
        data_pointer = v.data();
        data_size = sizeof v[0];
    };
    switch (channel_atom_width) {
        case 8:
            set_data(v8);
            break;
        case 16:
            set_data(v16);
            break;
        case 32:
            set_data(v32);
            break;
        default:
            throw std::runtime_error("unsupported channel atom width");
    }

    size_t initial_pos = trigger_pos - sample_size * pre_samples;
    size_t total_bytes = elements * data_size;
    /* this is an identity, just want to be sure */
    if (total_bytes != (pre_samples + post_samples) * sample_size)
        throw std::logic_error("elements * data_size different from samples * sample_size");

    /* FIXME: perform circular buffer dance correctly */
    bar2_read_v(&bars, initial_pos, data_pointer, total_bytes);

    auto convert_result = [elements](auto &v) -> std::vector<Data> {
        if constexpr (sizeof v[0] == sizeof(Data)) return v;
        std::vector<Data> result;
        result.resize(elements);
        std::copy(v.begin(), v.end(), result.begin());
        return result;
    };
    switch (channel_atom_width) {
        case 8:
            return convert_result(v8);
            break;
        case 16:
            return convert_result(v16);
            break;
        case 32:
            return convert_result(v32);
            break;
    }

    throw std::logic_error("should be unreachable");
}

template<class Data>
acq_result<Data> Controller::result(std::optional<std::chrono::milliseconds> wait_time)
{
    std::chrono::steady_clock::time_point start_time;
    if (wait_time) start_time = std::chrono::steady_clock::now();

    acq_result<Data> r;
    const acq_status *status;
    do {
        r = result_async<Data>();
    } while (
        (status = std::get_if<acq_status>(&r)) &&
        *status == acq_status::in_progress &&
        (!wait_time || std::chrono::steady_clock::now() - start_time < *wait_time)
    );

    return r;
}
template acq_result<uint32_t> Controller::result(std::optional<std::chrono::milliseconds>);
template acq_result<uint16_t> Controller::result(std::optional<std::chrono::milliseconds>);
template acq_result<uint8_t> Controller::result(std::optional<std::chrono::milliseconds>);
template acq_result<int32_t> Controller::result(std::optional<std::chrono::milliseconds>);
template acq_result<int16_t> Controller::result(std::optional<std::chrono::milliseconds>);
template acq_result<int8_t> Controller::result(std::optional<std::chrono::milliseconds>);

template<class Data>
acq_result<Data> Controller::result_async()
{
    /* XXX: add async DMA API if it ever becomes available */
    if (m_step == acq_step::acq_stop) {
        write_config();
        start_acquisition();
        m_step = acq_step::acq_started;
    }
    if (m_step == acq_step::acq_started) {
        if (acquisition_ready())
            m_step = acq_step::acq_done;
        else
            return acq_status::in_progress;
    }
    if (m_step == acq_step::acq_done) {
        m_step = acq_step::acq_stop;

        return get_result<Data>();
    }

    throw std::logic_error("should be unreachable");
}

template<typename T>
void Controller::print_csv(FILE *f, std::vector<T> &res)
{
    for (unsigned i = 0; i < (pre_samples + post_samples); i++) {
        for (unsigned j = 0; j < channel_num_atoms; j++) {
            char tmp[32];
            auto r = std::to_chars(tmp, tmp + sizeof tmp, res[i * channel_num_atoms + j]);
            *r.ptr = '\0';
            fputs(tmp, f);
            fputc(',', f);
        }
        fputc('\n', f);
    }
}
template void Controller::print_csv<int32_t>(FILE *, std::vector<int32_t> &);
template void Controller::print_csv<uint32_t>(FILE *, std::vector<uint32_t> &);

} /* namespace acq */
