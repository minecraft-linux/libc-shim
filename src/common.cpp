#include <libc_shim.h>

#include <stdexcept>
#include "common.h"
#include "pthreads.h"

using namespace shim;

int bionic::to_host_clock_type(bionic::clock_type type) {
    switch (type) {
        case clock_type::REALTIME: return CLOCK_REALTIME;
        case clock_type::MONOTONIC: return CLOCK_MONOTONIC;
        default: throw std::runtime_error("Unexpected clock type");
    }
}

std::vector<shimmed_symbol> shim::get_shimmed_symbols() {
    std::vector<shimmed_symbol> ret;
    add_pthread_shimmed_symbols(ret);
    return ret;
}