/*
 * Copyright (C) 2022 CNPEM (cnpem.br)
 * Author: Érico Nogueira <erico.rolim@lnls.br>
 *
 * Released according to the GNU GPL, version 3 or any later version.
 */

#include <cstdio>
#include <cstring>

#include "pcie.h"
#include "wb_acq_core_regs.h"

static void print_bool(FILE *f, const char *name, int value, const char *truth, const char *not_truth)
{
    fprintf(f, "%s: %s\n", name, value ? (truth?truth:"true") : (not_truth?not_truth:"false"));
}

static void print_progress(FILE *f, const char *name, int value)
{
    print_bool(f, name, value, "completed", "in progress");
}

static void print_enable(FILE *f, const char *name, int value)
{
    print_bool(f, name, value, "enabled", "disabled");
}

static void print_value(FILE *f, const char *name, unsigned value)
{
    fprintf(f, "%s: %u\n", name, value);
}

void decode_registers_print(struct acq_core *acq, FILE *f)
{
    unsigned t, t2;

    /* control register */
    t = acq->ctl;
    print_bool(f, "FSQ_ACQ_NOW", t & ACQ_CORE_CTL_FSM_ACQ_NOW, "acquire immediately", "wait on trigger");

    /* status register */
    t = acq->sta;

    fprintf(f, "FSM_STATE: ");
    const char *fsm_states[] = {"IDLE", "PRE_TRIG", "WAIT_TRIG", "POST_TRIG", "DECR_SHOT"};
    switch ((t2 = (t & ACQ_CORE_STA_FSM_STATE_MASK) >> ACQ_CORE_STA_FSM_STATE_SHIFT)) {
        case 0:
        case 6:
        case 7:
            fprintf(f, "illegal (%u)", t2);
            break;
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
            fprintf(f, "%s", fsm_states[t2-1]);
            break;
    }
    fputc('\n', f);

    print_progress(f, "FSM_ACQ_DONE", t & ACQ_CORE_STA_FSM_ACQ_DONE);
    print_progress(f, "FC_TRANS_DONE", t & ACQ_CORE_STA_FC_TRANS_DONE);
    print_bool(f, "FC_FULL", t & ACQ_CORE_STA_FC_FULL, "full (data may be lost)", "not full");
    print_progress(f, "DDR3_TRANS_DONE", t & ACQ_CORE_STA_DDR3_TRANS_DONE);

    /* trigger configuration */
    t = acq->trig_cfg;
    print_bool(f, "HW_TRIG_SEL", t & ACQ_CORE_TRIG_CFG_HW_TRIG_SEL, "external", "internal");
    print_bool(f, "HW_TRIG_POL", t & ACQ_CORE_TRIG_CFG_HW_TRIG_POL, "negative edge", "positive edge");
    print_enable(f, "HW_TRIG_EN", t & ACQ_CORE_TRIG_CFG_HW_TRIG_EN);
    print_enable(f, "SW_TRIG_EN", t & ACQ_CORE_TRIG_CFG_SW_TRIG_EN);
    print_value(f, "INT_TRIG_SEL channel", ((t & ACQ_CORE_TRIG_CFG_INT_TRIG_SEL_MASK) >> ACQ_CORE_TRIG_CFG_INT_TRIG_SEL_SHIFT) + 1);

    /* trigger data config thresold */
    t = acq->trig_data_cfg;
    print_value(f, "THRES_FILT", (t & ACQ_CORE_TRIG_DATA_CFG_THRES_FILT_MASK) >> ACQ_CORE_TRIG_DATA_CFG_THRES_FILT_SHIFT);

    /* trigger */
    print_value(f, "TRIG_DATA_THRES", acq->trig_data_thres);
    print_value(f, "TRIG_DLY", acq->trig_dly);

    /* number of shots */
    t = acq->shots;
    print_value(f, "NB", (t & ACQ_CORE_SHOTS_NB_MASK) >> ACQ_CORE_SHOTS_NB_SHIFT);
    print_bool(f, "MULTISHOT_RAM_SIZE_IMPL", t & ACQ_CORE_SHOTS_MULTISHOT_RAM_SIZE_IMPL, "multishot RAM size reg is implemented", "multishot RAM size reg is not implemented");
    print_value(f, "MULTISHOT_RAM_SIZE", (t & ACQ_CORE_SHOTS_MULTISHOT_RAM_SIZE_MASK) >> ACQ_CORE_SHOTS_MULTISHOT_RAM_SIZE_SHIFT);

    /* trigger address register */
    print_value(f, "TRIG_POS", acq->trig_pos);
    
    /* samples */
    print_value(f, "PRE_SAMPLES", acq->pre_samples);
    print_value(f, "POST_SAMPLES", acq->post_samples);
    print_value(f, "SAMPLES_CNT", acq->samples_cnt);

    /* ddr3 addresses */
    print_value(f, "DDR3_START_ADDR", acq->ddr3_start_addr);
    print_value(f, "DDR3_END_ADDR", acq->ddr3_end_addr);

    /* acquisition channel control */
    t = acq->acq_chan_ctl;
    /* will be used to determine how many channels to show in the next block */
    unsigned num_chan = (t & ACQ_CORE_ACQ_CHAN_CTL_NUM_CHAN_MASK) >> ACQ_CORE_ACQ_CHAN_CTL_NUM_CHAN_SHIFT;
    print_value(f, "WHICH", (t & ACQ_CORE_ACQ_CHAN_CTL_WHICH_MASK) >> ACQ_CORE_ACQ_CHAN_CTL_WHICH_SHIFT);
    print_value(f, "DTRIG_WHICH", (t & ACQ_CORE_ACQ_CHAN_CTL_DTRIG_WHICH_MASK) >> ACQ_CORE_ACQ_CHAN_CTL_DTRIG_WHICH_SHIFT);
    print_value(f, "NUM_CHAN", num_chan);

#define MAX_NUM_CHAN 24
    if (num_chan > MAX_NUM_CHAN) {
        fprintf(f, "ERROR: max number of supported channels is %d, received %u\n", MAX_NUM_CHAN, num_chan);
        return;
    }

    /* channel description and atom description - 24 channels maximum (0-23).
     * the loop being used depends on the struct having no padding and elements in a set order */
    uint32_t p[MAX_NUM_CHAN];
    memcpy(p, &acq->ch0_desc, sizeof p);
    for (unsigned i = 0; i < num_chan; i++) {
        uint32_t desc = p[i*2], adesc = p[i*2 + 1];
        fprintf(f, "    channel %u:\n", i);
        print_value(f, "INT_WIDTH", (desc & ACQ_CORE_CH0_DESC_INT_WIDTH_MASK) >> ACQ_CORE_CH0_DESC_INT_WIDTH_SHIFT);
        print_value(f, "NUM_COALESCE", (desc & ACQ_CORE_CH0_DESC_NUM_COALESCE_MASK) >> ACQ_CORE_CH0_DESC_NUM_COALESCE_SHIFT);
        print_value(f, "NUM_ATOMS", (adesc & ACQ_CORE_CH0_ATOM_DESC_NUM_ATOMS_MASK) >> ACQ_CORE_CH0_ATOM_DESC_NUM_ATOMS_SHIFT);
        print_value(f, "ATOM_WIDTH", (adesc & ACQ_CORE_CH0_ATOM_DESC_ATOM_WIDTH_MASK) >> ACQ_CORE_CH0_ATOM_DESC_ATOM_WIDTH_SHIFT);
    }
}
