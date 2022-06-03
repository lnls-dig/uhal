/*
 * Copyright (C) 2022 CNPEM (cnpem.br)
 * Author: Érico Nogueira <erico.rolim@lnls.br>
 *
 * Released according to the GNU GPL, version 3 or any later version.
 */

#include <chrono>
#include <cstdlib>
#include <iostream>

#include <argparse/argparse.hpp>

#include "defer.h"
#include "pcie.h"
#include "pcie-open.h"

using namespace std::literals;

int main(int argc, char *argv[])
{
    argparse::ArgumentParser args("bench-cp", "1.0", argparse::default_arguments::help);
    args.add_argument("-b").help("device number").required();

    args.parse_args(argc, argv);

    auto device_number = args.get<std::string>("-b");
    struct pcie_bars bars = dev_open_slot(device_number.c_str());
    defer _(nullptr, [&bars](...){dev_close(bars);});

    const size_t s = 32UL << 20; // MB
    void *dst = malloc(s);
    if (!dst) return 1;

    auto ti = std::chrono::high_resolution_clock::now();
    bar2_read_v(&bars, 0, dst, s);
    auto tf = std::chrono::high_resolution_clock::now();
	 auto d = tf - ti;
    std::cout << d / 1ms << " ms" << std::endl;

    return 0;
}
