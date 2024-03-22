#ifndef CONTROLLERS_H
#define CONTROLLERS_H

#include "decoders.h"
#include "sdb-defs.h"

class RegisterController: public RegisterDecoderBase {
  protected:
    RegisterController(struct pcie_bars &bars, const struct sdb_device_info &);

    /** Child classes can implement this function to capture one-time values
     * from hardware and perform any other necessary initialization */
    virtual void set_devinfo_callback() { }

    /** Child classes must implement this function to encode their
     * configuration parameters into hardware registers */
    virtual void encode_params() = 0;

    /** Child classes can implement this function to unset any commands (e.g.
     * reset commands) which cause side-effects on every write */
    virtual void unset_commands() {}

  public:
    void set_devinfo(const struct sdb_device_info &) override;

    /** Child classes can implement this function when their write procedures
     * require more than simply writing the regs structure */
    virtual void write_params();
};

class RegisterDecoderController: public RegisterController {
    RegisterDecoder *pdec;

  protected:
    RegisterDecoderController(
        struct pcie_bars &bars,
        const struct sdb_device_info &devinfo,
        RegisterDecoder *pdec):
        RegisterController(bars, devinfo),
        pdec(pdec)
    {
    }

  public:
    void set_devinfo(const struct sdb_device_info &devinfo) override
    {
        RegisterController::set_devinfo(devinfo);
        pdec->set_devinfo(devinfo);
        pdec->get_data();
    }

    virtual void encode_params() override { }

    void write_general(const char *name, int32_t value)
    {
        pdec->write_general(name, value, read_dest);
    }
    void write_channel(const char *name, unsigned pos, int32_t value)
    {
        pdec->write_channel(name, pos, value, read_dest);
    }
};

#endif
