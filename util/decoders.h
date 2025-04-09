#ifndef DECODERS_H
#define DECODERS_H

#include <cstdint>
#include <cstdio>
#include <memory>
#include <optional>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

#include <sys/types.h>

#include "sdb-defs.h"

#define LNLS_VENDORID 0x1000000000001215

struct RegisterDecoderPrivate;
class Printer;

#define CONSTRUCTOR_REGS(type) regs_storage(new type ()), regs(*regs_storage)

namespace decoders {
    /** int32_t is so far a generic enough type to be used here, but int64_t
     * can be considered if it ever becomes an issue. We use double for
     * floating point values */
    using data_type = std::variant<std::int32_t, double>;

    using data_key = std::pair<std::string_view, std::optional<unsigned>>;
}

/** This class defines base methods that will be used by both decoders and
 * controllers. */
class RegisterDecoderBase {
    /** Is set to true when set_devinfo() is called, used to protect us from
     * using uninitialized device information */
    bool devinfo_is_set = false;

    uint64_t vendor_id;
    uint32_t device_id;
    uint8_t major_version;
    /** Is set to true when we use the
     * RegisterDecoderBase(struct pcie_bars &, const struct sdb_device_info &)
     * initializer */
    bool check_devinfo = false;

    [[nodiscard]]
    bool match_devinfo(const struct sdb_device_info &) const;

  protected:
    size_t read_size;
    void *read_dest;

    void set_read_dest(auto &dest)
    {
        read_dest = &dest;
        read_size = sizeof dest;
    }

    struct pcie_bars &bars;
    struct sdb_device_info devinfo;
    size_t addr;

    /** Initializer that sets the device information supported by the
     * implementation */
    RegisterDecoderBase(struct pcie_bars &, const struct sdb_device_info &);

    /** Read values from BAR4 into #read_dest */
    virtual void read();

  public:
    void check_devinfo_is_set() const;
    virtual void set_devinfo(const struct sdb_device_info &);

    const device_match_fn match_devinfo_lambda;
};

struct RegisterField {
    decoders::data_type value;
    size_t offset;
    uint32_t mask;
    unsigned fixed_point_pos = 0;
    bool multibit;
    bool is_signed;
    bool is_fixed_point = false;
};

/** This class defines a common interface to the FPGA cores on an AFC board.
This class fulfils two main roles:

- exposing register fields to a library user, who can address them by name (a
string) and index (a number). This is provided by the `get_generic_data()`
method;
- printing register fields in a common format. This is provided by the `print()`
method. */
class RegisterDecoder: public RegisterDecoderBase {
    bool is_boolean_value(const char *) const;
    int32_t try_boolean_value(const char *, int32_t) const;

    std::unique_ptr<RegisterDecoderPrivate> pvt;

    template <class T>
    void add_data_internal(const char *, decoders::data_key::second_type, T);

    void rf_add_data_internal(const char *, decoders::data_key::second_type, RegisterField);

    size_t register2offset(const uint32_t *);
    uint32_t *offset2register(size_t, void *);

    void write_internal(const char *, std::optional<unsigned>, decoders::data_type, void *);

  protected:
    /** A device that has multiple channels will set this to the maximum amount
     * of channels */
    std::optional<unsigned> number_of_channels;

    std::unordered_map<std::string_view, Printer> printers;

    RegisterDecoder(struct pcie_bars &, const struct sdb_device_info &, std::unordered_map<std::string_view, Printer>);

    /** Save an int32_t (or smaller) value to a key */
    void add_general(const char *, int32_t);
    /** Save a double value to a key */
    void add_general_double(const char *, double);
    /** Save an int32_t (or smaller) value to a key and index.
     * #number_of_channels must be set before this can be called */
    void add_channel(const char *, unsigned, int32_t);
    /** Save a double value to a key and index. #number_of_channels must be set
     * before this can be called */
    void add_channel_double(const char *, unsigned, double);

    /** get_bit() implementation which saves how to decode a register field. \p
     * reg must be within the memory region defined by
     * RegisterDecoderBase#read_dest and RegisterDecoderBase#read_size */
    RegisterField rf_get_bit(uint32_t &, uint32_t);
    /** equivalent to rf_get_bit() for extract_value() */
    RegisterField rf_extract_value(uint32_t &, uint32_t, bool=false);
    /** equivalent to rf_extract_value() with mask=UINT32_MAX, useful for when
     * values take up a whole register and a MASK macro isn't defined */
    RegisterField rf_whole_register(uint32_t &value, bool is_signed=false)
    {
        return rf_extract_value(value, UINT32_MAX, is_signed);
    }
    /** set RegisterField metadata for conversion to and from fixed point */
    static RegisterField rf_fixed2float(RegisterField, unsigned);

    /** add_general() that takes a RegisterField */
    void add_general(const char *name, RegisterField rf)
    {
        rf_add_data_internal(name, std::nullopt, rf);
    }
    /** add_channel() that takes a RegisterField */
    void add_channel(const char *name, unsigned pos, RegisterField rf)
    {
        rf_add_data_internal(name, pos, rf);
    }

    /** This simply calls read(), but can be specified by subclasses to read
     * only changing values from BAR4 into RegisterDecoderBase#read_dest */
    virtual void read_monitors();
    /** Decode registers into actual values. Implemented by subclasses */
    virtual void decode() = 0;
    /** This simply calls decode(), but can be specified by subclasses to
     * decode only changing values */
    virtual void decode_monitors();

  public:
    virtual ~RegisterDecoder();
    /** Read from device and decode registers. Choose between all values or
     * only monitors */
    void get_data(bool=false);
    void binary_dump(FILE *) const;
    virtual void print(FILE *, bool) const;

    template <class T>
    T get_general_data(const char *name) const
    {
        return std::get<T>(get_generic_data(name));
    }
    template <class T>
    T get_channel_data(const char *name, unsigned channel_index) const
    {
        return std::get<T>(get_generic_data(name, channel_index));
    }

    decoders::data_type get_generic_data(const char *, decoders::data_key::second_type=std::nullopt) const;

    std::optional<unsigned> channel;

    void write_general(const char *name, decoders::data_type value, void *dest)
    {
        write_internal(name, std::nullopt, value, dest);
    }
    void write_channel(const char *name, unsigned pos, decoders::data_type value, void *dest)
    {
        write_internal(name, pos, value, dest);
    }
};

#endif
