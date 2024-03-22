#include <charconv>
#include <string>

#include "printer.h"

Printer::Printer(const char *name, const char *description, PrinterType type):
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
        case PrinterType::custom_function:
            throw std::invalid_argument("this constructor shouldn't be used for PrinterType::custom_function");
            break;
        default:
            break;
    }
}

Printer::Printer(const char *name, const char *description, PrinterType type, printing_function custom_fn):
    type(type), name(name), description(description), custom_fn(custom_fn)
{
    if (type != PrinterType::custom_function) {
        throw std::invalid_argument("this constructor should only be used for PrinterType::custom_function");
    }
}

Printer::Printer(const char *name, const char *description, PrinterType type, const char *truth, const char *not_truth):
    type(type), name(name), description(description), boolean_names{truth, not_truth}
{
    if (type != PrinterType::boolean) {
        throw std::invalid_argument("this constructor should only be used for PrinterType::boolean");
    }
}

PrinterType Printer::get_type() const
{
    return type;
}

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
            fprintf(f, "%s: 0x%08X\n", final_name, (unsigned)value);
            break;
        case PrinterType::value_float:
            fprintf(f, "%s: %lf\n", final_name, (double)value);
            break;
        case PrinterType::custom_function:
            fprintf(f, "%s: ", final_name);
            custom_fn(f, verbose, value);
            break;
    }
}
template void Printer::print(FILE *, bool, unsigned, int32_t) const;
template void Printer::print(FILE *, bool, unsigned, double) const;
