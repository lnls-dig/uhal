# AFC Boards

All BPM crates at LNLS use AMCs called AFCs, which share gateware
implementation strategies.

## Hardware interface

The gateware projects for the AFC boards used at LNLS have a standardized
interface for accessing their registers and onboard memory. These boards are
connected to the crate's host CPU via PCIe, and there are three available BARs,
which provide all necessary controls for these boards.

- `BAR0`: general PCIe control registers, paging index for other BARs, and DMA
  control;
- `BAR2`: access to onboard RAM;
- `BAR4`: application-specific registers.

The natural unit for accessing these BARs is 32-bit words; byte addressing
doesn't seem to work well, but wider accesses, at least for `BAR2`, do work.

### BAR0

The general PCIe control registers often don't need to be used; it is possible,
however, to reset the PCIe communication by writing into one of the `BAR0`
registers.

`BAR2` and `BAR4` have a limited size (e.g. 1MiB and 512KiB, respectively), but
the full address space for the RAM and for the application specific registers
is considerably larger (e.g. 2GiB for RAM). This makes it necessary to have a
mechanism to determine to which region of the underlying memory that the BAR is
actually pointing; this is done by writing into the paging registers located in
`BAR0`.

The PCIe core used in the AFC boards has DMA capabilities. Only one transaction
per board is supported at a time, and the DMA core requires coherent memory
(either by pointing to physical memory or by using IOMMU features), since its
parameters are only the host memory address and amount of bytes (i.e. there's
no scatter-gather support).

### BAR2

`BAR2` exposes the onboard RAM, which is used to store acquisitions from the
acquisition core. This way, acquisition results can be accessed. The RAM has a
limited size, and must be shared by all acquisition cores, which the managing
software must explicitly do by configuring the addresses to which each
acquisition core can write.

This memory region can be mapped by the host OS as write-combining, which,
despite the name, can also aid performance for reads, since reading multiple
words at once decreases the packet overhead and latency costs. Reading from
`BAR2` using SSE4.1 SIMD instructions is implemented in this project, although
it is not clear whether the measured performance improvements are definitely
caused by the write-combining feature.

### BAR4

Most of the regular interaction with an AFC board and its cores goes through
`BAR4`. It exposes an underlying Wishbone bus (which is mostly abstracted
away), to which an SDB (Self Describing Bus) filesystem and all FPGA cores are
connected.

- SDB: the SDB filesystem is a read-only structure which is responsible for
  documenting the location of each FPGA core in the address space of the
  Wishbone bus. It includes information about their hierarchical organization,
  device and vendor IDs, and version. The SDB also includes information about
  the particular bitstream synthesis: build date, tooling version, author, and
  commit.
- FPGA cores: these are logical units within the gateware, which are
  responsible for specific features and/or devices. The controls for each core
  are part of a core-specific register map, which is then mapped into the
  Wishbone address space. This way, one can easily find a core's register in
  order to read from or write into them.

#### Register maps

The register maps for each FPGA core are defined using
[Cheby](https://gitlab.cern.ch/be-cem-edl/common/cheby). Some legacy cores used
[wbgen2](https://ohwr.org/project/wishbone-gen).

These tools generate VHDL code implementing these register maps, as well as
HTML documentation and C headers. Along with the register address macros and
register field bitmasks, Cheby also includes a C struct definition in the
generated header.

These registers can be read-only, read-write, and "strobes" --- which receive
writes but immediately clear the written bit.

#### Addressing oddity

Accessing the registers in `BAR4` requires manipulation of their addresses,
because it shifts addresses 3 bits to the right. Therefore, in order to access
a 32-bit word located at address `0x0100` (per the SDB), for example, one must
access `0x0100 << 3 = 0x0800`; and, in order to access the next 32-bit word,
one must access `(0x0100 + 4) << 3 = 0x0820`.
