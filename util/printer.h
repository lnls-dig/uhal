/*
 * Copyright (C) 2022 CNPEM (cnpem.br)
 * Author: Érico Nogueira <erico.rolim@lnls.br>
 *
 * Released according to the GNU GPL, version 3 or any later version.
 */

#ifndef PRINTER_H
#define PRINTER_H

#include <stdexcept>
#include <functional>

#include <stdint.h>
#include <stdio.h>

enum class PrinterType {
    /* boolean values */
    boolean,
    progress,
    enable,
    /* directly print value */
    value, /* generic, so we don't need signed or unsigned versions */
    value_hex,
    /* allow it to print its own value */
    custom_function,
};

typedef std::function<void(FILE *, bool, uint32_t)> printing_function;

class Printer {
    PrinterType type;
    const char *name, *description;

    struct {
        const char *truth;
        const char *not_truth;
    } boolean_names{};

    printing_function custom_fn;

  public:
    Printer(const char *name, const char *description, PrinterType type);
    Printer(const char *name, const char *description, PrinterType type, printing_function custom_fn);
    Printer(const char *name, const char *description, PrinterType type, const char *truth, const char *not_truth);

    template<typename T>
    void print(FILE *, bool, unsigned, T) const;
};

/* helper for defining std::unordered_map<std::string_view, Printer> */
#define I(name, ...) {name, {name, __VA_ARGS__}}

void print_reg_impl(FILE *f, bool v, unsigned &indent, const char *reg, unsigned offset);

#endif
