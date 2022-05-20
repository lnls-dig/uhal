#ifndef __CHEBY__ACQ_CORE__H__
#define __CHEBY__ACQ_CORE__H__
#define ACQ_CORE_SIZE 252 /* 0xfc */

/* Control register */
#define ACQ_CORE_CTL 0x0UL
#define ACQ_CORE_CTL_FSM_START_ACQ 0x1UL
#define ACQ_CORE_CTL_FSM_STOP_ACQ 0x2UL
#define ACQ_CORE_CTL_RESERVED1_MASK 0xfffcUL
#define ACQ_CORE_CTL_RESERVED1_SHIFT 2
#define ACQ_CORE_CTL_FSM_ACQ_NOW 0x10000UL
#define ACQ_CORE_CTL_RESERVED2_MASK 0xfffe0000UL
#define ACQ_CORE_CTL_RESERVED2_SHIFT 17

/* Status register */
#define ACQ_CORE_STA 0x4UL
#define ACQ_CORE_STA_FSM_STATE_MASK 0x7UL
#define ACQ_CORE_STA_FSM_STATE_SHIFT 0
#define ACQ_CORE_STA_FSM_ACQ_DONE 0x8UL
#define ACQ_CORE_STA_RESERVED1_MASK 0xf0UL
#define ACQ_CORE_STA_RESERVED1_SHIFT 4
#define ACQ_CORE_STA_FC_TRANS_DONE 0x100UL
#define ACQ_CORE_STA_FC_FULL 0x200UL
#define ACQ_CORE_STA_RESERVED2_MASK 0xfc00UL
#define ACQ_CORE_STA_RESERVED2_SHIFT 10
#define ACQ_CORE_STA_DDR3_TRANS_DONE 0x10000UL
#define ACQ_CORE_STA_RESERVED3_MASK 0xfffe0000UL
#define ACQ_CORE_STA_RESERVED3_SHIFT 17

/* Trigger configuration */
#define ACQ_CORE_TRIG_CFG 0x8UL
#define ACQ_CORE_TRIG_CFG_HW_TRIG_SEL 0x1UL
#define ACQ_CORE_TRIG_CFG_HW_TRIG_POL 0x2UL
#define ACQ_CORE_TRIG_CFG_HW_TRIG_EN 0x4UL
#define ACQ_CORE_TRIG_CFG_SW_TRIG_EN 0x8UL
#define ACQ_CORE_TRIG_CFG_INT_TRIG_SEL_MASK 0x1f0UL
#define ACQ_CORE_TRIG_CFG_INT_TRIG_SEL_SHIFT 4
#define ACQ_CORE_TRIG_CFG_RESERVED_MASK 0xfffffe00UL
#define ACQ_CORE_TRIG_CFG_RESERVED_SHIFT 9

/* Trigger data config threshold */
#define ACQ_CORE_TRIG_DATA_CFG 0xcUL
#define ACQ_CORE_TRIG_DATA_CFG_THRES_FILT_MASK 0xffUL
#define ACQ_CORE_TRIG_DATA_CFG_THRES_FILT_SHIFT 0
#define ACQ_CORE_TRIG_DATA_CFG_RESERVED_MASK 0xffffff00UL
#define ACQ_CORE_TRIG_DATA_CFG_RESERVED_SHIFT 8

/* Trigger data threshold */
#define ACQ_CORE_TRIG_DATA_THRES 0x10UL

/* Trigger delay */
#define ACQ_CORE_TRIG_DLY 0x14UL

/* Software trigger */
#define ACQ_CORE_SW_TRIG 0x18UL

/* Number of shots */
#define ACQ_CORE_SHOTS 0x1cUL
#define ACQ_CORE_SHOTS_NB_MASK 0xffffUL
#define ACQ_CORE_SHOTS_NB_SHIFT 0
#define ACQ_CORE_SHOTS_MULTISHOT_RAM_SIZE_IMPL 0x10000UL
#define ACQ_CORE_SHOTS_MULTISHOT_RAM_SIZE_MASK 0xfffe0000UL
#define ACQ_CORE_SHOTS_MULTISHOT_RAM_SIZE_SHIFT 17

/* Trigger address register */
#define ACQ_CORE_TRIG_POS 0x20UL

/* Pre-trigger samples */
#define ACQ_CORE_PRE_SAMPLES 0x24UL

/* Post-trigger samples */
#define ACQ_CORE_POST_SAMPLES 0x28UL

/* Samples counter */
#define ACQ_CORE_SAMPLES_CNT 0x2cUL

/* DDR3 Start Address */
#define ACQ_CORE_DDR3_START_ADDR 0x30UL

/* DDR3 End Address */
#define ACQ_CORE_DDR3_END_ADDR 0x34UL

