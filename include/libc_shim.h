#pragma once

#include <vector>

namespace shim {

    struct shimmed_symbol {
        const char *name;
        void (*value)();

        template <typename Ret, typename ...Args>
        shimmed_symbol(const char *name, Ret (*ptr)(Args...))
            : name(name), value((void (*)()) ptr) {}
    };

    static std::vector<shimmed_symbol> get_shimmed_symbols();

}