#pragma once

#include <libc_shim.h>

namespace shim {

    namespace bionic {

        enum class ioctl_index : unsigned long {
            FILE_NBIO = 0x5421,

            SOCKET_CGIFCONF = 0x8912
        };

    }

    int ioctl(int fd, bionic::ioctl_index cmd, void *arg);

    void add_ioctl_shimmed_symbols(std::vector<shimmed_symbol> &list);

}