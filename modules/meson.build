modules_src = ['acq.cc', 'afc_timing.cc', 'lamp.cc', 'fofb_cc.cc', 'fofb_processing.cc', 'sysid.cc', 'trigger_iface.cc', 'trigger_mux.cc']
modules_lib = static_library(
  'uhal-modules',
  modules_src,
  dependencies: [utilities],
  include_directories: install_inc,
  install: true
)
modules = declare_dependency(link_with: modules_lib, include_directories: install_inc)
