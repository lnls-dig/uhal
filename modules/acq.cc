/*
 * Copyright (C) 2022 CNPEM (cnpem.br)
 * Author: Érico Nogueira <erico.rolim@lnls.br>
 *
 * Released according to the GNU GPL, version 3 or any later version.
 */

#include <algorithm>
#include <array>
#include <bit>
#include <charconv>
#include <cstring>
#include <stdexcept>
#include <thread>
#include <type_traits>
#include <unordered_map>

#include "pcie.h"
#include "printer.h"
#include "util.h"
#include "modules/acq.h"

namespace {

const int max_acq_cores = 8;
const size_t acq_ram = 2UL * 1024UL * 1024UL * 1024UL;
const size_t acq_ram_per_core = acq_ram / max_acq_cores;

class MemoryAllocator {
    std::unordered_map<struct pcie_bars *, int> counts;

    MemoryAllocator() {}

  public:
    std::array<size_t, 2> get_range(struct pcie_bars &bars)
    {
        auto index = counts[&bars];
        counts[&bars]++;

        if (counts[&bars] >= max_acq_cores) {
            throw std::logic_error("we only support up to " + std::to_string(max_acq_cores) + " acq");
        }

        return {index * acq_ram_per_core, (index+1) * acq_ram_per_core};
    }

    static MemoryAllocator &get_memory_allocator()
    {
        /* singleton instance */
        static MemoryAllocator instance;
        return instance;
    }
};

}

namespace acq {

#include "hw/wb_acq_core_regs.h"

#define MAX_NUM_CHAN 24
#define REGISTERS_PER_CHAN 2

namespace {
    const unsigned ddr3_payload_size = 32;
    using namespace std::chrono_literals;
    const auto acq_loop_time = 1ms;

