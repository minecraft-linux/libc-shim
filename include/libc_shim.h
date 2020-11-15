#pragma once

#include <vector>
#include <string>

namespace shim {

    struct shimmed_symbol {
        const char *name;
        void *value;

        shimmed_symbol(const char *name, void *value)
            : name(name), value(value) {}

        template <typename Ret, typename ...Args>
        shimmed_symbol(const char *name, Ret (*ptr)(Args...))
            : name(name), value((void*) ptr) {}

        template <typename Ret, typename ...Args>
        shimmed_symbol(const char *name, Ret (*ptr)(Args..., ...))
                : name(name), value((void*) ptr) {}
    };

    std::vector<shimmed_symbol> get_shimmed_symbols();

    // Rewrite access from from_android_data_dir ( "/data/data" ) to to_android_data_dir folder
    extern std::string from_android_data_dir;
    extern std::string to_android_data_dir;
}