#ifndef PRINTER_H
#define PRINTER_H

#include <cstdint>
#include <cstdio>
#include <stdexcept>
#include <functional>

enum class PrinterType {
    /* boolean values */
    boolean,
    progress,
    enable,
    /* directly print value */
    value, /* generic, so we don't need signed or unsigned versions */
    value_hex,
    value_float,
    /* allow it to print its own value */
    internal_custom_function,
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
    Printer(const char *name, const char *description, printing_function custom_fn);
    Printer(const char *name, const char *description, const char *not_truth, const char *truth);

    PrinterType get_type() const;

    template<typename T>
    void print(FILE *, bool, unsigned, T) const;
};

/* helper for defining std::unordered_map<std::string_view, Printer> */
#define PRINTER(name, ...) {name, {name, __VA_ARGS__}}

#endif