    struct sdb_device_info ref_devinfo = {
        .vendor_id = LNLS_VENDORID,
        .device_id = ACQ_DEVID,
        .abi_ver_major = 2
    };
}


Core::Core(struct pcie_bars &bars):
    RegisterDecoder(bars, ref_devinfo, {
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

    add_general("FSM_STATE", extract_value<uint32_t>(t, ACQ_CORE_STA_FSM_STATE_MASK));
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
    add_general("INT_TRIG_SEL", extract_value<uint32_t>(t, ACQ_CORE_TRIG_CFG_INT_TRIG_SEL_MASK) + 1);

    /* trigger data config thresold */
    t = regs.trig_data_cfg;
    add_general("THRES_FILT", extract_value<uint32_t>(t, ACQ_CORE_TRIG_DATA_CFG_THRES_FILT_MASK));

    /* trigger */
    add_general("TRIG_DATA_THRES", (int32_t)regs.trig_data_thres);
    add_general("TRIG_DLY", regs.trig_dly);

    /* number of shots */
    t = regs.shots;
    add_general("NB", extract_value<uint32_t>(t, ACQ_CORE_SHOTS_NB_MASK));
    add_general("MULTISHOT_RAM_SIZE_IMPL", t & ACQ_CORE_SHOTS_MULTISHOT_RAM_SIZE_IMPL);
    add_general("MULTISHOT_RAM_SIZE", extract_value<uint32_t>(t, ACQ_CORE_SHOTS_MULTISHOT_RAM_SIZE_MASK));

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
    unsigned num_chan = extract_value<uint32_t>(t, ACQ_CORE_ACQ_CHAN_CTL_NUM_CHAN_MASK);
    add_general("WHICH", extract_value<uint32_t>(t, ACQ_CORE_ACQ_CHAN_CTL_WHICH_MASK));
    add_general("DTRIG_WHICH", extract_value<uint32_t>(t, ACQ_CORE_ACQ_CHAN_CTL_DTRIG_WHICH_MASK));
    add_general("NUM_CHAN", num_chan);

    if (num_chan > MAX_NUM_CHAN) {
        throw std::runtime_error("max number of supported channels is 12, received " + std::to_string(num_chan) + "\n");
    }
    number_of_channels = num_chan;

    /* channel description and atom description - 24 channels maximum (0-23).
     * the loop being used depends on the struct having no padding and elements in a set order */
    uint32_t p[MAX_NUM_CHAN * REGISTERS_PER_CHAN];
    memcpy(p, &regs.ch0_desc, sizeof p);

    for (unsigned i = 0; i < num_chan; i++) {
        uint32_t desc = p[i*REGISTERS_PER_CHAN], adesc = p[i*REGISTERS_PER_CHAN + 1];
        add_channel("INT_WIDTH", i, extract_value<uint32_t>(desc, ACQ_CORE_CH0_DESC_INT_WIDTH_MASK));
        add_channel("NUM_COALESCE", i, extract_value<uint32_t>(desc, ACQ_CORE_CH0_DESC_NUM_COALESCE_MASK));
        add_channel("NUM_ATOMS", i, extract_value<uint32_t>(adesc, ACQ_CORE_CH0_ATOM_DESC_NUM_ATOMS_MASK));
        add_channel("ATOM_WIDTH", i, extract_value<uint32_t>(adesc, ACQ_CORE_CH0_ATOM_DESC_ATOM_WIDTH_MASK));

        data_order_done = true;
    }
}

Controller::Controller(struct pcie_bars &bars):
    RegisterController(bars),
    regs_storage(new struct acq_core()),
    regs(*regs_storage)
{
    auto addrs = MemoryAllocator::get_memory_allocator().get_range(bars);
    ram_start_addr = addrs[0];
    ram_end_addr = addrs[1];
}
Controller::~Controller() = default;

void Controller::set_devinfo_callback()
{
    /* we want a consistent view of the world, and that includes no acquisitions
     * we don't know about. it's too complicated to gather information about
     * running acquisitions, as well, so that's not supported for now */
    stop_acquisition();
}

struct BadSampleSize: std::logic_error {
    using std::logic_error::logic_error;
};
struct BadAtomWidth: std::logic_error {
    BadAtomWidth(): std::logic_error("we only support 8, 16 and 32 bit atoms") {}
};
struct BadPostSamples: std::runtime_error {
    BadPostSamples(): std::runtime_error("post samples should be 0") {}
};
struct TooManySamples: std::runtime_error {
    TooManySamples(): std::runtime_error("too many samples") {}
};
struct NoSamples: std::runtime_error {
    NoSamples(): std::runtime_error("no samples requested") {}
};

void Controller::get_internal_values()
{
    uint32_t channel_desc = bar4_read(&bars, addr + ACQ_CORE_CH0_DESC + 8*channel);
    uint32_t num_coalesce = extract_value<uint32_t>(channel_desc, ACQ_CORE_CH0_DESC_NUM_COALESCE_MASK);
    uint32_t int_width = extract_value<uint32_t>(channel_desc, ACQ_CORE_CH0_DESC_INT_WIDTH_MASK);

    /* int_width is in bits, so needs to be converted to bytes */
    sample_size = (int_width / 8) * num_coalesce;
    if (std::popcount(sample_size) != 1)
        throw BadSampleSize("we can only handle power of 2 sample sizes");

    alignment = (ddr3_payload_size > sample_size) ? ddr3_payload_size / sample_size : 1;

    uint32_t channel_atom_desc = bar4_read(&bars, addr + ACQ_CORE_CH0_ATOM_DESC + 8*channel);
    channel_atom_width = extract_value<uint32_t>(channel_atom_desc, ACQ_CORE_CH0_ATOM_DESC_ATOM_WIDTH_MASK);
    channel_num_atoms = extract_value<uint32_t>(channel_atom_desc, ACQ_CORE_CH0_ATOM_DESC_NUM_ATOMS_MASK);

    if (channel_atom_width != 8 && channel_atom_width != 16 && channel_atom_width != 32)
        throw BadAtomWidth();
}

void Controller::encode_config()
{
    get_internal_values();
    acq_pre_samples = pre_samples;
    acq_post_samples = post_samples;

    clear_and_insert(regs.acq_chan_ctl, channel, ACQ_CORE_ACQ_CHAN_CTL_WHICH_MASK);

    auto align_extend = [](unsigned value, unsigned alignment, bool can_be_zero) -> unsigned {
        if (value == 0) {
            if (can_be_zero) return 0;
            else return alignment;
        }

        unsigned extra = value % alignment;
        if (extra)
            return value + (alignment - extra);
        else
            return value;
    };

    if (post_samples + pre_samples == 0)
        throw NoSamples();

    if (post_samples != 0 && trigger_type == "now")
        throw BadPostSamples();

    const size_t pre_samples_aligned = align_extend(pre_samples, alignment, false);
    const size_t post_samples_aligned = align_extend(post_samples, alignment, trigger_type == "now");

    const size_t max_samples = (ram_end_addr - ram_start_addr) / sample_size;
    if (pre_samples_aligned + post_samples_aligned >= max_samples)
        throw TooManySamples();

    regs.pre_samples = pre_samples_aligned;
    regs.post_samples = post_samples_aligned;

    clear_and_insert(regs.shots, number_shots, ACQ_CORE_SHOTS_NB_MASK);

    const static std::unordered_map<std::string_view, std::array<bool, 4>> trigger_types({
        /* CTL_FSM_ACQ_NOW, TRIG_CFG_HW_TRIG_EN, TRIG_CFG_SW_TRIG_EN, TRIG_CFG_HW_TRIG_SEL */
        {"now", {true, false, false, false}},
        {"external", {false, true, false, true}},
        {"data", {false, true, false, false}},
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

    regs.ddr3_start_addr = ram_start_addr;
    /* we want the last sample to stay in the reserved area */
    regs.ddr3_end_addr = ram_end_addr - sample_size;
}

void Controller::write_config()
{
    encode_config();
    bar4_write_v(&bars, addr, &regs, sizeof regs);
}

acq_error Controller::start_acquisition()
{
    if (m_step != acq_step::stop)
        throw std::logic_error("acquisition should only be started if it's not currently running");

    try {
        write_config();
    } catch (BadPostSamples &e) {
        return acq_error::bad_post_samples;
    } catch (TooManySamples &e) {
        return acq_error::too_many_samples;
    } catch (NoSamples &e) {
        return acq_error::no_samples;
    } catch (std::exception &e) {
        /* catch-all for exceptions that aren't handled in a specific way */
        fprintf(stderr, "start_acquisition error: %s\n", e.what());
        return acq_error::error;
    }

    m_step = acq_step::started;

    insert_bit(regs.ctl, true, ACQ_CORE_CTL_FSM_START_ACQ);
    bar4_write(&bars, addr + ACQ_CORE_CTL, regs.ctl);

    /* clear start for next acquisition */
    insert_bit(regs.ctl, false, ACQ_CORE_CTL_FSM_START_ACQ);

    return acq_error::success;
}

void Controller::stop_acquisition()
{
    m_step = acq_step::stop;

    insert_bit(regs.ctl, true, ACQ_CORE_CTL_FSM_STOP_ACQ);
    bar4_write(&bars, addr + ACQ_CORE_CTL, regs.ctl);

    /* clear bit */
    insert_bit(regs.ctl, false, ACQ_CORE_CTL_FSM_STOP_ACQ);
}

#define ACQ_CORE_STA_FSM_IDLE (1 << ACQ_CORE_STA_FSM_STATE_SHIFT)
#define COMPLETE_MASK  (ACQ_CORE_STA_FSM_STATE_MASK | ACQ_CORE_STA_FSM_ACQ_DONE | ACQ_CORE_STA_FC_TRANS_DONE | ACQ_CORE_STA_DDR3_TRANS_DONE)
#define COMPLETE_VALUE (ACQ_CORE_STA_FSM_IDLE       | ACQ_CORE_STA_FSM_ACQ_DONE | ACQ_CORE_STA_FC_TRANS_DONE | ACQ_CORE_STA_DDR3_TRANS_DONE)

bool Controller::acquisition_ready()
{
    regs.sta = bar4_read(&bars, addr + ACQ_CORE_STA);
    return (regs.sta & COMPLETE_MASK) == COMPLETE_VALUE;
}

/* XXX: evaluate performance difference from reusing a vector from the caller
 * instead of always creating a new one */
template <class Data>
std::vector<Data> Controller::get_result()
{
    if (m_step != acq_step::done)
        throw std::logic_error("get_result() called in the wrong step");
    m_step = acq_step::stop;

    /* total number of elements (samples*atoms) */
    size_t total_samples = acq_pre_samples + acq_post_samples,
           elements = total_samples * channel_num_atoms;
    size_t total_bytes = elements * (channel_atom_width/8);

    /* this is an identity, just want to be sure */
    if (total_bytes != (total_samples) * sample_size)
        throw std::logic_error("elements * channel_atom_width/8 different from samples * sample_size");

    /* how we interpret the contents of FPGA memory depends on atom width,
     * so we need intermediate vectors for any size atoms */
    constexpr bool is_signed = std::is_signed_v<Data>;
    std::vector<std::conditional_t<is_signed, int8_t, uint8_t>> v8;
    std::vector<std::conditional_t<is_signed, int16_t, uint16_t>> v16;
    std::vector<std::conditional_t<is_signed, int32_t, uint32_t>> v32;

    /* allows the code below to be generic and only use data_pointer */
    void *data_pointer = nullptr;
    auto set_data = [&data_pointer, elements](auto &v) {
        v.resize(elements);
        data_pointer = v.data();
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
    }

    size_t trigger_pos = bar4_read(&bars, addr + ACQ_CORE_TRIG_POS);
    if (trigger_pos < ram_start_addr || trigger_pos >= ram_end_addr)
        throw std::runtime_error("trigger_pos is outside of valid address range");

    /* these functions convert bytes (as an offset from ram_start_addr) into
     * amount of samples */
    auto bytes2samples = [this](ssize_t v) -> ssize_t { return v / sample_size; };
    auto samples2bytes = [this](ssize_t v) -> ssize_t { return v * sample_size; };
    const ssize_t max_bytes = ram_end_addr - ram_start_addr,
          max_samples = bytes2samples(max_bytes);
    /* in order to simplify working with the acquisition circular buffer, think
     * first in terms of indexes into a circular buffer */
    const ssize_t trigger_index = bytes2samples(trigger_pos - ram_start_addr);
    ssize_t start_index = trigger_index - acq_pre_samples;
    /* trigger_index is the position of the first post_sample, so we need to
     * subtract one to get the end_index */
    ssize_t end_index = trigger_index + acq_post_samples - 1;
    /* convert from negative or >max_samples indexes */
    start_index %= max_samples;
    end_index %= max_samples;

    /* we have to use >= to account for acquisitions with just one sample */
    if (end_index >= start_index) {
        /* copy when the acquisition sits in a contiguous segment in RAM */
        bar2_read_v(&bars, ram_start_addr + samples2bytes(start_index), data_pointer, total_bytes);
    } else {
        /* copy when the acquisition wraps around the buffer */
        const ssize_t first_read = samples2bytes(max_samples - start_index);
        /* copy from the start of the acquisition to the end of the buffer */
        bar2_read_v(&bars, ram_start_addr + samples2bytes(start_index), data_pointer, first_read);
        /* copy from the start of the buffer to the end of the acquisition */
        bar2_read_v(&bars, ram_start_addr, (unsigned char *)data_pointer + first_read, total_bytes - first_read);
    }

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
std::vector<Data> Controller::result(std::optional<std::chrono::milliseconds> wait_time)
{
    start_acquisition();

    std::chrono::steady_clock::time_point start_time;
    if (wait_time) start_time = std::chrono::steady_clock::now();

    acq_status r;
    do {
        r = get_acq_status();
        if (r == acq_status::in_progress) std::this_thread::sleep_for(acq_loop_time);
    } while (
        r == acq_status::in_progress &&
        (!wait_time || std::chrono::steady_clock::now() - start_time < *wait_time)
    );

    if (r == acq_status::success)
        return get_result<Data>();

    throw std::runtime_error("acquisition failed");
}
template std::vector<uint32_t> Controller::result(std::optional<std::chrono::milliseconds>);
template std::vector<uint16_t> Controller::result(std::optional<std::chrono::milliseconds>);
template std::vector<uint8_t> Controller::result(std::optional<std::chrono::milliseconds>);
template std::vector<int32_t> Controller::result(std::optional<std::chrono::milliseconds>);
template std::vector<int16_t> Controller::result(std::optional<std::chrono::milliseconds>);
template std::vector<int8_t> Controller::result(std::optional<std::chrono::milliseconds>);

acq_status Controller::get_acq_status()
{
    if (m_step == acq_step::stop) {
        return acq_status::idle;
    }
    if (m_step == acq_step::started) {
        if (acquisition_ready())
            m_step = acq_step::done;
        else
            return acq_status::in_progress;
    }
    if (m_step == acq_step::done) {
        return acq_status::success;
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