/* Acquisition channel control */
#define ACQ_CORE_ACQ_CHAN_CTL 0x38UL
#define ACQ_CORE_ACQ_CHAN_CTL_WHICH_MASK 0x1fUL
#define ACQ_CORE_ACQ_CHAN_CTL_WHICH_SHIFT 0
#define ACQ_CORE_ACQ_CHAN_CTL_RESERVED_MASK 0xe0UL
#define ACQ_CORE_ACQ_CHAN_CTL_RESERVED_SHIFT 5
#define ACQ_CORE_ACQ_CHAN_CTL_DTRIG_WHICH_MASK 0x1f00UL
#define ACQ_CORE_ACQ_CHAN_CTL_DTRIG_WHICH_SHIFT 8
#define ACQ_CORE_ACQ_CHAN_CTL_RESERVED1_MASK 0xe000UL
#define ACQ_CORE_ACQ_CHAN_CTL_RESERVED1_SHIFT 13
#define ACQ_CORE_ACQ_CHAN_CTL_NUM_CHAN_MASK 0x1f0000UL
#define ACQ_CORE_ACQ_CHAN_CTL_NUM_CHAN_SHIFT 16
#define ACQ_CORE_ACQ_CHAN_CTL_RESERVED2_MASK 0xffe00000UL
#define ACQ_CORE_ACQ_CHAN_CTL_RESERVED2_SHIFT 21

/* Channel 0 Description */
#define ACQ_CORE_CH0_DESC 0x3cUL
#define ACQ_CORE_CH0_DESC_INT_WIDTH_MASK 0xffffUL
#define ACQ_CORE_CH0_DESC_INT_WIDTH_SHIFT 0
#define ACQ_CORE_CH0_DESC_NUM_COALESCE_MASK 0xffff0000UL
#define ACQ_CORE_CH0_DESC_NUM_COALESCE_SHIFT 16

/* Channel 0 Atom Description */
#define ACQ_CORE_CH0_ATOM_DESC 0x40UL
#define ACQ_CORE_CH0_ATOM_DESC_NUM_ATOMS_MASK 0xffffUL
#define ACQ_CORE_CH0_ATOM_DESC_NUM_ATOMS_SHIFT 0
#define ACQ_CORE_CH0_ATOM_DESC_ATOM_WIDTH_MASK 0xffff0000UL
#define ACQ_CORE_CH0_ATOM_DESC_ATOM_WIDTH_SHIFT 16

/* Channel 1 Description */
#define ACQ_CORE_CH1_DESC 0x44UL
#define ACQ_CORE_CH1_DESC_INT_WIDTH_MASK 0xffffUL
#define ACQ_CORE_CH1_DESC_INT_WIDTH_SHIFT 0
#define ACQ_CORE_CH1_DESC_NUM_COALESCE_MASK 0xffff0000UL
#define ACQ_CORE_CH1_DESC_NUM_COALESCE_SHIFT 16

/* Channel 1 Atom Description */
#define ACQ_CORE_CH1_ATOM_DESC 0x48UL
#define ACQ_CORE_CH1_ATOM_DESC_NUM_ATOMS_MASK 0xffffUL
#define ACQ_CORE_CH1_ATOM_DESC_NUM_ATOMS_SHIFT 0
#define ACQ_CORE_CH1_ATOM_DESC_ATOM_WIDTH_MASK 0xffff0000UL
#define ACQ_CORE_CH1_ATOM_DESC_ATOM_WIDTH_SHIFT 16

/* Channel 2 Description */
#define ACQ_CORE_CH2_DESC 0x4cUL
#define ACQ_CORE_CH2_DESC_INT_WIDTH_MASK 0xffffUL
#define ACQ_CORE_CH2_DESC_INT_WIDTH_SHIFT 0
#define ACQ_CORE_CH2_DESC_NUM_COALESCE_MASK 0xffff0000UL
#define ACQ_CORE_CH2_DESC_NUM_COALESCE_SHIFT 16

/* Channel 2 Atom Description */
#define ACQ_CORE_CH2_ATOM_DESC 0x50UL
#define ACQ_CORE_CH2_ATOM_DESC_NUM_ATOMS_MASK 0xffffUL
#define ACQ_CORE_CH2_ATOM_DESC_NUM_ATOMS_SHIFT 0
#define ACQ_CORE_CH2_ATOM_DESC_ATOM_WIDTH_MASK 0xffff0000UL
#define ACQ_CORE_CH2_ATOM_DESC_ATOM_WIDTH_SHIFT 16

