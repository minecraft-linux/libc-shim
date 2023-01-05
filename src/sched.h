#pragma once

#include <libc_shim.h>
#include <sched.h>

typedef struct prop_info prop_info;

namespace shim {
    int sched_setaffinity(pid_t pid, size_t set_size, const cpu_set_t* set);

}
