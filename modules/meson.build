modules_src = [
  'acq.cc',
  'afc_timing.cc',
  'bpm_swap.cc',
  'fmc250m_4ch.cc',
  'fmc_active_clk.cc',
  'fmc_adc_common.cc',
  'fmcpico1m_4ch.cc',
  'fofb_cc.cc',
  'fofb_processing.cc',
  'fofb_shaper_filt.cc',
  'lamp.cc',
  'orbit_intlk.cc',
  'pos_calc.cc',
  'si57x_ctrl.cc',
  'spi.cc',
  'sysid.cc',
  'trigger_iface.cc',
  'trigger_mux.cc',
]
modules_lib = static_library(
  'uhal-modules',
  modules_src,
  dependencies: [utilities],
  include_directories: install_inc,
  install: true
)
modules = declare_dependency(link_with: modules_lib, include_directories: install_inc)
