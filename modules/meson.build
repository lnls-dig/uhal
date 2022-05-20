modules_src = ['acq.cc', 'lamp_v1.cc', 'lamp_v2.cc']
modules_lib = static_library('modules', modules_src, dependencies: [utilities])
modules_inc = include_directories('.')

modules = declare_dependency(link_with: modules_lib, include_directories: modules_inc)
