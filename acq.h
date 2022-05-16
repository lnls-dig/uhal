/*
 * Copyright (C) 2022 CNPEM (cnpem.br)
 * Author: Érico Nogueira <erico.rolim@lnls.br>
 *
 * Released according to the GNU GPL, version 3 or any later version.
 */

#ifndef ACQ_H
#define ACQ_H

#include <chrono>
#include <string>
#include <variant>
#include <vector>

#include "decoders.h"
#include "wb_acq_core_regs.h"

class LnlsBpmAcqCore: public RegisterDecoder {
    struct acq_core regs;

  public:
    LnlsBpmAcqCore();
    void print(FILE *, bool);
};

enum class data_sign {
    d_unsigned,
    d_signed,
};

enum class acq_status {
    success,
    in_progress,
    timeout,
};
typedef std::variant<acq_status, std::vector<uint32_t>, std::vector<int32_t>> acq_result;

class LnlsBpmAcqCoreController {
    const struct pcie_bars *bars;
    size_t addr;
    unsigned sample_size, alignment;
    /* current channel variables */
    unsigned channel_atom_width, channel_num_atoms;

    struct acq_core regs{};

    void get_internal_values();
    void encode_config();
    void write_config();
    void start_acquisition();
    bool acquisition_ready();

    acq_result m_result(data_sign sign, bool is_timed, std::chrono::milliseconds wait_time={});
    std::vector<uint32_t> result_unsigned();
    std::vector<int32_t> convert_to_signed(std::vector<uint32_t> unsigned_result);

    /* state variables for async */
    enum class acq_step {
        acq_stop,
        acq_started,
        acq_done,
    } m_step = acq_step::acq_stop;

  public:
    LnlsBpmAcqCoreController(const struct pcie_bars *bars, size_t addr):
        bars(bars), addr(addr)
    {
    }

    unsigned channel = 0;
    unsigned pre_samples = 4;
    unsigned post_samples = 0;
    unsigned number_shots = 1;
    std::string trigger_type = "immediate";
    unsigned data_trigger_threshold = 0;
    bool data_trigger_polarity_neg = true;
    unsigned data_trigger_sel = 0;
    unsigned data_trigger_filt = 1;
    unsigned data_trigger_channel = 0;
    unsigned trigger_delay = 0;

    [[nodiscard]]
    acq_result result(data_sign sign);
    [[nodiscard]]
    acq_result result(data_sign sign, std::chrono::milliseconds wait_time);
    [[nodiscard]]
    acq_result result_async(data_sign sign);
};

#endif
