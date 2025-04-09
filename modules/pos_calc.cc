#include <array>
#include <cmath>
#include <stdexcept>

#include "pcie.h"
#include "printer.h"
#include "util.h"
#include "modules/pos_calc.h"

namespace pos_calc {

#include "hw/wb_pos_calc_regs.h"

struct GainArrays {
    std::array<uint32_t *, 4> gains_inverse, gains_direct;
    GainArrays(struct pos_calc &regs):
        gains_inverse{
            &regs.adc_ch0_swclk_0_gain,
            &regs.adc_ch1_swclk_0_gain,
            &regs.adc_ch2_swclk_0_gain,
            &regs.adc_ch3_swclk_0_gain,},
        gains_direct{
            &regs.adc_ch0_swclk_1_gain,
            &regs.adc_ch1_swclk_1_gain,
            &regs.adc_ch2_swclk_1_gain,
            &regs.adc_ch3_swclk_1_gain,}
    {
    }
};

struct OffsetArrays {
    std::array<uint32_t *, 4> offsets_inverse, offsets_direct;
    OffsetArrays(struct pos_calc &regs):
        offsets_inverse{
            &regs.adc_ch0_swclk_0_offset,
            &regs.adc_ch1_swclk_0_offset,
            &regs.adc_ch2_swclk_0_offset,
            &regs.adc_ch3_swclk_0_offset,},
        offsets_direct{
            &regs.adc_ch0_swclk_1_offset,
            &regs.adc_ch1_swclk_1_offset,
            &regs.adc_ch2_swclk_1_offset,
            &regs.adc_ch3_swclk_1_offset,}
    {
    }
};

struct SyncMaskRates {
    struct {
        uint32_t *tag, *data_mask_ctl, *data_mask_samples;
    } tbt, facq, monit;
    SyncMaskRates(struct pos_calc &regs):
        tbt{
            .tag = &regs.tbt_tag,
            .data_mask_ctl = &regs.tbt_data_mask_ctl,
            .data_mask_samples = &regs.tbt_data_mask_samples,},
        facq{
            .tag = &regs.monit1_tag,
            .data_mask_ctl = &regs.monit1_data_mask_ctl,
            .data_mask_samples = &regs.monit1_data_mask_samples,},
        monit{
            .tag = &regs.monit_tag,
            .data_mask_ctl = &regs.monit_data_mask_ctl,
            .data_mask_samples = &regs.monit_data_mask_samples,}
    {
    }
};

namespace {
    constexpr unsigned NUM_CHANNELS = 4;
    constexpr unsigned NUM_RATES = 3;
    constexpr unsigned ksum_fixed_point_pos = 24;
    constexpr unsigned ADC_OFFSET_VERSION = 1;

