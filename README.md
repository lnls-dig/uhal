# decode-reg

This program performs high level operations with our PCI devices, without the
need for underlying daemons or anything of the sort.

## How to build

Use Make (`make`) or Meson (`meson build; ninja -C build`). Export
`LDFLAGS=-static` to create a static binary that can be exported to other
machines. This is extra relevant because the utility uses C++17 features, which
might not be available with the installed compiler/runtime on an older
platform.

## How to use

Run `decode-reg` with the `-h` option to see the available arguments. Due to
bugs in argument parsing, provide flags and arguments in the order shown in the
help output. No special flags, and the utility reads ACQ registers. With `-r`,
it performs an acquisition using the ACQ core, and with `-x`, it writes
configuration parameters into the LAMP registers.
