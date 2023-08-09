/*
 * Copyright (C) 2022 CNPEM (cnpem.br)
 * Author: Érico Nogueira <erico.rolim@lnls.br>
 *
 * Released according to the GNU GPL, version 3 or any later version.
 */

#ifndef DECODERS_H
#define DECODERS_H

#include <cstdint>
#include <cstdio>
#include <optional>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>

#include <sys/types.h>

#include "sdb-defs.h"

#define LNLS_VENDORID 0x1000000000001215

class Printer;

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

    bool match_devinfo(const struct sdb_device_info &) const;

  protected:
    size_t read_size;
    void *read_dest;

    struct pcie_bars &bars;
    struct sdb_device_info devinfo;
    size_t addr;

    RegisterDecoderBase(struct pcie_bars &);
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

class RegisterDecoder: public RegisterDecoderBase {
    bool is_boolean_value(const char *) const;
    int32_t try_boolean_value(const char *, int32_t) const;

    /** int32_t is so far a generic enough type to be used here, but int64_t
     * can be considered if it ever becomes an issue. We use double for
     * floating point values */
    using data_type = std::variant<std::int32_t, double>;
    /** Hold decoded data from normal registers */
    std::unordered_map<std::string_view, data_type> general_data;
    /** Hold decoded data from registers that are repeated for each channel */
    std::unordered_map<std::string_view, std::vector<data_type>> channel_data;

    /** Hold the order in which data has been added to #general_data, which
     * allows us to implement printing prettily and in order while also using
     * an unordered_map */
    std::vector<const char *> general_data_order;
    /** Hold the order in which data has been added to #channel_data, like
     * #general_data_order */
    std::vector<const char *> channel_data_order;

    template <class T>
    void add_general_internal(const char *, T, bool);
    template <class T>
    void add_channel_internal(const char *, unsigned, T, bool);

  protected:
    /** Flag to indicate that the #general_data_order and #channel_data_order
     * vectors have been populated: it should only happen once, and is cheaper
     * than checking for membership of strings */
    bool data_order_done = false;

    /** A device that has multiple channels will set this */
    std::optional<unsigned> number_of_channels;

    std::unordered_map<std::string_view, Printer> printers;

    RegisterDecoder(struct pcie_bars &, const struct sdb_device_info &, std::unordered_map<std::string_view, Printer>);

    /** Save an int32_t (or smaller) value to a key */
    void add_general(const char *, int32_t, bool = false);
    /** Save a double value to a key */
    void add_general_double(const char *, double, bool = false);
    /** Save an int32_t (or smaller) value to a key and index.
     * #number_of_channels must be set before this can be called */
    void add_channel(const char *, unsigned, int32_t, bool = false);
    /** Save a double value to a key and index. #number_of_channels must be set
     * before this can be called */
    void add_channel_double(const char *, unsigned, double, bool = false);

    /** This simply calls read(), but can be specified by subclasses to read
     * only changing values from BAR4 into #read_dest */
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
    T get_general_data(const char *) const;
    template <class T>
    T get_channel_data(const char *, unsigned) const;

    std::optional<unsigned> channel;
};

#endif