/* Channel 3 Description */
#define ACQ_CORE_CH3_DESC 0x54UL
#define ACQ_CORE_CH3_DESC_INT_WIDTH_MASK 0xffffUL
#define ACQ_CORE_CH3_DESC_INT_WIDTH_SHIFT 0
#define ACQ_CORE_CH3_DESC_NUM_COALESCE_MASK 0xffff0000UL
#define ACQ_CORE_CH3_DESC_NUM_COALESCE_SHIFT 16

/* Channel 3 Atom Description */
#define ACQ_CORE_CH3_ATOM_DESC 0x58UL
#define ACQ_CORE_CH3_ATOM_DESC_NUM_ATOMS_MASK 0xffffUL
#define ACQ_CORE_CH3_ATOM_DESC_NUM_ATOMS_SHIFT 0
#define ACQ_CORE_CH3_ATOM_DESC_ATOM_WIDTH_MASK 0xffff0000UL
#define ACQ_CORE_CH3_ATOM_DESC_ATOM_WIDTH_SHIFT 16

/* Channel 4 Description */
#define ACQ_CORE_CH4_DESC 0x5cUL
#define ACQ_CORE_CH4_DESC_INT_WIDTH_MASK 0xffffUL
#define ACQ_CORE_CH4_DESC_INT_WIDTH_SHIFT 0
#define ACQ_CORE_CH4_DESC_NUM_COALESCE_MASK 0xffff0000UL
#define ACQ_CORE_CH4_DESC_NUM_COALESCE_SHIFT 16

/* Channel 4 Atom Description */
#define ACQ_CORE_CH4_ATOM_DESC 0x60UL
#define ACQ_CORE_CH4_ATOM_DESC_NUM_ATOMS_MASK 0xffffUL
#define ACQ_CORE_CH4_ATOM_DESC_NUM_ATOMS_SHIFT 0
#define ACQ_CORE_CH4_ATOM_DESC_ATOM_WIDTH_MASK 0xffff0000UL
#define ACQ_CORE_CH4_ATOM_DESC_ATOM_WIDTH_SHIFT 16

/* Channel 5 Description */
#define ACQ_CORE_CH5_DESC 0x64UL
#define ACQ_CORE_CH5_DESC_INT_WIDTH_MASK 0xffffUL
#define ACQ_CORE_CH5_DESC_INT_WIDTH_SHIFT 0
#define ACQ_CORE_CH5_DESC_NUM_COALESCE_MASK 0xffff0000UL
#define ACQ_CORE_CH5_DESC_NUM_COALESCE_SHIFT 16

/* Channel 5 Atom Description */
#define ACQ_CORE_CH5_ATOM_DESC 0x68UL
#define ACQ_CORE_CH5_ATOM_DESC_NUM_ATOMS_MASK 0xffffUL
#define ACQ_CORE_CH5_ATOM_DESC_NUM_ATOMS_SHIFT 0
#define ACQ_CORE_CH5_ATOM_DESC_ATOM_WIDTH_MASK 0xffff0000UL
#define ACQ_CORE_CH5_ATOM_DESC_ATOM_WIDTH_SHIFT 16

/* Channel 6 Description */
#define ACQ_CORE_CH6_DESC 0x6cUL
#define ACQ_CORE_CH6_DESC_INT_WIDTH_MASK 0xffffUL
#define ACQ_CORE_CH6_DESC_INT_WIDTH_SHIFT 0
#define ACQ_CORE_CH6_DESC_NUM_COALESCE_MASK 0xffff0000UL
#define ACQ_CORE_CH6_DESC_NUM_COALESCE_SHIFT 16

/* Channel 6 Atom Description */
#define ACQ_CORE_CH6_ATOM_DESC 0x70UL
#define ACQ_CORE_CH6_ATOM_DESC_NUM_ATOMS_MASK 0xffffUL
#define ACQ_CORE_CH6_ATOM_DESC_NUM_ATOMS_SHIFT 0
#define ACQ_CORE_CH6_ATOM_DESC_ATOM_WIDTH_MASK 0xffff0000UL
#define ACQ_CORE_CH6_ATOM_DESC_ATOM_WIDTH_SHIFT 16

/* Channel 7 Description */
#define ACQ_CORE_CH7_DESC 0x74UL
#define ACQ_CORE_CH7_DESC_INT_WIDTH_MASK 0xffffUL
#define ACQ_CORE_CH7_DESC_INT_WIDTH_SHIFT 0
#define ACQ_CORE_CH7_DESC_NUM_COALESCE_MASK 0xffff0000UL
#define ACQ_CORE_CH7_DESC_NUM_COALESCE_SHIFT 16

/* Channel 7 Atom Description */
#define ACQ_CORE_CH7_ATOM_DESC 0x78UL
#define ACQ_CORE_CH7_ATOM_DESC_NUM_ATOMS_MASK 0xffffUL
#define ACQ_CORE_CH7_ATOM_DESC_NUM_ATOMS_SHIFT 0
#define ACQ_CORE_CH7_ATOM_DESC_ATOM_WIDTH_MASK 0xffff0000UL
#define ACQ_CORE_CH7_ATOM_DESC_ATOM_WIDTH_SHIFT 16

