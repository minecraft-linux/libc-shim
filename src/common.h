#pragma once

#include "argrewrite.h"

namespace shim {

    namespace bionic {

        enum class clock_type : uint32_t {
            REALTIME = 0,
            MONOTONIC = 1
        };

        int to_host_clock_type(clock_type type);

        enum class mmap_flags : int {
            FIXED = 0x10,
            ANON = 0x20,
            NORESERVE = 0x4000
        };

        int to_host_mmap_flags(mmap_flags flags);

        enum class rlimit_resource : int {
            NOFILE = 7
        };

        int to_host_rlimit_resource(rlimit_resource r);

        struct rlimit {
            unsigned long int rlim_cur, rlim_max;
        };


        extern uintptr_t stack_chk_guard;

        void on_stack_chk_fail();

#if defined(__LP64__)
        using off_t = ::off_t;
#else
        using off_t = int32_t;
#endif

        size_t strlcpy(char *dst, const char *src, size_t siz);

    }

    void assert(const char* file, int line, const char* msg);
    void assert2(const char* file, int line, const char* function, const char* msg);

    size_t strlen_chk(const char *str, size_t max_len);

#ifndef __LP64__
    /* Bionic uses a 32-bit off_t; this doesn't match up on Darwin so let's
     * overkill and apply it on all 32-bit platforms. */

    int ftruncate(int fd, bionic::off_t len);

    ssize_t pread(int fd, void *buf, size_t len, bionic::off_t off);

    ssize_t pwrite(int fd, const void *buf, size_t len, bionic::off_t off);
#endif

    void *memalign(size_t alignment, size_t size);

    void *mmap(void *addr, size_t length, int prot, bionic::mmap_flags flags, int fd, bionic::off_t offset);

    int getrusage(int who, void *usage);

    int getrlimit(bionic::rlimit_resource res, bionic::rlimit *info);

    int clock_gettime(bionic::clock_type clock, struct timespec *ts);

    void add_common_shimmed_symbols(std::vector<shimmed_symbol> &list);

    void add_stdlib_shimmed_symbols(std::vector<shimmed_symbol> &list);

    void add_malloc_shimmed_symbols(std::vector<shimmed_symbol> &list);

    void add_ctype_shimmed_symbols(std::vector<shimmed_symbol> &list);

    void add_math_shimmed_symbols(std::vector<shimmed_symbol> &list);

    void add_time_shimmed_symbols(std::vector<shimmed_symbol> &list);

    void add_sched_shimmed_symbols(std::vector<shimmed_symbol> &list);

    void add_unistd_shimmed_symbols(std::vector<shimmed_symbol> &list);

    void add_signal_shimmed_symbols(std::vector<shimmed_symbol> &list);

    void add_string_shimmed_symbols(std::vector<shimmed_symbol> &list);

    void add_wchar_shimmed_symbols(std::vector<shimmed_symbol> &list);

    void add_mman_shimmed_symbols(std::vector<shimmed_symbol> &list);

    void add_resource_shimmed_symbols(std::vector<shimmed_symbol> &list);

}