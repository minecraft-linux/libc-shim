#pragma once

#include "argrewrite.h"

namespace shim {

    namespace bionic {

        enum class clock_type : uint32_t {
            REALTIME = 0,
            MONOTONIC = 1
        };

        int to_host_clock_type(clock_type type);

        extern uintptr_t stack_chk_guard;

        void on_stack_chk_fail();

    }

    void assert(const char* file, int line, const char* msg);
    void assert2(const char* file, int line, const char* function, const char* msg);

    void add_common_shimmed_symbols(std::vector<shimmed_symbol> &list);

    void add_stdlib_shimmed_symbols(std::vector<shimmed_symbol> &list);

    void add_malloc_shimmed_symbols(std::vector<shimmed_symbol> &list);

    void add_ctype_shimmed_symbols(std::vector<shimmed_symbol> &list);

    void add_math_shimmed_symbols(std::vector<shimmed_symbol> &list);

    void add_sched_shimmed_symbols(std::vector<shimmed_symbol> &list);

}