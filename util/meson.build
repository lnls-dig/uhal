utilities_src = ['decoders.cc', 'pcie.c', 'pcie-open.cc', 'printer.cc', 'util.cc']
utilities_lib = static_library('utilities', utilities_src, dependencies: [sdbfs])
utilities_inc = include_directories('.')
utilities = declare_dependency(link_with: utilities_lib, include_directories: utilities_inc)
