/*
 * Copyright (C) 2022 CNPEM (cnpem.br)
 * Author: Érico Nogueira <erico.rolim@lnls.br>
 *
 * Released according to the GNU GPL, version 3 or any later version.
 */

#ifndef PRINTER_H
#define PRINTER_H

#include <functional>

#include <stdint.h>
#include <stdio.h>

enum class PrinterType {
    /* boolean values */
    boolean,
    progress,
    enable,
    /* directly print value */
    value,
    value_hex,
    value_2c, /* 2's complement */
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
    } boolean_names;

    printing_function custom_fn;

  public:
    Printer(const char *name, const char *description, PrinterType type):
        type(type), name(name), description(description)
    {
        switch (type) {
            case PrinterType::boolean:
                boolean_names.truth = "true";
                boolean_names.not_truth = "false";
                break;
            case PrinterType::progress:
                boolean_names.truth = "completed";
                boolean_names.not_truth = "in progress";
                break;
            case PrinterType::enable:
                boolean_names.truth = "enabled";
                boolean_names.not_truth = "disabled";
                break;
            default:
                break;
        }
    }

    Printer(const char *name, const char *description, PrinterType type, printing_function custom_fn):
        type(type), name(name), description(description), custom_fn(custom_fn)
    {
    }

    Printer(const char *name, const char *description, PrinterType type, const char *truth, const char *not_truth):
        type(type), name(name), description(description), boolean_names{truth, not_truth}
    {
    }

    void print(FILE *, bool, uint32_t) const;
};

#endif
