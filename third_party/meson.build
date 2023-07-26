sdbfs_inc = include_directories(['sdbfs/lib', 'sdbfs/include/linux'])

cc = meson.get_compiler('c')
c_args = []
if cc.has_argument('-Wno-language-extension-token')
  c_args += '-Wno-language-extension-token'
endif

sdbfs_lib = static_library(
  'sdbfs',
  ['sdbfs/lib/access.c', 'sdbfs/lib/glue.c'],
  include_directories: sdbfs_inc,
  c_args: c_args
)
sdbfs = declare_dependency(link_with: sdbfs_lib, include_directories: sdbfs_inc)
