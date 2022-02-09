/*
 * Copyright (C) 2022 CNPEM (cnpem.br)
 * Author: Érico Nogueira <erico.rolim@lnls.br>
 *
 * Released according to the GNU GPL, version 3 or any later version.
 */

#include <cstring>
#include <functional>
#include <unordered_map>

#include "decoders.h"

void RegisterDecoder::read(const struct pcie_bars *bars, size_t address)
{
    bar4_read_u32s(bars, address, read_dest, read_size/4);
}

LnlsBpmAcqCore::LnlsBpmAcqCore()
{
    read_size = sizeof regs;
    read_dest = &regs;
}

enum class PrinterType {
    /* boolean values */
    boolean,
    progress,
    enable,
    /* directly print value */
    value,
    value_hex,
    value_2c, /* 2's complement */
    /* allow it to print its own value */
    custom_function,
};

typedef std::function<void(FILE *, bool, uint32_t)> printing_function;

class Printer {
    PrinterType type;
    const char *name, *description;

    struct {
        const char *truth;
        const char *not_truth;
    } boolean_names;

    printing_function custom_fn;

  public:
    Printer(const char *name, const char *description, PrinterType type):
        type(type), name(name), description(description)
    {
        switch (type) {
            case PrinterType::boolean:
                boolean_names.truth = "true";
                boolean_names.not_truth = "false";
                break;
            case PrinterType::progress:
                boolean_names.truth = "completed";
                boolean_names.not_truth = "in progress";
                break;
            case PrinterType::enable:
                boolean_names.truth = "enabled";
                boolean_names.not_truth = "disabled";
                break;
            default:
                break;
        }
    }

    Printer(const char *name, const char *description, PrinterType type, printing_function custom_fn):
        type(type), name(name), description(description), custom_fn(custom_fn)
    {
    }

    Printer(const char *name, const char *description, PrinterType type, const char *truth, const char *not_truth):
        type(type), name(name), description(description), boolean_names{truth, not_truth}
    {
    }

    void print(FILE *f, bool verbose, uint32_t value) const
    {
        std::string s;
        const char *final_name;
        if (verbose) {
            s = name;
            s += " (";
            s += description;
            s += ")";
            final_name = s.c_str();
        } else {
            final_name = name;
        }

        switch (type) {
            case PrinterType::boolean:
            case PrinterType::progress:
            case PrinterType::enable:
                fprintf(f, "%s: %s\n", final_name, value ? boolean_names.truth : boolean_names.not_truth);
                break;
            case PrinterType::value:
                fprintf(f, "%s: %u\n", final_name, (unsigned)value);
                break;
            case PrinterType::value_hex:
                fprintf(f, "%s: %08X\n", final_name, (unsigned)value);
                break;
            case PrinterType::value_2c:
                fprintf(f, "%s: %d\n", final_name, (int)(int32_t)value);
                break;
            case PrinterType::custom_function:
                custom_fn(f, verbose, value);
                break;
        }
    }
};

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
