/*
 * Copyright (C) 2022 CNPEM (cnpem.br)
 * Author: Érico Nogueira <erico.rolim@lnls.br>
 *
 * Released according to the GNU GPL, version 3 or any later version.
 */

#include "decoders.h"

RegisterDecoder::RegisterDecoder(std::unordered_map<const char *, Printer> printers):
    printers(printers)
{
}

void RegisterDecoder::set_devinfo(const struct sdb_device_info &new_devinfo)
{
    devinfo = new_devinfo;
}

void RegisterDecoder::read(struct pcie_bars *bars)
{
    bar4_read_v(bars, devinfo.start_addr, read_dest, read_size);
}

void RegisterDecoder::print(FILE *f, bool verbose)
{
    decode();

    unsigned indent = 0;

    auto print = [this, f, verbose, &indent](const char *name, auto value) {
        printers.at(name).print(f, verbose, indent, value);
    };

    for (const auto name: general_data_order) {
        print(name, general_data.at(name));
    }

    if (number_of_channels) for (unsigned i = 0; i < *number_of_channels; i++) {
        if (channel && *channel != i) {
            i++;
            continue;
        }
        fprintf(f, "channel %u:\n", i++);
        indent = 4;

        for (const auto name: channel_data_order) {
            print(name, channel_data.at(name).at(i));
        }
    }
}

int32_t RegisterDecoder::get_general_data(const char *name)
{
    try {
        return general_data.at(name);
    } catch (std::out_of_range &e) {
        fprintf(stderr, "%s: bad key '%s'\n", __func__, name);
        throw e;
    }
}

int32_t RegisterDecoder::get_channel_data(const char *name, unsigned channel_index)
{
    try {
        return channel_data.at(name).at(channel_index);
    } catch (std::out_of_range &e) {
        fprintf(stderr, "%s: bad key '%s'\n", __func__, name);
        throw e;
    }
}
