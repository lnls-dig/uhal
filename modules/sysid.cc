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
#include "modules/sysid.h"

namespace sys_id {

#include "hw/wb_fofb_sys_id_regs.h"

static_assert(WB_FOFB_SYS_ID_REGS_PRBS_BPM_POS_DISTORT == offsetof(wb_fofb_sys_id_regs, prbs.bpm_pos_distort.distort_ram));

static_assert(WB_FOFB_SYS_ID_REGS_PRBS_SP_DISTORT_CH_LEVELS_LEVEL_0_MASK ==
    WB_FOFB_SYS_ID_REGS_PRBS_BPM_POS_DISTORT_DISTORT_RAM_LEVELS_LEVEL_0_MASK);
static_assert(WB_FOFB_SYS_ID_REGS_PRBS_SP_DISTORT_CH_LEVELS_LEVEL_1_MASK ==
    WB_FOFB_SYS_ID_REGS_PRBS_BPM_POS_DISTORT_DISTORT_RAM_LEVELS_LEVEL_1_MASK);
#define LEVEL_0_MASK WB_FOFB_SYS_ID_REGS_PRBS_SP_DISTORT_CH_LEVELS_LEVEL_0_MASK
#define LEVEL_1_MASK WB_FOFB_SYS_ID_REGS_PRBS_SP_DISTORT_CH_LEVELS_LEVEL_1_MASK

namespace {
    const size_t NUM_SETPOINTS = 12, NUM_POSITIONS = 256;

