/*
 * Copyright (C) 2022 CNPEM (cnpem.br)
 * Author: Érico Nogueira <erico.rolim@lnls.br>
 *
 * Released according to the GNU GPL, version 3 or any later version.
 */

#include <array>
#include <cstring>
#include <stdexcept>
#include <unordered_map>

#include "decoders.h"
#include "printer.h"
#include "util.h"
#include "acq.h"

static unsigned ddr3_payload_size = 32;

LnlsBpmAcqCore::LnlsBpmAcqCore()
{
    read_size = sizeof regs;
    read_dest = &regs;
}

void LnlsBpmAcqCore::print(FILE *f, bool verbose)
{
    #define I(name, ...) {name, {name, __VA_ARGS__}}
    static const std::unordered_map<const char *, Printer> printers({
        I("FSQ_ACQ_NOW", "Acquire data immediately and don't wait for any trigger", PrinterType::boolean, "acquire immediately", "wait on trigger"),
        I("FSM_STATE", "State machine status", PrinterType::custom_function,
            [](FILE *f, bool v, uint32_t value){
                (void)v;
                const char *fsm_states[] = {"IDLE", "PRE_TRIG", "WAIT_TRIG", "POST_TRIG", "DECR_SHOT"};
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
        I("FSM_ACQ_DONE", "FSM acquisition status", PrinterType::progress),
        I("FC_TRANS_DONE", "External flow control transfer status", PrinterType::boolean),
        I("FC_FULL", "External flow control FIFO full status", PrinterType::boolean, "full (data may be lost)", "full"),
        I("DDR3_TRANS_DONE", "DDR3 transfer status", PrinterType::progress),
        I("HW_TRIG_SEL", "Hardware trigger selection", PrinterType::boolean, "external", "internal"),
        I("HW_TRIG_POL", "Hardware trigger polarity", PrinterType::boolean, "negative edge", "positive edge"),
        I("HW_TRIG_EN", "Hardware trigger enable", PrinterType::enable),
        I("SW_TRIG_EN", "Software trigger enable", PrinterType::enable),
        I("INT_TRIG_SEL", "Atom selection for internal trigger", PrinterType::value),
        I("THRES_FILT", "Internal trigger threshold glitch filter", PrinterType::value),
        I("TRIG_DATA_THRES", "Threshold for internal trigger", PrinterType::value_2c),
        I("TRIG_DLY", "Trigger delay value", PrinterType::value),
        I("NB", "Number of shots required in multi-shot mode, one if in single-shot mode", PrinterType::value),
        I("MULTISHOT_RAM_SIZE_IMPL", "MultiShot RAM size reg implemented", PrinterType::boolean),
        I("MULTISHOT_RAM_SIZE", "MultiShot RAM size", PrinterType::value),
        I("TRIG_POS", "Trigger address in DDR memory", PrinterType::value_hex),
        I("PRE_SAMPLES", "Number of requested pre-trigger samples", PrinterType::value),
        I("POST_SAMPLES", "Number of requested post-trigger samples", PrinterType::value),
        I("SAMPLES_CNT", "Samples counter", PrinterType::value),
        I("DDR3_START_ADDR", "Start address in DDR3 memory for the next acquisition", PrinterType::value_hex),
        I("DDR3_END_ADDR", "End address in DDR3 memory for the next acquisition", PrinterType::value_hex),
        I("WHICH", "Acquisition channel selection", PrinterType::value),
        I("DTRIG_WHICH", "Data-driven channel selection", PrinterType::value),
        I("NUM_CHAN", "Number of acquisition channels", PrinterType::value),
        /* per channel info */
        I("INT_WIDTH", "Internal Channel Width", PrinterType::value),
        I("NUM_COALESCE", "Number of coalescing words", PrinterType::value),
        I("NUM_ATOMS", "Number of atoms inside the complete data word", PrinterType::value),
        I("ATOM_WIDTH", "Atom width in bits", PrinterType::value),
    });
    #undef I

    bool v = verbose;
    unsigned indent = 0;
    unsigned t;

    auto print_reg = [f, v, &indent](const char *reg, unsigned offset) {
        if (v) {
            fprintf(f, "%s (0x%02X)\n", reg, offset);
            indent = 4;
        }
    };

    auto print = [f, v, &indent](const char *name, uint32_t value) {
        unsigned i = indent;
        while (i--) fputc(' ', f);
        printers.at(name).print(f, v, value);
    };

    /* control register */
    print_reg("control register", ACQ_CORE_CTL);
    t = regs.ctl;
    print("FSQ_ACQ_NOW", t & ACQ_CORE_CTL_FSM_ACQ_NOW);

    /* status register */
    print_reg("status register", ACQ_CORE_STA);
    t = regs.sta;

    print("FSM_STATE", (t & ACQ_CORE_STA_FSM_STATE_MASK) >> ACQ_CORE_STA_FSM_STATE_SHIFT);
    print("FSM_ACQ_DONE", t & ACQ_CORE_STA_FSM_ACQ_DONE);
    print("FC_TRANS_DONE", t & ACQ_CORE_STA_FC_TRANS_DONE);
    print("FC_FULL", t & ACQ_CORE_STA_FC_FULL);
    print("DDR3_TRANS_DONE", t & ACQ_CORE_STA_DDR3_TRANS_DONE);

    /* trigger configuration */
    print_reg("trigger configuration", ACQ_CORE_TRIG_CFG);
    t = regs.trig_cfg;
    print("HW_TRIG_SEL", t & ACQ_CORE_TRIG_CFG_HW_TRIG_SEL);
    print("HW_TRIG_POL", t & ACQ_CORE_TRIG_CFG_HW_TRIG_POL);
    print("HW_TRIG_EN", t & ACQ_CORE_TRIG_CFG_HW_TRIG_EN);
    print("SW_TRIG_EN", t & ACQ_CORE_TRIG_CFG_SW_TRIG_EN);
    print("INT_TRIG_SEL", ((t & ACQ_CORE_TRIG_CFG_INT_TRIG_SEL_MASK) >> ACQ_CORE_TRIG_CFG_INT_TRIG_SEL_SHIFT) + 1);

    /* trigger data config thresold */
    print_reg("trigger data config threshold", ACQ_CORE_TRIG_DATA_CFG);
    t = regs.trig_data_cfg;
    print("THRES_FILT", (t & ACQ_CORE_TRIG_DATA_CFG_THRES_FILT_MASK) >> ACQ_CORE_TRIG_DATA_CFG_THRES_FILT_SHIFT);

    /* trigger */
    print_reg("trigger data threshold", ACQ_CORE_TRIG_DATA_THRES);
    print("TRIG_DATA_THRES", regs.trig_data_thres);
    print_reg("trigger delay", ACQ_CORE_TRIG_DLY);
    print("TRIG_DLY", regs.trig_dly);

    /* number of shots */
    print_reg("number of shots", ACQ_CORE_SHOTS);
    t = regs.shots;
    print("NB", (t & ACQ_CORE_SHOTS_NB_MASK) >> ACQ_CORE_SHOTS_NB_SHIFT);
    print("MULTISHOT_RAM_SIZE_IMPL", t & ACQ_CORE_SHOTS_MULTISHOT_RAM_SIZE_IMPL);
    print("MULTISHOT_RAM_SIZE", (t & ACQ_CORE_SHOTS_MULTISHOT_RAM_SIZE_MASK) >> ACQ_CORE_SHOTS_MULTISHOT_RAM_SIZE_SHIFT);

    /* trigger address register */
    print_reg("trigger address register", ACQ_CORE_TRIG_POS);
    print("TRIG_POS", regs.trig_pos);

    /* samples */
    print_reg("pre-trigger samples", ACQ_CORE_PRE_SAMPLES);
    print("PRE_SAMPLES", regs.pre_samples);
    print_reg("post-trigger samples", ACQ_CORE_POST_SAMPLES);
    print("POST_SAMPLES", regs.post_samples);
    print_reg("sample counter", ACQ_CORE_SAMPLES_CNT);
    print("SAMPLES_CNT", regs.samples_cnt);

    /* ddr3 addresses */
    print_reg("DDR3 Start Address", ACQ_CORE_DDR3_START_ADDR);
    print("DDR3_START_ADDR", regs.ddr3_start_addr);
    print_reg("DDR3 End Address", ACQ_CORE_DDR3_END_ADDR);
    print("DDR3_END_ADDR", regs.ddr3_end_addr);

    /* acquisition channel control */
    print_reg("acquisition channel control", ACQ_CORE_ACQ_CHAN_CTL);
    t = regs.acq_chan_ctl;
    /* will be used to determine how many channels to show in the next block */
    unsigned num_chan = (t & ACQ_CORE_ACQ_CHAN_CTL_NUM_CHAN_MASK) >> ACQ_CORE_ACQ_CHAN_CTL_NUM_CHAN_SHIFT;
    print("WHICH", (t & ACQ_CORE_ACQ_CHAN_CTL_WHICH_MASK) >> ACQ_CORE_ACQ_CHAN_CTL_WHICH_SHIFT);
    print("DTRIG_WHICH", (t & ACQ_CORE_ACQ_CHAN_CTL_DTRIG_WHICH_MASK) >> ACQ_CORE_ACQ_CHAN_CTL_DTRIG_WHICH_SHIFT);
    print("NUM_CHAN", num_chan);

#define MAX_NUM_CHAN 24
    if (num_chan > MAX_NUM_CHAN) {
        fprintf(f, "ERROR: max number of supported channels is %d, received %u\n", MAX_NUM_CHAN, num_chan);
        return;
    }

    /* channel description and atom description - 24 channels maximum (0-23).
     * the loop being used depends on the struct having no padding and elements in a set order */
    uint32_t p[MAX_NUM_CHAN];
    memcpy(p, &regs.ch0_desc, sizeof p);
    for (unsigned i = 0; i < num_chan; i++) {
        uint32_t desc = p[i*2], adesc = p[i*2 + 1];
        fprintf(f, "channel %u description and atom description (%02X and %02X):\n",
            i, (unsigned)ACQ_CORE_CH0_DESC + 8*i, (unsigned)ACQ_CORE_CH0_ATOM_DESC + 8*i);
        indent = 4;
        print("INT_WIDTH", (desc & ACQ_CORE_CH0_DESC_INT_WIDTH_MASK) >> ACQ_CORE_CH0_DESC_INT_WIDTH_SHIFT);
        print("NUM_COALESCE", (desc & ACQ_CORE_CH0_DESC_NUM_COALESCE_MASK) >> ACQ_CORE_CH0_DESC_NUM_COALESCE_SHIFT);
        print("NUM_ATOMS", (adesc & ACQ_CORE_CH0_ATOM_DESC_NUM_ATOMS_MASK) >> ACQ_CORE_CH0_ATOM_DESC_NUM_ATOMS_SHIFT);
        print("ATOM_WIDTH", (adesc & ACQ_CORE_CH0_ATOM_DESC_ATOM_WIDTH_MASK) >> ACQ_CORE_CH0_ATOM_DESC_ATOM_WIDTH_SHIFT);
    }
}

void LnlsBpmAcqCoreController::get_internal_values()
{
    uint32_t channel_desc = bar4_read(bars, addr + ACQ_CORE_CH0_DESC + 8*channel);
    uint32_t num_coalesce = (channel_desc & ACQ_CORE_CH0_DESC_NUM_COALESCE_MASK) >> ACQ_CORE_CH0_DESC_NUM_COALESCE_SHIFT;
    uint32_t int_width = (channel_desc & ACQ_CORE_CH0_DESC_INT_WIDTH_MASK) >> ACQ_CORE_CH0_DESC_INT_WIDTH_SHIFT;

    /* taken from halcs:
     *  - int_width is a width in bits, so we need to divide by 8 to get a number of bytes
     */
    sample_size = (int_width / 8) * num_coalesce;

    alignment = (ddr3_payload_size < sample_size) ? ddr3_payload_size / sample_size : 1;

    uint32_t channel_atom_desc = bar4_read(bars, addr + ACQ_CORE_CH0_ATOM_DESC + 8*channel);
    channel_atom_width = (channel_atom_desc & ACQ_CORE_CH0_ATOM_DESC_ATOM_WIDTH_MASK) >> ACQ_CORE_CH0_ATOM_DESC_ATOM_WIDTH_SHIFT;
    channel_num_atoms = (channel_atom_desc & ACQ_CORE_CH0_ATOM_DESC_NUM_ATOMS_MASK) >> ACQ_CORE_CH0_ATOM_DESC_NUM_ATOMS_SHIFT;
}

void LnlsBpmAcqCoreController::encode_config()
{
    get_internal_values();

    clear_and_insert(regs.acq_chan_ctl, channel, ACQ_CORE_ACQ_CHAN_CTL_WHICH_MASK, ACQ_CORE_ACQ_CHAN_CTL_WHICH_SHIFT);

    regs.pre_samples = align_extend(pre_samples, alignment);
    regs.post_samples = align_extend(post_samples, alignment);

    clear_and_insert(regs.shots, number_shots, ACQ_CORE_SHOTS_NB_MASK, ACQ_CORE_SHOTS_NB_SHIFT);

    const static std::unordered_map<std::string, std::array<bool, 4>> trigger_types({
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
    clear_and_insert(regs.trig_cfg, data_trigger_sel, ACQ_CORE_TRIG_CFG_INT_TRIG_SEL_MASK, ACQ_CORE_TRIG_CFG_INT_TRIG_SEL_SHIFT);
    clear_and_insert(regs.trig_data_cfg, data_trigger_filt, ACQ_CORE_TRIG_DATA_CFG_THRES_FILT_MASK, ACQ_CORE_TRIG_DATA_CFG_THRES_FILT_SHIFT);
    clear_and_insert(regs.ctl, data_trigger_channel, ACQ_CORE_ACQ_CHAN_CTL_DTRIG_WHICH_MASK, ACQ_CORE_ACQ_CHAN_CTL_DTRIG_WHICH_SHIFT);
    regs.trig_dly = trigger_delay;
}

void LnlsBpmAcqCoreController::write_config()
{
    encode_config();

    /* FIXME: before writing we should make sure nothing is currently running */
    bar4_write_u32s(bars, addr, &regs, sizeof regs / 4);
}

void LnlsBpmAcqCoreController::start_acquisition()
{
    insert_bit(regs.ctl, true, ACQ_CORE_CTL_FSM_START_ACQ);
    bar4_write(bars, addr + ACQ_CORE_CTL, regs.ctl);
    /* FIXME: hardcoded memory size */ bar4_write(bars, addr + ACQ_CORE_DDR3_END_ADDR, 0x0FFFFFE0);
}

#define ACQ_CORE_STA_FSM_IDLE (1 << ACQ_CORE_STA_FSM_STATE_SHIFT)
#define COMPLETE_MASK  (ACQ_CORE_STA_FSM_STATE_MASK | ACQ_CORE_STA_FSM_ACQ_DONE | ACQ_CORE_STA_FC_TRANS_DONE | ACQ_CORE_STA_DDR3_TRANS_DONE)
#define COMPLETE_VALUE (ACQ_CORE_STA_FSM_IDLE       | ACQ_CORE_STA_FSM_ACQ_DONE | ACQ_CORE_STA_FC_TRANS_DONE | ACQ_CORE_STA_DDR3_TRANS_DONE)

bool LnlsBpmAcqCoreController::acquisition_ready()
{
    regs.sta = bar4_read(bars, addr + ACQ_CORE_STA);
    return (regs.sta & COMPLETE_MASK) == COMPLETE_VALUE;
}

void LnlsBpmAcqCoreController::wait_for_acquisition()
{
    while (!acquisition_ready());
}

acq_status LnlsBpmAcqCoreController::wait_for_acquisition(std::chrono::milliseconds wait_time)
{
    auto start = std::chrono::steady_clock::now();

    while (!acquisition_ready()) {
        if (std::chrono::steady_clock::now() - start > wait_time)
            return acq_status::timeout;
    }

    return acq_status::success;
}

std::vector<uint32_t> LnlsBpmAcqCoreController::result_unsigned()
{
    /* intermediate vectors for smaller atoms */
    std::vector<uint8_t> v8;
    std::vector<uint16_t> v16;

    std::vector<uint32_t> result;

    /* total length to be read */
    size_t length = (pre_samples + post_samples) * channel_num_atoms;

    size_t trigger_pos = bar4_read(bars, addr + ACQ_CORE_TRIG_POS);

    /* how we interpret the contents of FPGA memory depends on atom width */
    void *data_pointer;
    size_t data_size;
    switch (channel_atom_width) {
        case 8:
            v8.resize(length);
            data_pointer = v8.data();
            data_size = 1;
            break;
        case 16:
            v16.resize(length);
            data_pointer = v16.data();
            data_size = 2;
            break;
        case 32:
            result.resize(length);
            data_pointer = result.data();
            data_size = 4;
            break;
        default:
            throw std::runtime_error("unsupported channel atom width");
    }
    bar2_read_v(bars, trigger_pos - sample_size * pre_samples, data_pointer, length * data_size);

    /* stuff values back into result, if necessary - no effect if vectors are empty.
     * the reserve() call should also have no effect if resize() was called above.
     * since these are unsigned, we don't need to care about signal extension. */
    result.reserve(length);
    for (auto v: v8) result.push_back(v);
    for (auto v: v16) result.push_back(v);

    return result;
}

std::vector<int32_t> LnlsBpmAcqCoreController::result_signed()
{
    std::vector<uint32_t> unsigned_results = result_unsigned();
    std::vector<int32_t> result;
    result.reserve(unsigned_results.size());

    sign_extension_fn &chosen_se = sign_extend_function(channel_atom_width);

    for (auto v: unsigned_results) result.push_back(chosen_se(v));

    return result;
}
