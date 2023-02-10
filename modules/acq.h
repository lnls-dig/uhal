/*
 * Copyright (C) 2022 CNPEM (cnpem.br)
 * Author: Érico Nogueira <erico.rolim@lnls.br>
 *
 * Released according to the GNU GPL, version 3 or any later version.
 */

#ifndef ACQ_H
#define ACQ_H

#include <chrono>
#include <memory>
#include <string>
#include <variant>
#include <vector>

#include "controllers.h"
#include "decoders.h"

namespace acq {

constexpr unsigned ACQ_DEVID = 0x4519a0ad;

inline const device_match_fn device_match_acq =
    device_match_impl<LNLS_VENDORID, ACQ_DEVID, 2>;

/* forward declaration */
struct acq_core;

class Core: public RegisterDecoder {
    std::unique_ptr<struct acq_core> regs_storage;
    struct acq_core &regs;

    void decode();

  public:
    Core(struct pcie_bars &);
    ~Core();
};

enum class acq_status {
    success,
    in_progress,
    timeout,
};
template<class Data>
using acq_result = std::variant<acq_status, std::vector<Data>>;

class Controller: public RegisterController {
    unsigned sample_size, alignment;
    /* current channel variables */
    unsigned channel_atom_width, channel_num_atoms;

    std::unique_ptr<struct acq_core> regs_storage;
    struct acq_core &regs;

    void get_internal_values();
    void encode_config();
    void write_config();
    void start_acquisition();
    bool acquisition_ready();

    template <class Data>
    std::vector<Data> get_result();

    /* state variables for async */
    enum class acq_step {
        acq_stop,
        acq_started,
        acq_done,
    } m_step = acq_step::acq_stop;

  public:
    Controller(struct pcie_bars &);
    ~Controller();

    static inline const device_match_fn device_match = device_match_acq;

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

    template <class Data> [[nodiscard]]
    std::vector<Data> result(std::optional<std::chrono::milliseconds> wait_time=std::nullopt);
    template <class Data> [[nodiscard]]
    acq_result<Data> result_async();

    template<typename T>
    void print_csv(FILE *f, std::vector<T> &res);
};

} /* namespace acq */

#endif
