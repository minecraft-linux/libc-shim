#pragma once

#include "argrewrite.h"

namespace shim {

    namespace bionic {

        int translate_errno_from_host(int err);

        int translate_errno_to_host(int err);

        int *errno_with_translation();

        void set_errno_with_translation(int err);

        void set_errno_without_translation(int err);

    }

}