/* Channel 8 Description */
#define ACQ_CORE_CH8_DESC 0x7cUL
#define ACQ_CORE_CH8_DESC_INT_WIDTH_MASK 0xffffUL
#define ACQ_CORE_CH8_DESC_INT_WIDTH_SHIFT 0
#define ACQ_CORE_CH8_DESC_NUM_COALESCE_MASK 0xffff0000UL
#define ACQ_CORE_CH8_DESC_NUM_COALESCE_SHIFT 16

/* Channel 8 Atom Description */
#define ACQ_CORE_CH8_ATOM_DESC 0x80UL
#define ACQ_CORE_CH8_ATOM_DESC_NUM_ATOMS_MASK 0xffffUL
#define ACQ_CORE_CH8_ATOM_DESC_NUM_ATOMS_SHIFT 0
#define ACQ_CORE_CH8_ATOM_DESC_ATOM_WIDTH_MASK 0xffff0000UL
#define ACQ_CORE_CH8_ATOM_DESC_ATOM_WIDTH_SHIFT 16

/* Channel 9 Description */
#define ACQ_CORE_CH9_DESC 0x84UL
#define ACQ_CORE_CH9_DESC_INT_WIDTH_MASK 0xffffUL
#define ACQ_CORE_CH9_DESC_INT_WIDTH_SHIFT 0
#define ACQ_CORE_CH9_DESC_NUM_COALESCE_MASK 0xffff0000UL
#define ACQ_CORE_CH9_DESC_NUM_COALESCE_SHIFT 16

/* Channel 9 Atom Description */
#define ACQ_CORE_CH9_ATOM_DESC 0x88UL
#define ACQ_CORE_CH9_ATOM_DESC_NUM_ATOMS_MASK 0xffffUL
#define ACQ_CORE_CH9_ATOM_DESC_NUM_ATOMS_SHIFT 0
#define ACQ_CORE_CH9_ATOM_DESC_ATOM_WIDTH_MASK 0xffff0000UL
#define ACQ_CORE_CH9_ATOM_DESC_ATOM_WIDTH_SHIFT 16

/* Channel 10 Description */
#define ACQ_CORE_CH10_DESC 0x8cUL
#define ACQ_CORE_CH10_DESC_INT_WIDTH_MASK 0xffffUL
#define ACQ_CORE_CH10_DESC_INT_WIDTH_SHIFT 0
#define ACQ_CORE_CH10_DESC_NUM_COALESCE_MASK 0xffff0000UL
#define ACQ_CORE_CH10_DESC_NUM_COALESCE_SHIFT 16

/* Channel 10 Atom Description */
#define ACQ_CORE_CH10_ATOM_DESC 0x90UL
#define ACQ_CORE_CH10_ATOM_DESC_NUM_ATOMS_MASK 0xffffUL
#define ACQ_CORE_CH10_ATOM_DESC_NUM_ATOMS_SHIFT 0
#define ACQ_CORE_CH10_ATOM_DESC_ATOM_WIDTH_MASK 0xffff0000UL
#define ACQ_CORE_CH10_ATOM_DESC_ATOM_WIDTH_SHIFT 16

/* Channel 11 Description */
#define ACQ_CORE_CH11_DESC 0x94UL
#define ACQ_CORE_CH11_DESC_INT_WIDTH_MASK 0xffffUL
#define ACQ_CORE_CH11_DESC_INT_WIDTH_SHIFT 0
#define ACQ_CORE_CH11_DESC_NUM_COALESCE_MASK 0xffff0000UL
#define ACQ_CORE_CH11_DESC_NUM_COALESCE_SHIFT 16

/* Channel 11 Atom Description */
#define ACQ_CORE_CH11_ATOM_DESC 0x98UL
#define ACQ_CORE_CH11_ATOM_DESC_NUM_ATOMS_MASK 0xffffUL
#define ACQ_CORE_CH11_ATOM_DESC_NUM_ATOMS_SHIFT 0
#define ACQ_CORE_CH11_ATOM_DESC_ATOM_WIDTH_MASK 0xffff0000UL
#define ACQ_CORE_CH11_ATOM_DESC_ATOM_WIDTH_SHIFT 16

/* Channel 12 Description */
#define ACQ_CORE_CH12_DESC 0x9cUL
#define ACQ_CORE_CH12_DESC_INT_WIDTH_MASK 0xffffUL
#define ACQ_CORE_CH12_DESC_INT_WIDTH_SHIFT 0
#define ACQ_CORE_CH12_DESC_NUM_COALESCE_MASK 0xffff0000UL
#define ACQ_CORE_CH12_DESC_NUM_COALESCE_SHIFT 16

