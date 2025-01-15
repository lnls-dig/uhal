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

/* forward declaration */
struct acq_core;

/** This class isn't relevant for most users, see Controller. */
class Core: public RegisterDecoder {
    std::unique_ptr<struct acq_core> regs_storage;
    struct acq_core &regs;

    void decode() override;

  public:
    Core(struct pcie_bars &);
    ~Core() override;
};

enum class acq_error {
    success,
    error,
    bad_post_samples,
    too_many_samples,
    no_samples,
};

enum class acq_status {
    idle,
    success,
    in_progress,
    timeout,
};

/** For most users, the Core class isn't relevant, since it simply provides the
 * current state of this core's registers, which doesn't reflect any hardware
 * state beyond the acquisition state machine. This class is the relevant one,
 * because it provides an interface to control an acquisition, and obtain its
 * data.
 *
 * It can also be used to control an acquisition asynchronously and safely,
 * while still registering the configuration for the next acquisition. */
class Controller: public RegisterController {
    /* read from internal MemoryAllocator */
    size_t ram_start_addr, ram_end_addr;

    /* information from the current acquisition:
     * - current channel information
     * - current channel information that had to be calculated
     * - amount of samples */
    unsigned channel_atom_width, channel_num_atoms,
             sample_size, alignment,
             acq_pre_samples, acq_post_samples;

    std::unique_ptr<struct acq_core> regs_storage;
    struct acq_core &regs;

    void get_internal_values();
    void encode_params() override;
    bool acquisition_ready();

    void set_devinfo_callback() override;

    /* state variables for async */
    enum class acq_step {
        stop,
        started,
        done,
    } m_step = acq_step::stop;

  public:
    Controller(struct pcie_bars &);
    ~Controller();

    unsigned channel = 0;
    unsigned pre_samples = 4;
    unsigned post_samples = 0;
    unsigned number_shots = 1;
    std::string trigger_type = "now";
    unsigned data_trigger_threshold = 0;
    bool data_trigger_polarity_neg = true;
    unsigned data_trigger_sel = 0;
    unsigned data_trigger_filt = 1;
    unsigned data_trigger_channel = 0;
    unsigned trigger_delay = 0;

    acq_error start_acquisition();
    void stop_acquisition();

    template <class Data>
    std::vector<Data> get_result();

    template <class Data> [[nodiscard]]
    std::vector<Data> result(std::optional<std::chrono::milliseconds> wait_time=std::nullopt);
    /** Get acquisition status. For asynchronous use */
    [[nodiscard]]
    acq_status get_acq_status();

    template<typename T>
    void print_csv(FILE *f, std::vector<T> &res);
};

} /* namespace acq */

#endif
