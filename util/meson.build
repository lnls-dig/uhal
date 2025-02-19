utilities_src = [
  'controllers.cc',
  'decoderbase.cc',
  'decoders.cc',
  'pcie-open.cc',
  'pcie.c',
  'printer.cc',
  'sdb.cc',
  'si57x_util.cc',
  'util.cc',
]
utilities_lib = static_library(
  'uhal-utilities',
  utilities_src,
  dependencies: [sdbfs, ordered_map],
  install: true
)
utilities_inc = include_directories('.')
utilities = declare_dependency(
  link_with: utilities_lib,
  include_directories: utilities_inc,
  dependencies: ordered_map
)

if build_tests
  subdir('tests')
endif

install_headers(
  [
    'controllers.h',
    'decoders.h',
    'pcie-defs.h',
    'pcie-open.h',
    'sdb-defs.h',
    'util_sdb.h',
  ],
  subdir: header_dir
)
