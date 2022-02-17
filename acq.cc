/*
 * Copyright (C) 2022 CNPEM (cnpem.br)
 * Author: Érico Nogueira <erico.rolim@lnls.br>
 *
 * Released according to the GNU GPL, version 3 or any later version.
 */

#include <array>
#include <cstring>
#include <unordered_map>

#include "printer.h"
#include "decoders.h"
#include "acq.h"

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
        I("FC_FULL", "External flow control FIFO full status", PrinterType::progress, "full (data may be lost)", "full"),
        I("DDR3_TRANS_DONE", "DDR3 transfer status", PrinterType::progress),
        I("HW_TRIG_SEL", "Hardware trigger selection", PrinterType::boolean, "external", "internal"),
        I("HW_TRIG_POL", "Hardware trigger polarity", PrinterType::boolean, "negative edge", "positive edge"),
        I("HW_TRIG_EN", "Hardware trigger enable", PrinterType::enable),
        I("SW_TRIG_EN", "Software trigger enable", PrinterType::enable),
        I("INT_TRIG_SEL", "Channel selection for internal trigger", PrinterType::value),
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
            i, (unsigned)ACQ_CORE_CH0_DESC + 4*i, (unsigned)ACQ_CORE_CH0_ATOM_DESC + 4*i);
        indent = 4;
        print("INT_WIDTH", (desc & ACQ_CORE_CH0_DESC_INT_WIDTH_MASK) >> ACQ_CORE_CH0_DESC_INT_WIDTH_SHIFT);
        print("NUM_COALESCE", (desc & ACQ_CORE_CH0_DESC_NUM_COALESCE_MASK) >> ACQ_CORE_CH0_DESC_NUM_COALESCE_SHIFT);
        print("NUM_ATOMS", (adesc & ACQ_CORE_CH0_ATOM_DESC_NUM_ATOMS_MASK) >> ACQ_CORE_CH0_ATOM_DESC_NUM_ATOMS_SHIFT);
        print("ATOM_WIDTH", (adesc & ACQ_CORE_CH0_ATOM_DESC_ATOM_WIDTH_MASK) >> ACQ_CORE_CH0_ATOM_DESC_ATOM_WIDTH_SHIFT);
    }
}

static void clear_and_insert(uint32_t &dest, unsigned value, uint32_t mask, unsigned shift)
{
    dest &= UINT32_MAX & ~mask;
    dest |= (value << shift) & mask;
}

static void insert_bit(uint32_t &dest, bool value, uint32_t mask)
{
    if (value)
        dest |= mask;
    else
        dest &= ~mask;
}

void LnlsBpmAcqCoreController::encode_config()
{
    clear_and_insert(regs.acq_chan_ctl, channel, ACQ_CORE_ACQ_CHAN_CTL_WHICH_MASK, ACQ_CORE_ACQ_CHAN_CTL_WHICH_SHIFT);
    regs.pre_samples = pre_samples;
    regs.post_samples = post_samples;
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
    insert_bit(regs.trig_cfg, !data_trigger_polarity_p, ACQ_CORE_TRIG_CFG_HW_TRIG_POL);
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

void LnlsBpmAcqCoreController::wait_for_acquisition()
{
    do {
        regs.sta = bar4_read(bars, addr + ACQ_CORE_STA);
    } while ((regs.sta & COMPLETE_MASK) != COMPLETE_VALUE);
}