    constexpr unsigned POS_CALC_DEVID = 0x1bafbf1e;
    struct sdb_device_info ref_devinfo = {
        .vendor_id = LNLS_VENDORID,
        .device_id = POS_CALC_DEVID,
        .abi_ver_major = 1
    };
}

Core::Core(struct pcie_bars &bars):
    RegisterDecoder(bars, ref_devinfo, {
        PRINTER("KX", "BPM sensitivity in X axis", PrinterType::value),
        PRINTER("KY", "BPM sensitivity in Y axis", PrinterType::value),
        PRINTER("KSUM", "BPM sensitivity for Sum", PrinterType::value_float),
        PRINTER("TEST_DATA", "Test data counter for all channels", PrinterType::enable),
        PRINTER("AMPFIFO_MONIT_AMP", "Channel Amplitude", PrinterType::value),
        PRINTER("AMPFIFO_MONIT_COUNT", "Number of data records currently being stored in FIFO 'AMP FIFO Monitoring'", PrinterType::value),
        PRINTER("AMPFIFO_MONIT_FULL", "", PrinterType::boolean),
        PRINTER("AMPFIFO_MONIT_EMPTY", "", PrinterType::boolean),
        PRINTER("FOFB_SYNC_EN", "Switching Tag Synchronization Enable", PrinterType::enable),
        PRINTER("FOFB_DESYNC_CNT", "Switching Desynchronization Counter", PrinterType::value),
        PRINTER("FOFB_DATA_MASK_EN", "Switching Data Mask Enable", PrinterType::enable),
        PRINTER("FOFB_DATA_MASK_SAMPLES", "Switching Data Mask Samples", PrinterType::value),
        PRINTER("SYNC_EN", "Synchronizing Trigger Enable", PrinterType::enable),
        PRINTER("SYNC_DLY", "Synchronizing Trigger Delay", PrinterType::value),
        PRINTER("DESYNC_CNT", "Desynchronization Counter", PrinterType::value),
        PRINTER("DATA_MASK_EN", "Data Mask Enable", PrinterType::enable),
        PRINTER("DATA_MASK_SAMPLES_BEG", "Beginning Data Masking Samples", PrinterType::value),
        PRINTER("DATA_MASK_SAMPLES_END", "Ending Data Masking Samples", PrinterType::value),
        PRINTER("OFFSET_X", "BPM X position offset parameter register", PrinterType::value),
        PRINTER("OFFSET_Y", "BPM Y position offset parameter register", PrinterType::value),
        PRINTER("ADC_GAINS_FIXED_POINT_POS", "Fixed-point position constant value", PrinterType::value),
        PRINTER("ADC_SWCLK_INV_GAIN", "ADC channel gain on RFFE switch state 0 (inverted)", PrinterType::value_float),
        PRINTER("ADC_SWCLK_DIR_GAIN", "ADC channel gain on RFFE switch state 1 (direct)", PrinterType::value_float),
        PRINTER("ADC_SWCLK_INV_OFFSET", "ADC channel offset on RFFE switch state 0 (inverted)", PrinterType::value),
        PRINTER("ADC_SWCLK_DIR_OFFSET", "ADC channel offset on RFFE switch state 1 (direct)", PrinterType::value),
    }),
    CONSTRUCTOR_REGS(struct pos_calc)
{
    set_read_dest(regs);

    number_of_channels = NUM_CHANNELS;

    /* initialize these keys in the map */
    decode_fifo_csr();
    decode_fifo_amps();
}
Core::~Core() = default;

void Core::read()
{
    /* we can't read ampfifo registers r0-r3 here, since that has side effects
     * for their FIFO. The function has to be custom anyway, so we might as well
     * read as few values as possible */
    bar4_read_v(&bars, addr + POS_CALC_KX, &regs.kx, POS_CALC_KSUM - POS_CALC_KX + 4);
    regs.dds_cfg = bar4_read(&bars, addr + POS_CALC_DDS_CFG);
    bar4_read_v(&bars, addr + POS_CALC_SW_TAG, &regs.sw_tag, POS_CALC_SIZE - POS_CALC_SW_TAG);
}

void Core::decode()
{
    add_general("KX", rf_extract_value(regs.kx, POS_CALC_KX_VAL_MASK));
    add_general("KY", rf_extract_value(regs.ky, POS_CALC_KY_VAL_MASK));
    add_general(
        "KSUM",
        rf_fixed2float(rf_extract_value(regs.ksum, POS_CALC_KSUM_VAL_MASK), ksum_fixed_point_pos));
    add_general("TEST_DATA", rf_get_bit(regs.dds_cfg, POS_CALC_DDS_CFG_TEST_DATA));

    add_general("FOFB_SYNC_EN", rf_get_bit(regs.sw_tag, POS_CALC_SW_TAG_EN));
    add_general("FOFB_DESYNC_CNT", rf_extract_value(regs.sw_tag, POS_CALC_SW_TAG_DESYNC_CNT_MASK));
    add_general("FOFB_DESYNC_CNT_RST", rf_get_bit(regs.sw_tag, POS_CALC_SW_TAG_DESYNC_CNT_RST));

    add_general("FOFB_DATA_MASK_EN", rf_get_bit(regs.sw_data_mask, POS_CALC_SW_DATA_MASK_EN));
    add_general("FOFB_DATA_MASK_SAMPLES", rf_extract_value(regs.sw_data_mask, POS_CALC_SW_DATA_MASK_SAMPLES_MASK));

    const SyncMaskRates sync_mask_rates{regs};
    auto get_sync_and_mask = [this](const unsigned i, const auto rate) {
        add_channel("SYNC_EN", i, rf_get_bit(*rate.tag, POS_CALC_TBT_TAG_EN));
        add_channel("SYNC_DLY", i, rf_extract_value(*rate.tag, POS_CALC_TBT_TAG_DLY_MASK));
        add_channel("DESYNC_CNT", i, rf_extract_value(*rate.tag, POS_CALC_TBT_TAG_DESYNC_CNT_MASK));
        add_channel("DESYNC_CNT_RST", i, rf_get_bit(*rate.tag, POS_CALC_TBT_TAG_DESYNC_CNT_RST));

        add_channel("DATA_MASK_EN", i, rf_get_bit(*rate.data_mask_ctl, POS_CALC_TBT_DATA_MASK_CTL_EN));

        add_channel("DATA_MASK_SAMPLES_BEG", i, rf_extract_value(*rate.data_mask_samples, POS_CALC_TBT_DATA_MASK_SAMPLES_BEG_MASK));
        add_channel("DATA_MASK_SAMPLES_END", i, rf_extract_value(*rate.data_mask_samples, POS_CALC_TBT_DATA_MASK_SAMPLES_END_MASK));
    };
    get_sync_and_mask(0, sync_mask_rates.tbt);
    get_sync_and_mask(1, sync_mask_rates.facq);
    get_sync_and_mask(2, sync_mask_rates.monit);

    add_general("OFFSET_X", rf_whole_register(regs.offset_x, true));
    add_general("OFFSET_Y", rf_whole_register(regs.offset_y, true));

    auto rf_adc_gains_fixed_point_pos = rf_extract_value(regs.adc_gains_fixed_point_pos, POS_CALC_ADC_GAINS_FIXED_POINT_POS_DATA_MASK);
    add_general("ADC_GAINS_FIXED_POINT_POS", rf_adc_gains_fixed_point_pos);
    auto adc_gains_fixed_point_pos = std::get<int32_t>(rf_adc_gains_fixed_point_pos.value);

    const GainArrays gains_arrays{regs};
    auto get_gains = [this, adc_gains_fixed_point_pos](const char *name, const auto &gains) {
        for (unsigned i = 0; i < gains.size(); i++)
            add_channel(
                name, i,
                rf_fixed2float(rf_whole_register(*gains[i]), adc_gains_fixed_point_pos));
    };
    get_gains("ADC_SWCLK_INV_GAIN", gains_arrays.gains_inverse);
    get_gains("ADC_SWCLK_DIR_GAIN", gains_arrays.gains_direct);

    if (devinfo.abi_ver_minor >= ADC_OFFSET_VERSION) {
        const OffsetArrays offsets_arrays{regs};
        auto get_offsets = [this](const char *name, const auto &offsets) {
            for (unsigned i = 0; i < offsets.size(); i++)
                add_channel(name, i, rf_extract_value(*offsets[i], POS_CALC_ADC_CH0_SWCLK_0_OFFSET_DATA_MASK, true));
        };
        get_offsets("ADC_SWCLK_INV_OFFSET", offsets_arrays.offsets_inverse);
        get_offsets("ADC_SWCLK_DIR_OFFSET", offsets_arrays.offsets_direct);
    }
}

void Core::read_fifo_csr()
{
    regs.ampfifo_monit.ampfifo_monit_csr = bar4_read(&bars, addr + POS_CALC_AMPFIFO_MONIT_AMPFIFO_MONIT_CSR);
}

void Core::decode_fifo_csr()
{
    uint32_t t = regs.ampfifo_monit.ampfifo_monit_csr;
    add_general("AMPFIFO_MONIT_COUNT", extract_value(t, POS_CALC_AMPFIFO_MONIT_AMPFIFO_MONIT_CSR_COUNT_MASK));
    add_general("AMPFIFO_MONIT_FULL", get_bit(t, POS_CALC_AMPFIFO_MONIT_AMPFIFO_MONIT_CSR_FULL));
    add_general("AMPFIFO_MONIT_EMPTY", get_bit(t, POS_CALC_AMPFIFO_MONIT_AMPFIFO_MONIT_CSR_EMPTY));
}

void Core::read_fifo_amps()
{
    bar4_read_v(&bars, addr + POS_CALC_AMPFIFO_MONIT_AMPFIFO_MONIT_R0,
        &regs.ampfifo_monit.ampfifo_monit_r0,
        POS_CALC_AMPFIFO_MONIT_AMPFIFO_MONIT_CSR - POS_CALC_AMPFIFO_MONIT_AMPFIFO_MONIT_R0);
}

void Core::decode_fifo_amps()
{
    add_channel("AMPFIFO_MONIT_AMP", 0, regs.ampfifo_monit.ampfifo_monit_r0);
    add_channel("AMPFIFO_MONIT_AMP", 1, regs.ampfifo_monit.ampfifo_monit_r1);
    add_channel("AMPFIFO_MONIT_AMP", 2, regs.ampfifo_monit.ampfifo_monit_r2);
    add_channel("AMPFIFO_MONIT_AMP", 3, regs.ampfifo_monit.ampfifo_monit_r3);
}

bool Core::fifo_empty()
{
    read_fifo_csr();
    decode_fifo_csr();
    return get_general_data<int32_t>("AMPFIFO_MONIT_EMPTY");
}

void Core::get_fifo_amps()
{
    read_fifo_amps();
    decode_fifo_amps();
}

Controller::Controller(struct pcie_bars &bars):
    RegisterDecoderController(bars, ref_devinfo, &dec),
    CONSTRUCTOR_REGS(struct pos_calc),
    dec(bars)
{
    set_read_dest(regs);
}
Controller::~Controller() = default;

void Controller::write_params()
{
    encode_params();

    /* has to write to the same registers Core::read() above reads, though we
     * can skip regs.ampfifo_monit.ampfifo_monit_csr, since it's read-only. The
     * reason is the same: we can't read the FIFO registers */
    bar4_write_v(&bars, addr + POS_CALC_KX, &regs.kx, POS_CALC_KSUM - POS_CALC_KX + 4);
    bar4_write(&bars, addr + POS_CALC_DDS_CFG, regs.dds_cfg);
    bar4_write_v(&bars, addr + POS_CALC_SW_TAG, &regs.sw_tag, POS_CALC_SIZE - POS_CALC_SW_TAG);

    write_general("FOFB_DESYNC_CNT_RST", 0);
    for (unsigned i = 0; i < NUM_RATES; i++)
        write_channel("DESYNC_CNT_RST", i, 0);
}

} /* namespace pos_calc */
