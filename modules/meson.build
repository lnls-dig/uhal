modules_src = ['acq.cc', 'lamp.cc', 'fofb_cc.cc', 'fofb_processing.cc', 'trigger_iface.cc', 'trigger_mux.cc']
modules_lib = static_library(
  'uhal-modules',
  modules_src,
  dependencies: [utilities],
  install: true
)
modules_inc = include_directories('.')
modules = declare_dependency(link_with: modules_lib, include_directories: modules_inc)

install_headers(
  ['acq.h', 'fofb_cc.h', 'fofb_processing.h', 'lamp.h', 'trigger_iface.h', 'trigger_mux.h'],
  subdir: header_dir
)
