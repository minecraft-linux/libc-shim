#include "sched.h"

int shim::sched_setaffinity(pid_t pid, size_t cpusetsize, const cpu_set_t *mask) {
    int ret;
#ifdef __linux__
    ret = ::sched_setaffinity(pid,cpusetsize,mask);
#else
    ret = 0;
#endif
    return ret;
}
