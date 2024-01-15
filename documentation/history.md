# μHAL History

## Motivation

The [Sirius synchrotron light source](https://lnls.cnpem.br/) uses MicroTCA
crates for its Beam Position Monitor (BPM) and Fast Orbit Feedback (FOFB)
systems. These systems use multiple Advanced Mezzanine Cards (AMC) called AMC
FMC Carriers (AFC), which feature FPGAs and expansion connectors for supporting
custom hardware. There is a PCIe connection between a crate's FPGAs and its
host CPU.

There are 4 kinds of AFCs plugged into each crate:

- Timing receiver
- Electron BPM (RFBPM)
- Photon BPM (XBPM)
- FOFB Controller

It is essential to expose each AFC's features and controls as
[EPICS](https://epics-controls.org/) Process Variables (PV), which makes it
necessary to provide some kind of interface an EPICS Input/Output Controller
(IOC) can use to interact with the hardware.

## Prior Art

### HALCS

The first sofware suite in deployment at Sirius for controlling these AFCs was
based on the [Hardware Abstraction Layer for Control Systems
(HALCS)](https://github.com/lnls-dig/halcs). HALCS depends on a [custom kernel
driver](https://github.com/lnls-dig/fpga_pcie_driver) which creates
`/dev/fpga-X` devices for each one of the AFCs. A daemon is launched for each
board, which opens the corresponding `/dev/fpga-X` device, automatically
discovers the available modules in the board using the embedded
[Self-Describing Bus (SDB) file
system](https://ohwr.org/project/sdb/wikis/home), and exposes the hardware
features via the Malamute broker, using ZeroMQ as the transport.

The HALCS daemon multiplexes access to the hardware, which is essential, given
that it isn't safe to control the same board from two different processes. The
main "consumer" for HALCS is the corresponding IOC(s): [Timing receiver
IOC](https://github.com/lnls-dig/tim-rx-epics-ioc), [BPM
IOC](https://github.com/lnls-dig/bpm-epics-ioc) and [FOFB
IOC](https://github.com/lnls-dig/fofb-epics-ioc); it should be noted that there
are 2 BPM IOCs instances for each AFC. There are also many utilities provided
by HALCS which connect to the HALCS daemon to implement some specific
functionality or test. All of these use the HALCS client library.

#### Shortcomings

1. **Kernel module**: the dependency on a custom kernel driver, which is only
   maintained internally, left us stuck on a Linux 4.4 kernel, and,
   consequently, stopped us from updating the host's Linux distribution.
2. **Malamute broker**: the official Malamute project was archived, meaning it
   also had to be maintained internally. Unfortunately, it's a very complex
   codebase, and suffers from serious bugs: disconnection and reconnection to
   the broker doesn't always work correctly, requiring restarting either the
   HALCS daemon or an IOC; and in some cases a memory leak is triggered, which
   either locks up the host in swap thrashing, or, when the service is memory
   limited, affects --- and basically impedes --- its functionality.
3. **Library interface**: dealing with signed values has to be done by library
   users, because all the library interfaces use `uint32_t` types; in addition,
   one and two-byte signed values have to be sign extended by library users.
   Furthermore, fixed-point registers aren't handled at the HALCS layer, so
   library users have to implement conversion to and from floating-point.
4. **Maintainability (HALCS)**: the way HALCS deals with RPC over ZeroMQ is
   boilerplate heavy and not very flexible. Adding the read and write functions
   for a given register field requires making coordinated changes to 5 files,
   with no build-time checks for correctness. HALCS stack traces are very
   complicated as well, due to the many layers of indirection, coupled with the
   object-oriented nature of the C code.
5. **Maintainability (IOC)**: HALCS based IOCs have a monolithic code
   structure, which makes code sharing between IOCs a manual and error-prone
   process, and requires manual propagation of bug fixes and new features.

## μHAL Design

In order to tackle these shortcomings, it was decided to start fresh with a new
project, in the form of a library for accessing the hardware. Many of the
design decisions were guided by the shortcomings of the existing software, by
operation experience with the controlled devices, and by general
maintainability concerns. Each specific shortcoming is addressed below:

1. To avoid the need for a custom kernel module, it was decided that we could
   (at least as a first step) forego DMA support in our application, and depend
   solely on standard Linux features; one of these features is the ability to
   map PCIe BARs via special files in `sysfs`.
2. Since RPC tends to require additional boilerplate and error handling, it was
   decided that any form of hardware multiplexing would happen at the
   middleware layer, via something like an EPICS IOC. This way, there aren't
   more RPC layers than strictly necessary, and the variety of error modes for
   internal library calls is constrained.
3. Default to a signed type for integers, `int32_t`, which covers most uses
   (and is the biggest integer EPICS 3 records even support). Sign extension
   for one and two-byte signed values is also implemented, and library users
   always interact with fixed-point register fields using `double` values,
   which are converted by the library itself.
4. Adding a new register field to be read requires changes to just one file;
   and if it has to be written, at most two files have to be changed.
5. Though the library supports any kind of control system integration, its main
   use was intended to be through the EPICS module Asyn. Therefore, the code
   organization was such that the IOC could be implemented following the same
   module structure as the library, and fetching values from hardware used the
   same strings used for identifying Asyn parameters.