    struct sdb_device_info ref_devinfo = {
        .vendor_id = LNLS_VENDORID,
        .device_id = SYS_ID_DEVID,
        .abi_ver_major = 1
    };
}

distortion_levels::distortion_levels(size_t size):
  prbs_0(size), prbs_1(size)
{
}

Core::Core(struct pcie_bars &bars):
    RegisterDecoder(bars, ref_devinfo, {
        PRINTER("MAX_NUM_CTE", "Maximum number of BPMs that can be flattenized", PrinterType::value),
        PRINTER("BASE_BPM_ID", "First BPM ID to be flattenized", PrinterType::value),
        PRINTER("PRBS_CTL_RST", "Enable triggered reset for PRBS", PrinterType::enable),
        PRINTER("PRBS_CTL_STEP_DURATION", "Duration of each PRBS step", PrinterType::value),
        PRINTER("PRBS_CTL_LFSR_LENGTH", "Length of internal LFSR", PrinterType::value),
        PRINTER("PRBS_CTL_BPM_POS_DISTORT_EN", "Enable PRBS-based distortion on BPM positions", PrinterType::enable),
        PRINTER("PRBS_CTL_SP_DISTORT_EN", "Enable PRBS-based distortion on accumulator setpoints", PrinterType::enable),
        PRINTER("SP_DISTORT_MOV_AVG_NUM_TAPS_SEL", "Setpoint distortion moving average taps selector", PrinterType::value),
        PRINTER("SP_DISTORT_MOV_AVG_MAX_NUM_TAPS_SEL_CTE", "Max value for SP_DISTORT_MOV_AVG_NUM_TAPS_SEL field", PrinterType::value),
    }),
    regs_storage(new struct wb_fofb_sys_id_regs),
    regs(*regs_storage),
    setpoint_distortion(NUM_SETPOINTS),
    posx_distortion(NUM_POSITIONS),
    posy_distortion(NUM_POSITIONS)
{
    read_size = sizeof regs;
    read_dest = &regs;

    device_match = device_match_sys_id;
}
Core::~Core() = default;

/* no monitored values */
void Core::read_monitors()
{
}
void Core::decode_monitors()
{
}

void Core::decode()
{
    uint32_t t;
    add_general("MAX_NUM_CTE", regs.bpm_pos_flatenizer.max_num_cte);

    t = regs.bpm_pos_flatenizer.ctl;
    add_general("BASE_BPM_ID", extract_value<uint8_t>(t, WB_FOFB_SYS_ID_REGS_BPM_POS_FLATENIZER_CTL_BASE_BPM_ID_MASK));

    t = regs.prbs.ctl;
    add_general("PRBS_CTL_RST", get_bit(t, WB_FOFB_SYS_ID_REGS_PRBS_CTL_RST));
    add_general("PRBS_CTL_STEP_DURATION", extract_value<uint16_t>(t, WB_FOFB_SYS_ID_REGS_PRBS_CTL_STEP_DURATION_MASK) + 1);
    add_general("PRBS_CTL_LFSR_LENGTH", extract_value<uint8_t>(t, WB_FOFB_SYS_ID_REGS_PRBS_CTL_LFSR_LENGTH_MASK) + 2);
    add_general("PRBS_CTL_BPM_POS_DISTORT_EN", get_bit(t, WB_FOFB_SYS_ID_REGS_PRBS_CTL_BPM_POS_DISTORT_EN));
    add_general("PRBS_CTL_SP_DISTORT_EN", get_bit(t, WB_FOFB_SYS_ID_REGS_PRBS_CTL_SP_DISTORT_EN));
    add_general("SP_DISTORT_MOV_AVG_NUM_TAPS_SEL", extract_value<uint8_t>(t, WB_FOFB_SYS_ID_REGS_PRBS_CTL_SP_DISTORT_MOV_AVG_NUM_TAPS_SEL_MASK));

    add_general("SP_DISTORT_MOV_AVG_MAX_NUM_TAPS_SEL_CTE", regs.prbs.sp_distort_mov_avg_max_num_taps_sel_cte);

    data_order_done = true;

    auto get_distortion = [](const auto levels, const auto index, auto &distort) {
        distort.prbs_0[index] = extract_value<int16_t>(levels, LEVEL_0_MASK);
        distort.prbs_1[index] = extract_value<int16_t>(levels, LEVEL_1_MASK);
    };
    for (size_t i = 0; i < NUM_SETPOINTS; i++) {
        get_distortion(regs.prbs.sp_distort.ch[i].levels, i, setpoint_distortion);
    }
    for (size_t i = 0; i < NUM_POSITIONS; i++) {
        get_distortion(regs.prbs.bpm_pos_distort.distort_ram[i].levels, i, posx_distortion);
        get_distortion(regs.prbs.bpm_pos_distort.distort_ram[i+NUM_POSITIONS].levels, i, posy_distortion);
    }
}

void Core::print(FILE *f, bool verbose) const
{
    RegisterDecoder::print(f, verbose);

    if (verbose) {
        auto print_prbs = [f](const char *name, auto &prbs) {
            fprintf(f, "%s: ", name);
            for (int v: prbs)
                fprintf(f, " %d,", v);
            fprintf(f, "\n");
        };
        print_prbs("setpoint prbs0", setpoint_distortion.prbs_0);
        print_prbs("setpoint prbs1", setpoint_distortion.prbs_1);
    }
}

Controller::Controller(struct pcie_bars &bars):
    RegisterController(bars),
    regs_storage(new struct wb_fofb_sys_id_regs),
    regs(*regs_storage),
    setpoint_distortion(NUM_SETPOINTS),
    posx_distortion(NUM_POSITIONS),
    posy_distortion(NUM_POSITIONS)
{
}
Controller::~Controller() = default;

void Controller::set_devinfo_callback()
{
    bar4_read_v(&bars, addr, &regs, sizeof regs);
}

void Controller::encode_config()
{
    if (step_duration < 1 || step_duration > 0x3FF + 1)
        throw std::runtime_error("step duration is outside of valid range");
    if (lfsr_length < 2 || lfsr_length > 0x1E + 2)
        throw std::runtime_error("lfsr length is outside of valid range");
    if (sp_mov_avg_samples > regs.prbs.sp_distort_mov_avg_max_num_taps_sel_cte)
        throw std::runtime_error("setpoint distortion moving average taps are outside of valid range");

    clear_and_insert(regs.bpm_pos_flatenizer.ctl, base_bpm_id, WB_FOFB_SYS_ID_REGS_BPM_POS_FLATENIZER_CTL_BASE_BPM_ID_MASK);

    insert_bit(regs.prbs.ctl, prbs_reset, WB_FOFB_SYS_ID_REGS_PRBS_CTL_RST);
    clear_and_insert(regs.prbs.ctl, step_duration - 1, WB_FOFB_SYS_ID_REGS_PRBS_CTL_STEP_DURATION_MASK);
    clear_and_insert(regs.prbs.ctl, lfsr_length - 2, WB_FOFB_SYS_ID_REGS_PRBS_CTL_LFSR_LENGTH_MASK);
    insert_bit(regs.prbs.ctl, bpm_pos_distort_en, WB_FOFB_SYS_ID_REGS_PRBS_CTL_BPM_POS_DISTORT_EN);
    insert_bit(regs.prbs.ctl, sp_distort_en, WB_FOFB_SYS_ID_REGS_PRBS_CTL_SP_DISTORT_EN);
    clear_and_insert(regs.prbs.ctl, sp_mov_avg_samples, WB_FOFB_SYS_ID_REGS_PRBS_CTL_SP_DISTORT_MOV_AVG_NUM_TAPS_SEL_MASK);

    auto set_distortion = [](auto &levels, auto index, const auto &distort) {
        /* the uint16_t casts are necessary to avoid sign extension issues */
        clear_and_insert(levels, (uint16_t)distort.prbs_0[index], LEVEL_0_MASK);
        clear_and_insert(levels, (uint16_t)distort.prbs_1[index], LEVEL_1_MASK);
    };
    for (size_t i = 0; i < NUM_SETPOINTS; i++) {
        set_distortion(regs.prbs.sp_distort.ch[i].levels, i, setpoint_distortion);
    }
    for (size_t i = 0; i < NUM_POSITIONS; i++) {
        set_distortion(regs.prbs.bpm_pos_distort.distort_ram[i].levels, i, posx_distortion);
        set_distortion(regs.prbs.bpm_pos_distort.distort_ram[i+NUM_POSITIONS].levels, i, posy_distortion);
    }
}

void Controller::write_params()
{
    encode_config();
    bar4_write_v(&bars, addr, &regs, sizeof regs);
}

} /* namespace sys_id */
