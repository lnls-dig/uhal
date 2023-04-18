/*
 * Copyright (C) 2022 CNPEM (cnpem.br)
 * Author: Érico Nogueira <erico.rolim@lnls.br>
 *
 * Released according to the GNU GPL, version 3 or any later version.
 */

#include <algorithm>
#include <stdexcept>
#include <type_traits>

#include "pcie.h"
#include "printer.h"
#include "util.h"
#include "modules/fofb_processing.h"

namespace fofb_processing {

#include "hw/wb_fofb_processing_regs.h"

static constexpr unsigned MAX_NUM_CHAN = 12, MAX_BPMS = 256;

Core::Core(struct pcie_bars &bars):
    RegisterDecoder(bars, {
        PRINTER("FIXED_POINT_POS_COEFF", "Position of point in fixed point representation of coefficientes", PrinterType::value),
        PRINTER("FIXED_POINT_POS_GAINS", "Position of point in fixed point representation of gains", PrinterType::value),
        PRINTER("INTLK_CTL_SRC_EN_ORB_DISTORT", "Enable orbit distortion interlock source", PrinterType::enable),
        PRINTER("INTLK_CTL_SRC_EN_PACKET_LOSS", "Enable packet loss interlock source", PrinterType::enable),
        PRINTER("INTLK_STA", "Loop interlock flags register", PrinterType::value_hex),
        PRINTER("INTLK_STA_ORB_DISTORT", "Orbit distortion loop interlock flag", PrinterType::boolean),
        PRINTER("INTLK_STA_PACKET_LOSS", "Packet loss loop interlock flag", PrinterType::boolean),
        PRINTER("INTLK_ORB_DISTORT_LIMIT", "Orbit distortion limit", PrinterType::value),
        PRINTER("INTLK_MIN_NUM_PACKETS", "Minimum number of packets per timeframe", PrinterType::value),
        PRINTER("SP_DECIM_RATIO_MAX", "Maximum setpoint decimation ratio", PrinterType::value),

        PRINTER("CH_ACC_CTL_FREEZE", "Freeze accumulator", PrinterType::enable),
        PRINTER("CH_ACC_GAIN", "Accumulator gain", PrinterType::value_float),
        PRINTER("CH_ACC_LIMITS_MAX", "Maximum saturation value", PrinterType::value),
        PRINTER("CH_ACC_LIMITS_MIN", "Minimum saturation value", PrinterType::value),
        PRINTER("CH_SP_DECIM_DATA", "Decimated setpoint", PrinterType::value),
        PRINTER("CH_SP_DECIM_RATIO", "Setpoint decimation ratio", PrinterType::value),
    }),
    regs_storage(new struct wb_fofb_processing_regs),
    regs(*regs_storage)
{
    read_size = sizeof regs;
    read_dest = &regs;

    device_match = device_match_fofb_processing;

    number_of_channels = MAX_NUM_CHAN;
}
Core::~Core() = default;

void Core::decode()
{
    uint32_t fixed_point_gain = extract_value<uint32_t>(regs.fixed_point_pos.accs_gains, WB_FOFB_PROCESSING_REGS_FIXED_POINT_POS_ACCS_GAINS_VAL_MASK);
    uint32_t fixed_point_coeff = extract_value<uint32_t>(regs.fixed_point_pos.coeff, WB_FOFB_PROCESSING_REGS_FIXED_POINT_POS_COEFF_VAL_MASK);
    add_general("FIXED_POINT_POS_GAINS", fixed_point_gain);
    add_general("FIXED_POINT_POS_COEFF", fixed_point_coeff);

    uint32_t t;
    t = regs.loop_intlk.ctl;
    add_general("INTLK_CTL_SRC_EN_ORB_DISTORT", get_bit(t, WB_FOFB_PROCESSING_REGS_LOOP_INTLK_CTL_SRC_EN_ORB_DISTORT));
    add_general("INTLK_CTL_SRC_EN_PACKET_LOSS", get_bit(t, WB_FOFB_PROCESSING_REGS_LOOP_INTLK_CTL_SRC_EN_PACKET_LOSS));
    t = regs.loop_intlk.sta;
    add_general("INTLK_STA", t);
    add_general("INTLK_STA_ORB_DISTORT", get_bit(t, WB_FOFB_PROCESSING_REGS_LOOP_INTLK_STA_ORB_DISTORT));
    add_general("INTLK_STA_PACKET_LOSS", get_bit(t, WB_FOFB_PROCESSING_REGS_LOOP_INTLK_STA_PACKET_LOSS));

    add_general("INTLK_ORB_DISTORT_LIMIT", regs.loop_intlk.orb_distort_limit);
    add_general("INTLK_MIN_NUM_PACKETS", regs.loop_intlk.min_num_pkts);

    add_general("SP_DECIM_RATIO_MAX", regs.sp_decim_ratio_max);

    ref_orb_x.resize(MAX_BPMS);
    ref_orb_y.resize(MAX_BPMS);
    coefficients_x.resize(MAX_NUM_CHAN);
    coefficients_y.resize(MAX_NUM_CHAN);

    size_t i;
    auto add_channel = [this, &i](const char *name, auto value, bool skip=false) {
        channel_data[name].resize(*number_of_channels);
        if constexpr (std::is_same_v<std::remove_reference_t<decltype(value)>, double>) {
            add_channel_impl_double(name, i, value, skip);
        } else {
            add_channel_impl(name, i, value, skip);
        }
    };
    for (i = 0; i < *number_of_channels; i++) {
        auto &ram_bank = regs.ch[i].coeff_ram_bank;
        const size_t elements = MAX_BPMS;
        coefficients_x[i].resize(elements);
        coefficients_y[i].resize(elements);

        size_t u = 0;
        std::generate(coefficients_x[i].begin(), coefficients_x[i].end(),
            [&](){ return fixed2float(ram_bank[u++].data, fixed_point_coeff); });
        u = MAX_BPMS;
        std::generate(coefficients_y[i].begin(), coefficients_y[i].end(),
            [&](){ return fixed2float(ram_bank[u++].data, fixed_point_coeff); });

        add_channel("CH_ACC_CTL_FREEZE", get_bit(regs.ch[i].acc.ctl, WB_FOFB_PROCESSING_REGS_CH_ACC_CTL_FREEZE));
        add_channel("CH_ACC_GAIN", fixed2float(regs.ch[i].acc.gain, fixed_point_gain));
        add_channel("CH_ACC_LIMITS_MAX", extract_value<int32_t>(regs.ch[i].sp_limits.max, WB_FOFB_PROCESSING_REGS_CH_SP_LIMITS_MAX_VAL_MASK));
        add_channel("CH_ACC_LIMITS_MIN", extract_value<int32_t>(regs.ch[i].sp_limits.min, WB_FOFB_PROCESSING_REGS_CH_SP_LIMITS_MIN_VAL_MASK));
        add_channel("CH_SP_DECIM_DATA", extract_value<int32_t>(regs.ch[i].sp_decim.data, WB_FOFB_PROCESSING_REGS_CH_SP_DECIM_DATA_VAL_MASK));
        add_channel("CH_SP_DECIM_RATIO", extract_value<int32_t>(regs.ch[i].sp_decim.ratio, WB_FOFB_PROCESSING_REGS_CH_SP_DECIM_RATIO_VAL_MASK) + 1);

        data_order_done = true;
    }

    size_t u = 0;
    std::generate(ref_orb_x.begin(), ref_orb_x.end(), [&](){ return (int32_t)regs.sps_ram_bank[u++].data; });
    u = MAX_BPMS; /* access the second half of the RAM bank */
    std::generate(ref_orb_y.begin(), ref_orb_y.end(), [&](){ return (int32_t)regs.sps_ram_bank[u++].data; });
}

void Core::print(FILE *f, bool verbose)
{
    const bool v = verbose;

    RegisterDecoder::print(f, v);

    if (verbose) {
        fputs("reference orbit x:\n", f);
        for (auto v: ref_orb_x) fprintf(f, "%d ", (int)v);

        fputs("\nreference orbit y:\n", f);
        for (auto v: ref_orb_y) fprintf(f, "%d ", (int)v);
        fputc('\n', f);
    }

    if (verbose && channel) {
        fputs("coefficients for x:\n", f);
        for (auto v: coefficients_x[*channel]) fprintf(f, "%lf ", v);

        fputs("\ncoefficients for y:\n", f);
        for (auto v: coefficients_y[*channel]) fprintf(f, "%lf ", v);
        fputc('\n', f);
    }
}

Controller::Controller(struct pcie_bars &bars):
    RegisterController(bars),
    regs_storage(new struct wb_fofb_processing_regs),
    regs(*regs_storage),
    ref_orb_x(MAX_BPMS), ref_orb_y(MAX_BPMS),
    parameters(MAX_NUM_CHAN)
{
    for (auto &p: parameters) {
        p.coefficients_x.resize(MAX_BPMS);
        p.coefficients_y.resize(MAX_BPMS);
    }
}
Controller::~Controller() = default;

void Controller::set_devinfo_callback()
{
    bar4_read_v(&bars, addr, &regs, sizeof regs);
    fixed_point_coeff = extract_value<uint32_t>(regs.fixed_point_pos.coeff, WB_FOFB_PROCESSING_REGS_FIXED_POINT_POS_COEFF_VAL_MASK);
    fixed_point_gains = extract_value<uint32_t>(regs.fixed_point_pos.accs_gains, WB_FOFB_PROCESSING_REGS_FIXED_POINT_POS_ACCS_GAINS_VAL_MASK);
}

void Controller::encode_config()
{
    insert_bit(regs.loop_intlk.ctl, intlk_sta_clr, WB_FOFB_PROCESSING_REGS_LOOP_INTLK_CTL_STA_CLR);
    insert_bit(regs.loop_intlk.ctl, intlk_en_orb_distort, WB_FOFB_PROCESSING_REGS_LOOP_INTLK_CTL_SRC_EN_ORB_DISTORT);
    insert_bit(regs.loop_intlk.ctl, intlk_en_packet_loss, WB_FOFB_PROCESSING_REGS_LOOP_INTLK_CTL_SRC_EN_PACKET_LOSS);

    regs.loop_intlk.orb_distort_limit = orb_distort_limit;
    regs.loop_intlk.min_num_pkts = min_num_packets;

    for (unsigned j = 0; j < MAX_BPMS; j++) {
        regs.sps_ram_bank[j].data = ref_orb_x[j];
        regs.sps_ram_bank[j + MAX_BPMS].data = ref_orb_y[j];
    }

    for (unsigned i = 0; i < MAX_NUM_CHAN; i++) {
        if (parameters[i].sp_decim_ratio < 1)
            throw std::runtime_error("decimation ration can't be 0");

        for (unsigned j = 0; j < MAX_BPMS; j++) {
            regs.ch[i].coeff_ram_bank[j].data = float2fixed(parameters[i].coefficients_x[j], fixed_point_coeff);
            regs.ch[i].coeff_ram_bank[j + MAX_BPMS].data = float2fixed(parameters[i].coefficients_y[j], fixed_point_coeff);
        }

        insert_bit(regs.ch[i].acc.ctl, parameters[i].acc_clear, WB_FOFB_PROCESSING_REGS_CH_ACC_CTL_CLEAR);
        insert_bit(regs.ch[i].acc.ctl, parameters[i].acc_freeze, WB_FOFB_PROCESSING_REGS_CH_ACC_CTL_FREEZE);
        regs.ch[i].acc.gain = float2fixed(parameters[i].acc_gain, fixed_point_gains);

        clear_and_insert(regs.ch[i].sp_limits.max, parameters[i].sp_limit_max, WB_FOFB_PROCESSING_REGS_CH_SP_LIMITS_MAX_VAL_MASK);
        clear_and_insert(regs.ch[i].sp_limits.min, parameters[i].sp_limit_min, WB_FOFB_PROCESSING_REGS_CH_SP_LIMITS_MIN_VAL_MASK);
        clear_and_insert(regs.ch[i].sp_decim.ratio, parameters[i].sp_decim_ratio - 1, WB_FOFB_PROCESSING_REGS_CH_SP_DECIM_RATIO_VAL_MASK);
    }
}

void Controller::write_params()
{
    encode_config();

    bar4_write_v(&bars, addr, &regs, sizeof regs);

    /* reset clear flags */
    intlk_sta_clr = false;
    for (auto &p: parameters) p.acc_clear = false;
}

} /* namespace fofb_processing */
