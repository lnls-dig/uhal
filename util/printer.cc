/*
 * Copyright (C) 2022 CNPEM (cnpem.br)
 * Author: Érico Nogueira <erico.rolim@lnls.br>
 *
 * Released according to the GNU GPL, version 3 or any later version.
 */

#include <charconv>
#include <string>

#include "printer.h"

template<typename T>
void Printer::print(FILE *f, bool verbose, unsigned indent, T value) const
{
    while (indent--) fputc(' ', f);

    std::string s;
    const char *final_name;
    if (verbose) {
        s = name;
        s += " (";
        s += description;
        s += ")";
        final_name = s.c_str();
    } else {
        final_name = name;
    }

    switch (type) {
        case PrinterType::boolean:
        case PrinterType::progress:
        case PrinterType::enable:
            fprintf(f, "%s: %s\n", final_name, value ? boolean_names.truth : boolean_names.not_truth);
            break;
        case PrinterType::value:
            {
                char tmp[32];
                auto r = std::to_chars(tmp, tmp + sizeof tmp, value);
                *r.ptr = '\0';
                fprintf(f, "%s: %s\n", final_name, tmp);
            }
            break;
        case PrinterType::value_hex:
            fprintf(f, "%s: %08X\n", final_name, (unsigned)value);
            break;
        case PrinterType::custom_function:
            fprintf(f, "%s: ", final_name);
            custom_fn(f, verbose, value);
            break;
    }
}
template void Printer::print(FILE *, bool, unsigned, uint64_t) const; /* masks are specified as xxxUL */
template void Printer::print(FILE *, bool, unsigned, uint32_t) const; /* most values fall under this category */
template void Printer::print(FILE *, bool, unsigned, uint16_t) const;
template void Printer::print(FILE *, bool, unsigned, int32_t) const;
template void Printer::print(FILE *, bool, unsigned, int16_t) const;

void print_reg_impl(FILE *f, bool v, unsigned &indent, const char *reg, unsigned offset)
{
    if (v) {
        fprintf(f, "%s (0x%02X)\n", reg, offset);
        indent = 4;
    }
}
