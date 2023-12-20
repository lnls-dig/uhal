#ifndef POS_CALC_H
#define POS_CALC_H

#include <chrono>
#include <memory>
#include <vector>

#include "controllers.h"
#include "decoders.h"

namespace pos_calc {

using namespace std::chrono_literals;

/* forward declaration */
struct pos_calc;

class Core: public RegisterDecoder {
    std::unique_ptr<struct pos_calc> regs_storage;
    struct pos_calc &regs;

    void decode() override;
    void read() override;
    void read_monitors() override;
    void decode_monitors() override;

    void read_fifo_csr();
    void decode_fifo_csr();
    void read_fifo_amps();
    void decode_fifo_amps();

  public:
    Core(struct pcie_bars &);
    ~Core() override;

    bool fifo_empty();
    void get_fifo_amps();
};

class Controller: public RegisterDecoderController {
    std::unique_ptr<struct pos_calc> regs_storage;
    struct pos_calc &regs;

    Core dec;

  public:
    Controller(struct pcie_bars &);
    ~Controller();

    void write_params() override;
};

} /* namespace pos_calc */

#endif
