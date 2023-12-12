/*
 * Copyright (C) 2022 CNPEM (cnpem.br)
 * Author: Érico Nogueira <erico.rolim@lnls.br>
 *
 * Released according to the GNU GPL, version 3 or any later version.
 */

#include <algorithm>
#include <stdexcept>

#include <tsl/ordered_map.h>

#include "pcie.h"
#include "printer.h"
#include "decoders.h"

struct RegisterDecoderPrivate {
    /** int32_t is so far a generic enough type to be used here, but int64_t
     * can be considered if it ever becomes an issue. We use double for
     * floating point values */
    using data_type = std::variant<std::int32_t, double>;
    /** Hold decoded data from normal registers */
    tsl::ordered_map<std::string_view, data_type> general_data;
    /** Hold decoded data from registers that are repeated for each channel */
    tsl::ordered_map<std::string_view, std::vector<data_type>> channel_data;
};

RegisterDecoder::RegisterDecoder(
    struct pcie_bars &bars,
    const struct sdb_device_info &ref_devinfo,
    std::unordered_map<std::string_view, Printer> printers):

    RegisterDecoderBase(bars, ref_devinfo),
    data(new RegisterDecoderPrivate()),
    printers(printers)
{
}
RegisterDecoder::~RegisterDecoder() = default;

bool RegisterDecoder::is_boolean_value(const char *name) const
{
    /* deal with values that don't have a printer defined for them */
    auto printer_it = printers.find(name);
    if (printer_it != printers.end()) {
        auto type = printer_it->second.get_type();
        return type == PrinterType::boolean || type == PrinterType::progress || type == PrinterType::enable;
    } else {
        return false;
    }
}

int32_t RegisterDecoder::try_boolean_value(const char *name, int32_t value) const
{
    return is_boolean_value(name) ? (bool)value : value;
}

template <class T>
void RegisterDecoder::add_general_internal(const char *name, T value)
{
    data->general_data[name] = value;
}

void RegisterDecoder::add_general(const char *name, int32_t value)
{
    value = try_boolean_value(name, value);
    add_general_internal(name, value);
}
void RegisterDecoder::add_general_double(const char *name, double value)
{
    add_general_internal(name, value);
}

template <class T>
void RegisterDecoder::add_channel_internal(const char *name, unsigned pos, T value)
{
    if (!number_of_channels)
        throw std::logic_error("number_of_channels must be set");
    data->channel_data[name].resize(*number_of_channels);

    /* using .at() to explicitly catch out of bounds access */
    data->channel_data[name].at(pos) = value;
}

void RegisterDecoder::add_channel(const char *name, unsigned pos, int32_t value)
{
    value = try_boolean_value(name, value);
    add_channel_internal(name, pos, value);
}
void RegisterDecoder::add_channel_double(const char *name, unsigned pos, double value)
{
    add_channel_internal(name, pos, value);
}

void RegisterDecoder::read_monitors()
{
    read();
}

void RegisterDecoder::decode_monitors()
{
    decode();
}

void RegisterDecoder::get_data(bool only_monitors)
{
    check_devinfo_is_set();

    if (only_monitors) {
        read_monitors();
        decode_monitors();
    } else {
        read();
        decode();
    }
}

void RegisterDecoder::binary_dump(FILE *f) const
{
    fwrite(read_dest, 1, read_size, f);
}

void RegisterDecoder::print(FILE *f, bool verbose) const
{
    unsigned indent = 0;

    auto print = [this, f, verbose, &indent](auto const &name, auto value) {
        try {
            if (auto vp = std::get_if<int32_t>(&value))
                printers.at(name).print(f, verbose, indent, *vp);
            else if (auto vp = std::get_if<double>(&value))
                printers.at(name).print(f, verbose, indent, *vp);
            else
                throw std::logic_error("unhandled data type from *_data");
        } catch (std::out_of_range &) {
            /* automatically skip values for which a printer isn't defined */
        }
    };

    for (const auto &[name, value]: data->general_data) {
        print(name, value);
    }

    if (number_of_channels) for (unsigned i = 0; i < *number_of_channels; i++) {
        if (channel && *channel != i) {
            continue;
        }
        fprintf(f, "channel %u:\n", i);
        indent = 4;

        for (const auto &[name, values]: data->channel_data) {
            print(name, values.at(i));
        }
    }
}

template <class T>
T RegisterDecoder::get_general_data(const char *name) const
{
    try {
        return std::get<T>(data->general_data.at(name));
    } catch (std::out_of_range &e) {
        fprintf(stderr, "%s: bad key '%s'\n", __func__, name);
        throw e;
    }
}
template int32_t RegisterDecoder::get_general_data(const char *) const;
template double RegisterDecoder::get_general_data(const char *) const;

template <class T>
T RegisterDecoder::get_channel_data(const char *name, unsigned channel_index) const
{
    try {
        return std::get<T>(data->channel_data.at(name).at(channel_index));
    } catch (std::out_of_range &e) {
        fprintf(stderr, "%s: bad key '%s'\n", __func__, name);
        throw e;
    }
}
template int32_t RegisterDecoder::get_channel_data(const char *, unsigned) const;
template double RegisterDecoder::get_channel_data(const char *, unsigned) const;
