#pragma once

#include <libc_shim.h>
#include "common.h"

namespace shim {

    namespace bionic {

        enum class ioctl_index : unsigned long {
            FILE_NBIO = 0x5421,

            SOCKET_CGIFCONF = 0x8912
        };

        enum class fcntl_index : int {
            SETFD = 2,
            SETFL = 4,
            SETLK = 6
        };

        struct flock {
            short l_type;
            short l_whence;
            off_t l_start;
            off_t l_len;
            int l_pid;
        };

    }

    int ioctl(int fd, bionic::ioctl_index cmd, void *arg);

    int fcntl(int fd, bionic::fcntl_index cmd, void *arg);

    void add_ioctl_shimmed_symbols(std::vector<shimmed_symbol> &list);

    void add_fcntl_shimmed_symbols(std::vector<shimmed_symbol> &list);

}