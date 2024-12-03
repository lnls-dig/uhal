# μHAL Design

A [primer on the hardware](hardware.md) with which this project interfaces
should be useful in understanding the design choices made.

## Low level interface

The low level interface consists simply of a `struct pcie_bars` object, which
allows its user to read and write from BARs 2 and 4 (`BAR0` is considered an
implementation detail) using the functions defined in `util/pcie.h`.

`BAR4` can also be accessed when `struct pcie_bars` points to a serial port,
which is used when debugging boards outside of a MTCA crate. The serial port
doesn't allow access to `BAR2`, though.

All other functionality of this project depends on this low level interface.

## Basic abstractions

### SDB access

SDB access is provided by the functions in `util/util_sdb.h`. These functions
are higher level abstractions on top of the `libsdbfs` project.

Unlike HALCS, which iterated through the SBD and launched a handler for each
core it found, the only function in this library which iterates over the SDB is
the one which prints its contents. For any other use, users of the library will
search for specific cores (with their index in depth-first traversal) with
`read_sdb()`. In order to know which cores should be available in a given
board, users should consult the build information provided by
`get_synthesis_info()`.

While this implementation might seem less flexible at first, when using this
library on an IOC, it's impossible to escape from the need to know what cores
are available: records for them must be instantiated and have the proper names.
Therefore, this isn't adding any new limitations to an IOC.

### RegisterDecoder

The `class RegisterDecoder` defines a common interface to the FPGA cores on the
AFC board. This class fulfils two main roles:

- exposing register fields to a library user, who can address them by name (a
  string) and index (a number). This is provided by the
  `RegisterDecoder::get_generic_data()` method;
- printing register fields in a common format. This is provided by the
  `RegisterDecoder::print()` method.

These register fields 