/* Channel 12 Atom Description */
#define ACQ_CORE_CH12_ATOM_DESC 0xa0UL
#define ACQ_CORE_CH12_ATOM_DESC_NUM_ATOMS_MASK 0xffffUL
#define ACQ_CORE_CH12_ATOM_DESC_NUM_ATOMS_SHIFT 0
#define ACQ_CORE_CH12_ATOM_DESC_ATOM_WIDTH_MASK 0xffff0000UL
#define ACQ_CORE_CH12_ATOM_DESC_ATOM_WIDTH_SHIFT 16

/* Channel 13 Description */
#define ACQ_CORE_CH13_DESC 0xa4UL
#define ACQ_CORE_CH13_DESC_INT_WIDTH_MASK 0xffffUL
#define ACQ_CORE_CH13_DESC_INT_WIDTH_SHIFT 0
#define ACQ_CORE_CH13_DESC_NUM_COALESCE_MASK 0xffff0000UL
#define ACQ_CORE_CH13_DESC_NUM_COALESCE_SHIFT 16

/* Channel 13 Atom Description */
#define ACQ_CORE_CH13_ATOM_DESC 0xa8UL
#define ACQ_CORE_CH13_ATOM_DESC_NUM_ATOMS_MASK 0xffffUL
#define ACQ_CORE_CH13_ATOM_DESC_NUM_ATOMS_SHIFT 0
#define ACQ_CORE_CH13_ATOM_DESC_ATOM_WIDTH_MASK 0xffff0000UL
#define ACQ_CORE_CH13_ATOM_DESC_ATOM_WIDTH_SHIFT 16

/* Channel 14 Description */
#define ACQ_CORE_CH14_DESC 0xacUL
#define ACQ_CORE_CH14_DESC_INT_WIDTH_MASK 0xffffUL
#define ACQ_CORE_CH14_DESC_INT_WIDTH_SHIFT 0
#define ACQ_CORE_CH14_DESC_NUM_COALESCE_MASK 0xffff0000UL
#define ACQ_CORE_CH14_DESC_NUM_COALESCE_SHIFT 16

/* Channel 14 Atom Description */
#define ACQ_CORE_CH14_ATOM_DESC 0xb0UL
#define ACQ_CORE_CH14_ATOM_DESC_NUM_ATOMS_MASK 0xffffUL
#define ACQ_CORE_CH14_ATOM_DESC_NUM_ATOMS_SHIFT 0
#define ACQ_CORE_CH14_ATOM_DESC_ATOM_WIDTH_MASK 0xffff0000UL
#define ACQ_CORE_CH14_ATOM_DESC_ATOM_WIDTH_SHIFT 16

/* Channel 15 Description */
#define ACQ_CORE_CH15_DESC 0xb4UL
#define ACQ_CORE_CH15_DESC_INT_WIDTH_MASK 0xffffUL
#define ACQ_CORE_CH15_DESC_INT_WIDTH_SHIFT 0
#define ACQ_CORE_CH15_DESC_NUM_COALESCE_MASK 0xffff0000UL
#define ACQ_CORE_CH15_DESC_NUM_COALESCE_SHIFT 16

/* Channel 15 Atom Description */
#define ACQ_CORE_CH15_ATOM_DESC 0xb8UL
#define ACQ_CORE_CH15_ATOM_DESC_NUM_ATOMS_MASK 0xffffUL
#define ACQ_CORE_CH15_ATOM_DESC_NUM_ATOMS_SHIFT 0
#define ACQ_CORE_CH15_ATOM_DESC_ATOM_WIDTH_MASK 0xffff0000UL
#define ACQ_CORE_CH15_ATOM_DESC_ATOM_WIDTH_SHIFT 16

/* Channel 16 Description */
#define ACQ_CORE_CH16_DESC 0xbcUL
#define ACQ_CORE_CH16_DESC_INT_WIDTH_MASK 0xffffUL
#define ACQ_CORE_CH16_DESC_INT_WIDTH_SHIFT 0
#define ACQ_CORE_CH16_DESC_NUM_COALESCE_MASK 0xffff0000UL
#define ACQ_CORE_CH16_DESC_NUM_COALESCE_SHIFT 16

/* Channel 16 Atom Description */
#define ACQ_CORE_CH16_ATOM_DESC 0xc0UL
#define ACQ_CORE_CH16_ATOM_DESC_NUM_ATOMS_MASK 0xffffUL
#define ACQ_CORE_CH16_ATOM_DESC_NUM_ATOMS_SHIFT 0
#define ACQ_CORE_CH16_ATOM_DESC_ATOM_WIDTH_MASK 0xffff0000UL
#define ACQ_CORE_CH16_ATOM_DESC_ATOM_WIDTH_SHIFT 16

