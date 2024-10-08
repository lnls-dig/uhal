#include <algorithm>
#include <stdexcept>

#include <tsl/ordered_map.h>

#include "pcie.h"
#include "printer.h"
#include "util.h"
#include "decoders.h"

struct pairhash {
    template <typename T, typename U>
    std::size_t operator()(const std::pair<T, U> &x) const
    {
        std::hash<T> hasher_t;
        std::hash<U> hasher_u;

        auto hash_t = hasher_t(x.first);
        auto hash_u = hasher_u(x.second);

        hash_u ^= hash_t + 0x9e3779b9 + (hash_u<<6) + (hash_u>>2);

        return hash_u;
    }
};

struct RegisterDecoderPrivate {
    /** Hold decoded data from all registers */
    tsl::ordered_map<decoders::data_key, decoders::data_type, pairhash> data;

    std::unordered_map<decoders::data_key, RegisterField, pairhash> register_fields;
};

RegisterDecoder::RegisterDecoder(
    struct pcie_bars &bars,
    const struct sdb_device_info &ref_devinfo,
    std::unordered_map<std::string_view, Printer> printers):

    RegisterDecoderBase(bars, ref_devinfo),
    pvt(new RegisterDecoderPrivate()),
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
void RegisterDecoder::add_data_internal(const char *name, decoders::data_key::second_type pos, T value)
{
    if (pos) {
        /* number_of_channels should always be set because it's needed in print() */
        if (!number_of_channels)
            throw std::logic_error("number_of_channels must be set");
        if (*pos >= *number_of_channels)
            throw std::out_of_range("pos can't be greater than number_of_channels");
    }

    if constexpr (std::is_integral_v<T>)
        value = try_boolean_value(name, value);

    pvt->data[{name, pos}] = value;
}

void RegisterDecoder::add_general(const char *name, int32_t value)
{
    add_data_internal(name, std::nullopt, value);
}
void RegisterDecoder::add_general_double(const char *name, double value)
{
    add_data_internal(name, std::nullopt, value);
}

void RegisterDecoder::add_channel(const char *name, unsigned pos, int32_t value)
{
    add_data_internal(name, pos, value);
}
void RegisterDecoder::add_channel_double(const char *name, unsigned pos, double value)
{
    add_data_internal(name, pos, value);
}

size_t RegisterDecoder::register2offset(uint32_t *reg)
{
    auto rdp = (uintptr_t)read_dest;
    auto rp = (uintptr_t)reg;

    assert(rp >= rdp && rp < rdp + read_size);

    return rp - rdp;
}
uint32_t *RegisterDecoder::offset2register(size_t offset, void *registers)
{
    assert(offset <= read_size);
    return (uint32_t *)((unsigned char *)registers + offset);
}

RegisterField RegisterDecoder::rf_get_bit(uint32_t &reg, uint32_t mask)
{
    return {
        .value = get_bit(reg, mask),
        .offset = register2offset(&reg),
        .mask = mask,
        .multibit = false,
        .is_signed = false,
    };
}

RegisterField RegisterDecoder::rf_extract_value(uint32_t &reg, uint32_t mask, bool is_signed)
{
    return {
        .value = extract_value(reg, mask, is_signed),
        .offset = register2offset(&reg),
        .mask = mask,
        .multibit = true,
        .is_signed = is_signed,
    };
}

RegisterField RegisterDecoder::rf_fixed2float(RegisterField rf, unsigned fixed_point_pos)
{
    rf.fixed_point_pos = fixed_point_pos;
    rf.is_signed = true;
    rf.is_fixed_point = true;
    rf.value = fixed2float(std::get<int32_t>(rf.value), fixed_point_pos);
    return rf;
}

void RegisterDecoder::rf_add_data_internal(const char *name, decoders::data_key::second_type pos, RegisterField rf)
{
    pvt->register_fields[{name, pos}] = rf;
    add_data_internal(name, pos, rf.value);
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

    for (const auto &[key, value]: pvt->data) {
        if (!key.second) /* position is nullopt */
            print(key.first, value);
    }

    if (number_of_channels) for (unsigned i = 0; i < *number_of_channels; i++) {
        if (channel && *channel != i) {
            continue;
        }
        fprintf(f, "channel %u:\n", i);
        indent = 4;

        for (const auto &[key, value]: pvt->data) {
            /* this loop isn't efficient, but printing isn't performance critical */
            if (key.second && *key.second == i)
                print(key.first, value);
        }
    }
}

decoders::data_type RegisterDecoder::get_generic_data(const char *name, decoders::data_key::second_type channel_index) const
{
    try {
        return pvt->data.at({name, channel_index});
    } catch (std::out_of_range &e) {
        fprintf(stderr, "%s: bad key '{%s,%u}'\n", __func__, name, channel_index ? *channel_index : -1);
        throw e;
    }
}

void RegisterDecoder::write_internal(const char *name, std::optional<unsigned> pos, decoders::data_type rvalue, void *dest)
{
    auto rf = pvt->register_fields.at({name, pos});
    uint32_t *reg = offset2register(rf.offset, dest);

    int32_t value = rf.is_fixed_point ?
        float2fixed(std::get<double>(rvalue), rf.fixed_point_pos) :
        std::get<int32_t>(rvalue);

    if (rf.multibit)
        if (rf.is_signed) clear_and_insert(*reg, value, rf.mask);
        else clear_and_insert(*reg, (uint32_t)value, rf.mask);
    else
        insert_bit(*reg, value, rf.mask);
}
