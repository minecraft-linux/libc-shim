#pragma once

#include "argrewrite.h"

namespace shim {

    namespace bionic {

        enum class clock_type : uint32_t {
            REALTIME = 0,
            MONOTONIC = 1
        };

        int to_host_clock_type(clock_type type);

    }

}