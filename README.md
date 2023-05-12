# μHAL

This project implements a Hardware Abstraction Layer (HAL) for LNLS's FPGA
devices, which use the PCIe bus and conform to the MicroTCA (μTCA) standard. No
daemons or kernel drivers are necessary.

Along with the main library, we have the `decode-reg` utility, which can read
and write into hardware registers -- with proper encoding and decoding --
enabling inspection and debugging of devices as well as directly controlling
them.

## How to build

This projects uses the Meson build system. The build steps are simple:

```
$ meson setup build
$ ninja -C build
```

You can export `LDFLAGS=-static` to the `meson` command to create a static
binary that can be exported to other machines. This is relevant because we use
many C++17 features, which might not be available with the installed
compiler/runtime on an older platform.

## How to install

In order to install only this project's files, and not any subproject's, the
command below can be used:

```
$ meson install -C build/ --skip-subprojects
```

## How to use

Run `decode-reg` without any arguments to see the available operation modes.
With the operation mode selected, the `-h` flag can be used to view all the
available options.
