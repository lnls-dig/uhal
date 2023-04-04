/*
 * Copyright (C) 2022 CNPEM (cnpem.br)
 * Author: Érico Nogueira <erico.rolim@lnls.br>
 *
 * Released according to the GNU GPL, version 3 or any later version.
 */

#include <stdexcept>

#include "decoders.h"

RegisterDecoder::RegisterDecoder(struct pcie_bars &bars, std::unordered_map<std::string_view, Printer> printers):
    bars(bars),
    printers(printers)
{
}
RegisterDecoder::~RegisterDecoder() = default;

bool RegisterDecoder::is_boolean_value(const char *name)
{
    /* deal with values that don't have a printer defined for them */
    try {
        auto type = printers.at(name).get_type();
        return type == PrinterType::boolean || type == PrinterType::progress || type == PrinterType::enable;
    } catch (std::out_of_range &e) {
        return false;
    }
}

int32_t RegisterDecoder::try_boolean_value(const char *name, int32_t value)
{
    return is_boolean_value(name) ? (bool)value : value;
}

template <class T>
void RegisterDecoder::add_general_internal(const char *name, T value, bool skip)
{
    general_data[name] = value;
    if (!data_order_done && !skip)
        general_data_order.push_back(name);
}

void RegisterDecoder::add_general(const char *name, int32_t value, bool skip)
{
    value = try_boolean_value(name, value);
    add_general_internal(name, value, skip);
}
void RegisterDecoder::add_general_double(const char *name, double value, bool skip)
{
    add_general_internal(name, value, skip);
}

template <class T>
void RegisterDecoder::add_channel_internal(const char *name, unsigned pos, T value, bool skip)
{
    /* using .at() to explicitly catch out of bounds access */
    channel_data[name].at(pos) = value;
    if (!data_order_done && !skip)
        channel_data_order.push_back(name);
}

void RegisterDecoder::add_channel_impl(const char *name, unsigned pos, int32_t value, bool skip)
{
    value = try_boolean_value(name, value);
    add_channel_internal(name, pos, value, skip);
}
void RegisterDecoder::add_channel_impl_double(const char *name, unsigned pos, double value, bool skip)
{
    add_channel_internal(name, pos, value, skip);
}

void RegisterDecoder::set_devinfo(const struct sdb_device_info &new_devinfo)
{
    devinfo = new_devinfo;
}

void RegisterDecoder::read()
{
    bar4_read_v(&bars, devinfo.start_addr, read_dest, read_size);

    decode();
}

void RegisterDecoder::print(FILE *f, bool verbose)
{
    unsigned indent = 0;

    auto print = [this, f, verbose, &indent](const char *name, auto value) {
        if (auto vp = std::get_if<int32_t>(&value))
            printers.at(name).print(f, verbose, indent, *vp);
        else if (auto vp = std::get_if<double>(&value))
            printers.at(name).print(f, verbose, indent, *vp);
        else
            throw std::logic_error("unhandled data type from *_data");
    };

    for (const auto name: general_data_order) {
        print(name, general_data.at(name));
    }

    if (number_of_channels) for (unsigned i = 0; i < *number_of_channels; i++) {
        if (channel && *channel != i) {
            continue;
        }
        fprintf(f, "channel %u:\n", i);
        indent = 4;

        for (const auto name: channel_data_order) {
            print(name, channel_data.at(name).at(i));
        }
    }
}

template <class T>
T RegisterDecoder::get_general_data(const char *name)
{
    try {
        return std::get<T>(general_data.at(name));
    } catch (std::out_of_range &e) {
        fprintf(stderr, "%s: bad key '%s'\n", __func__, name);
        throw e;
    }
}
template int32_t RegisterDecoder::get_general_data(const char *);
template double RegisterDecoder::get_general_data(const char *);

template <class T>
T RegisterDecoder::get_channel_data(const char *name, unsigned channel_index)
{
    try {
        return std::get<T>(channel_data.at(name).at(channel_index));
    } catch (std::out_of_range &e) {
        fprintf(stderr, "%s: bad key '%s'\n", __func__, name);
        throw e;
    }
}
template int32_t RegisterDecoder::get_channel_data(const char *, unsigned);
template double RegisterDecoder::get_channel_data(const char *, unsigned);
