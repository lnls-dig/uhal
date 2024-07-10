#ifndef SPI_H
#define SPI_H

#include <memory>

#include "controllers.h"
#include "decoders.h"

namespace spi {

struct spi;

struct Channel {
    unsigned channel;
    Channel(unsigned channel):
        channel(channel)
    {
        if (channel > 8)
            throw std::logic_error("bad spi slave index");
    }
};

class Core: public RegisterDecoder {
    std::unique_ptr<struct spi> regs_storage;
    struct spi &regs;

    void decode() override;

  public:
    Core(struct pcie_bars &);
    ~Core() override;
};

/** Even though the decoder supports the bidirectional registers, this
 * implementation does not support them whatsoever. */
class Controller: public RegisterDecoderController {
    std::unique_ptr<struct spi> regs_storage;
    struct spi &regs;

    Core dec;

    void set_devinfo_callback() override;
    static int32_t get_divider(int32_t, int32_t);

  public:
    Controller(struct pcie_bars &);
    ~Controller();

    void set_defaults();

    bool write_read_data(const unsigned char *, size_t, unsigned char *, size_t, Channel = {0});
};

} /* namespace spi */

#endif