/* Channel 17 Description */
#define ACQ_CORE_CH17_DESC 0xc4UL
#define ACQ_CORE_CH17_DESC_INT_WIDTH_MASK 0xffffUL
#define ACQ_CORE_CH17_DESC_INT_WIDTH_SHIFT 0
#define ACQ_CORE_CH17_DESC_NUM_COALESCE_MASK 0xffff0000UL
#define ACQ_CORE_CH17_DESC_NUM_COALESCE_SHIFT 16

/* Channel 17 Atom Description */
#define ACQ_CORE_CH17_ATOM_DESC 0xc8UL
#define ACQ_CORE_CH17_ATOM_DESC_NUM_ATOMS_MASK 0xffffUL
#define ACQ_CORE_CH17_ATOM_DESC_NUM_ATOMS_SHIFT 0
#define ACQ_CORE_CH17_ATOM_DESC_ATOM_WIDTH_MASK 0xffff0000UL
#define ACQ_CORE_CH17_ATOM_DESC_ATOM_WIDTH_SHIFT 16

/* Channel 18 Description */
#define ACQ_CORE_CH18_DESC 0xccUL
#define ACQ_CORE_CH18_DESC_INT_WIDTH_MASK 0xffffUL
#define ACQ_CORE_CH18_DESC_INT_WIDTH_SHIFT 0
#define ACQ_CORE_CH18_DESC_NUM_COALESCE_MASK 0xffff0000UL
#define ACQ_CORE_CH18_DESC_NUM_COALESCE_SHIFT 16

/* Channel 18 Atom Description */
#define ACQ_CORE_CH18_ATOM_DESC 0xd0UL
#define ACQ_CORE_CH18_ATOM_DESC_NUM_ATOMS_MASK 0xffffUL
#define ACQ_CORE_CH18_ATOM_DESC_NUM_ATOMS_SHIFT 0
#define ACQ_CORE_CH18_ATOM_DESC_ATOM_WIDTH_MASK 0xffff0000UL
#define ACQ_CORE_CH18_ATOM_DESC_ATOM_WIDTH_SHIFT 16

/* Channel 19 Description */
#define ACQ_CORE_CH19_DESC 0xd4UL
#define ACQ_CORE_CH19_DESC_INT_WIDTH_MASK 0xffffUL
#define ACQ_CORE_CH19_DESC_INT_WIDTH_SHIFT 0
#define ACQ_CORE_CH19_DESC_NUM_COALESCE_MASK 0xffff0000UL
#define ACQ_CORE_CH19_DESC_NUM_COALESCE_SHIFT 16

/* Channel 19 Atom Description */
#define ACQ_CORE_CH19_ATOM_DESC 0xd8UL
#define ACQ_CORE_CH19_ATOM_DESC_NUM_ATOMS_MASK 0xffffUL
#define ACQ_CORE_CH19_ATOM_DESC_NUM_ATOMS_SHIFT 0
#define ACQ_CORE_CH19_ATOM_DESC_ATOM_WIDTH_MASK 0xffff0000UL
#define ACQ_CORE_CH19_ATOM_DESC_ATOM_WIDTH_SHIFT 16

/* Channel 20 Description */
#define ACQ_CORE_CH20_DESC 0xdcUL
#define ACQ_CORE_CH20_DESC_INT_WIDTH_MASK 0xffffUL
#define ACQ_CORE_CH20_DESC_INT_WIDTH_SHIFT 0
#define ACQ_CORE_CH20_DESC_NUM_COALESCE_MASK 0xffff0000UL
#define ACQ_CORE_CH20_DESC_NUM_COALESCE_SHIFT 16

/* Channel 20 Atom Description */
#define ACQ_CORE_CH20_ATOM_DESC 0xe0UL
#define ACQ_CORE_CH20_ATOM_DESC_NUM_ATOMS_MASK 0xffffUL
#define ACQ_CORE_CH20_ATOM_DESC_NUM_ATOMS_SHIFT 0
#define ACQ_CORE_CH20_ATOM_DESC_ATOM_WIDTH_MASK 0xffff0000UL
#define ACQ_CORE_CH20_ATOM_DESC_ATOM_WIDTH_SHIFT 16

/* Channel 21 Description */
#define ACQ_CORE_CH21_DESC 0xe4UL
#define ACQ_CORE_CH21_DESC_INT_WIDTH_MASK 0xffffUL
#define ACQ_CORE_CH21_DESC_INT_WIDTH_SHIFT 0
#define ACQ_CORE_CH21_DESC_NUM_COALESCE_MASK 0xffff0000UL
#define ACQ_CORE_CH21_DESC_NUM_COALESCE_SHIFT 16

