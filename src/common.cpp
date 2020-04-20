#include <libc_shim.h>

#include "pthreads.h"

using namespace shim;

std::vector<shimmed_symbol> shim::get_shimmed_symbols() {
    std::vector<shimmed_symbol> ret;
    add_pthread_shimmed_symbols(ret);
    return ret;
}