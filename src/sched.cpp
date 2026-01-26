#include "sched.h"

int shim::sched_setaffinity(pid_t pid, size_t cpusetsize, const void* mask) {
    int ret;
#if defined(__linux__) && defined(__GLIBC__) && !(__GLIBC__ < 2 || __GLIBC__ == 2 && __GLIBC_MINOR__ < 42)
    ret = ::sched_setaffinity(pid, cpusetsize, (cpu_set_t*)mask);
#else
    ret = 0;
#endif
    return ret;
}

int shim::sched_getaffinity(pid_t pid, size_t cpusetsize, const void* mask) {
    int ret;
#ifdef __linux__
    ret = ::sched_getaffinity(pid, cpusetsize, (cpu_set_t*)mask);
#else
    ret = 0;
#endif
    return ret;
}
