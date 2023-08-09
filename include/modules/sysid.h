/*
 * Copyright (C) 2023 CNPEM (cnpem.br)
 * Author: Érico Nogueira <erico.rolim@lnls.br>
 *
 * Released according to the GNU GPL, version 3 or any later version.
 */

#ifndef SYS_ID_H
#define SYS_ID_H

#include <memory>
#include <vector>

#include "controllers.h"
#include "decoders.h"

namespace sys_id {

/* forward declaration */
struct wb_fofb_sys_id_regs;

class distortion_levels {
  public:
    distortion_levels(size_t);
    std::vector<int16_t> prbs_0, prbs_1;
};

class Core: public RegisterDecoder {
    std::unique_ptr<struct wb_fofb_sys_id_regs> regs_storage;
    struct wb_fofb_sys_id_regs &regs;

    void decode() override;
    void read_monitors() override;
    void decode_monitors() override;

  public:
    Core(struct pcie_bars &);
    ~Core();

    void print(FILE *, bool) const override;

    distortion_levels setpoint_distortion;
    distortion_levels posx_distortion, posy_distortion;
};

class Controller: public RegisterController {
  protected:
    std::unique_ptr<struct wb_fofb_sys_id_regs> regs_storage;
    struct wb_fofb_sys_id_regs &regs;

    void set_devinfo_callback() override;
    void encode_params();

  public:
    Controller(struct pcie_bars &);
    ~Controller();

    void write_params();

    /** First BPM ID whose position will be stored in SYSID acquisitions;
     * MAX_NUM_CTE BPMs will be stored */
    uint8_t base_bpm_id = 0;

    /** Enable PRBS reset via trigger */
    bool prbs_reset = false;
    /** Duration of each PRBS step. Must be at least 1 */
    uint16_t step_duration = 1;
    /** Set length of internal LFSR. Must be at least 2 */
    uint8_t lfsr_length = 2;
    /** Enable PRBS-based distortion on BPM positions */
    bool bpm_pos_distort_en = false;
    /** Enable PRBS-based distortion on accumulator setpoints */
    bool sp_distort_en = false;
    /** Set moving average samples for FOFBAcc distortion */
    uint8_t sp_mov_avg_samples;

    /** Distortion levels to apply on setpoint for each PRBS state */
    distortion_levels setpoint_distortion;
    /** Distortion levels to apply on X position for each PRBS state */
    distortion_levels posx_distortion;
    /** Distortion levels to apply on Y position for each PRBS state */
    distortion_levels posy_distortion;
};

} /* namespace sys_id */

#endif