/* Channel 21 Atom Description */
#define ACQ_CORE_CH21_ATOM_DESC 0xe8UL
#define ACQ_CORE_CH21_ATOM_DESC_NUM_ATOMS_MASK 0xffffUL
#define ACQ_CORE_CH21_ATOM_DESC_NUM_ATOMS_SHIFT 0
#define ACQ_CORE_CH21_ATOM_DESC_ATOM_WIDTH_MASK 0xffff0000UL
#define ACQ_CORE_CH21_ATOM_DESC_ATOM_WIDTH_SHIFT 16

/* Channel 22 Description */
#define ACQ_CORE_CH22_DESC 0xecUL
#define ACQ_CORE_CH22_DESC_INT_WIDTH_MASK 0xffffUL
#define ACQ_CORE_CH22_DESC_INT_WIDTH_SHIFT 0
#define ACQ_CORE_CH22_DESC_NUM_COALESCE_MASK 0xffff0000UL
#define ACQ_CORE_CH22_DESC_NUM_COALESCE_SHIFT 16

/* Channel 22 Atom Description */
#define ACQ_CORE_CH22_ATOM_DESC 0xf0UL
#define ACQ_CORE_CH22_ATOM_DESC_NUM_ATOMS_MASK 0xffffUL
#define ACQ_CORE_CH22_ATOM_DESC_NUM_ATOMS_SHIFT 0
#define ACQ_CORE_CH22_ATOM_DESC_ATOM_WIDTH_MASK 0xffff0000UL
#define ACQ_CORE_CH22_ATOM_DESC_ATOM_WIDTH_SHIFT 16

/* Channel 23 Description */
#define ACQ_CORE_CH23_DESC 0xf4UL
#define ACQ_CORE_CH23_DESC_INT_WIDTH_MASK 0xffffUL
#define ACQ_CORE_CH23_DESC_INT_WIDTH_SHIFT 0
#define ACQ_CORE_CH23_DESC_NUM_COALESCE_MASK 0xffff0000UL
#define ACQ_CORE_CH23_DESC_NUM_COALESCE_SHIFT 16

/* Channel 23 Atom Description */
#define ACQ_CORE_CH23_ATOM_DESC 0xf8UL
#define ACQ_CORE_CH23_ATOM_DESC_NUM_ATOMS_MASK 0xffffUL
#define ACQ_CORE_CH23_ATOM_DESC_NUM_ATOMS_SHIFT 0
#define ACQ_CORE_CH23_ATOM_DESC_ATOM_WIDTH_MASK 0xffff0000UL
#define ACQ_CORE_CH23_ATOM_DESC_ATOM_WIDTH_SHIFT 16

struct acq_core {
  /* [0x0]: REG (rw) Control register */
  uint32_t ctl;

  /* [0x4]: REG (ro) Status register */
  uint32_t sta;

  /* [0x8]: REG (rw) Trigger configuration */
  uint32_t trig_cfg;

  /* [0xc]: REG (rw) Trigger data config threshold */
  uint32_t trig_data_cfg;

  /* [0x10]: REG (rw) Trigger data threshold */
  uint32_t trig_data_thres;

  /* [0x14]: REG (rw) Trigger delay */
  uint32_t trig_dly;

  /* [0x18]: REG (wo) Software trigger */
  uint32_t sw_trig;

  /* [0x1c]: REG (rw) Number of shots */
  uint32_t shots;

  /* [0x20]: REG (ro) Trigger address register */
  uint32_t trig_pos;

  /* [0x24]: REG (rw) Pre-trigger samples */
  uint32_t pre_samples;

  /* [0x28]: REG (rw) Post-trigger samples */
  uint32_t post_samples;

  /* [0x2c]: REG (ro) Samples counter */
  uint32_t samples_cnt;

  /* [0x30]: REG (rw) DDR3 Start Address */
  uint32_t ddr3_start_addr;

  /* [0x34]: REG (rw) DDR3 End Address */
  uint32_t ddr3_end_addr;

  /* [0x38]: REG (rw) Acquisition channel control */
  uint32_t acq_chan_ctl;

  /* [0x3c]: REG (ro) Channel 0 Description */
  uint32_t ch0_desc;

  /* [0x40]: REG (ro) Channel 0 Atom Description */
  uint32_t ch0_atom_desc;

  /* [0x44]: REG (ro) Channel 1 Description */
  uint32_t ch1_desc;

  /* [0x48]: REG (ro) Channel 1 Atom Description */
  uint32_t ch1_atom_desc;

  /* [0x4c]: REG (ro) Channel 2 Description */
  uint32_t ch2_desc;

