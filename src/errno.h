#pragma once

#include <cstddef>

namespace shim {

    namespace bionic {

        int translate_errno_from_host(int err);

        int translate_errno_to_host(int err);

        int *errno_with_translation();

        void set_errno_with_translation(int err);

        void set_errno_without_translation(int err);

    }

    char *strerror(int err);

    int strerror_r(int err, char* buf, size_t len);

}