  /* [0x50]: REG (ro) Channel 2 Atom Description */
  uint32_t ch2_atom_desc;

  /* [0x54]: REG (ro) Channel 3 Description */
  uint32_t ch3_desc;

  /* [0x58]: REG (ro) Channel 3 Atom Description */
  uint32_t ch3_atom_desc;

  /* [0x5c]: REG (ro) Channel 4 Description */
  uint32_t ch4_desc;

  /* [0x60]: REG (ro) Channel 4 Atom Description */
  uint32_t ch4_atom_desc;

  /* [0x64]: REG (ro) Channel 5 Description */
  uint32_t ch5_desc;

  /* [0x68]: REG (ro) Channel 5 Atom Description */
  uint32_t ch5_atom_desc;

  /* [0x6c]: REG (ro) Channel 6 Description */
  uint32_t ch6_desc;

  /* [0x70]: REG (ro) Channel 6 Atom Description */
  uint32_t ch6_atom_desc;

  /* [0x74]: REG (ro) Channel 7 Description */
  uint32_t ch7_desc;

  /* [0x78]: REG (ro) Channel 7 Atom Description */
  uint32_t ch7_atom_desc;

  /* [0x7c]: REG (ro) Channel 8 Description */
  uint32_t ch8_desc;

  /* [0x80]: REG (ro) Channel 8 Atom Description */
  uint32_t ch8_atom_desc;

  /* [0x84]: REG (ro) Channel 9 Description */
  uint32_t ch9_desc;

  /* [0x88]: REG (ro) Channel 9 Atom Description */
  uint32_t ch9_atom_desc;

  /* [0x8c]: REG (ro) Channel 10 Description */
  uint32_t ch10_desc;

  /* [0x90]: REG (ro) Channel 10 Atom Description */
  uint32_t ch10_atom_desc;

  /* [0x94]: REG (ro) Channel 11 Description */
  uint32_t ch11_desc;

  /* [0x98]: REG (ro) Channel 11 Atom Description */
  uint32_t ch11_atom_desc;

  /* [0x9c]: REG (ro) Channel 12 Description */
  uint32_t ch12_desc;

  /* [0xa0]: REG (ro) Channel 12 Atom Description */
  uint32_t ch12_atom_desc;

  /* [0xa4]: REG (ro) Channel 13 Description */
  uint32_t ch13_desc;

  /* [0xa8]: REG (ro) Channel 13 Atom Description */
  uint32_t ch13_atom_desc;

  /* [0xac]: REG (ro) Channel 14 Description */
  uint32_t ch14_desc;

  /* [0xb0]: REG (ro) Channel 14 Atom Description */
  uint32_t ch14_atom_desc;

  /* [0xb4]: REG (ro) Channel 15 Description */
  uint32_t ch15_desc;

  /* [0xb8]: REG (ro) Channel 15 Atom Description */
  uint32_t ch15_atom_desc;

  /* [0xbc]: REG (ro) Channel 16 Description */
  uint32_t ch16_desc;

  /* [0xc0]: REG (ro) Channel 16 Atom Description */
  uint32_t ch16_atom_desc;

  /* [0xc4]: REG (ro) Channel 17 Description */
  uint32_t ch17_desc;

  /* [0xc8]: REG (ro) Channel 17 Atom Description */
  uint32_t ch17_atom_desc;

  /* [0xcc]: REG (ro) Channel 18 Description */
  uint32_t ch18_desc;

  /* [0xd0]: REG (ro) Channel 18 Atom Description */
  uint32_t ch18_atom_desc;

  /* [0xd4]: REG (ro) Channel 19 Description */
  uint32_t ch19_desc;

  /* [0xd8]: REG (ro) Channel 19 Atom Description */
  uint32_t ch19_atom_desc;

  /* [0xdc]: REG (ro) Channel 20 Description */
  uint32_t ch20_desc;

  /* [0xe0]: REG (ro) Channel 20 Atom Description */
  uint32_t ch20_atom_desc;

  /* [0xe4]: REG (ro) Channel 21 Description */
  uint32_t ch21_desc;

  /* [0xe8]: REG (ro) Channel 21 Atom Description */
  uint32_t ch21_atom_desc;

  /* [0xec]: REG (ro) Channel 22 Description */
  uint32_t ch22_desc;

  /* [0xf0]: REG (ro) Channel 22 Atom Description */
  uint32_t ch22_atom_desc;

  /* [0xf4]: REG (ro) Channel 23 Description */
  uint32_t ch23_desc;

  /* [0xf8]: REG (ro) Channel 23 Atom Description */
  uint32_t ch23_atom_desc;
};

#endif /* __CHEBY__ACQ_CORE__H__ */
