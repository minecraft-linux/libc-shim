#pragma once

#include <cstdint>
#include <cstddef>
#include <sys/types.h>
#include "argrewrite.h"
#include <stdarg.h>

namespace shim {

    namespace bionic {

        enum class clock_type : uint32_t {
            REALTIME = 0,
            MONOTONIC = 1,
            BOOTTIME = 7
        };

        clockid_t to_host_clock_type(clock_type type);

        enum class mmap_flags : int {
            PRIVATE = 2,
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

        enum class prctl_num {
            SET_NAME = 15
        };


        extern uintptr_t stack_chk_guard;

        void on_stack_chk_fail();

        struct timeval {
            long tv_sec, tv_usec;
        };

#if defined(__LP64__)
        using off_t = ::off_t;
#else
        using off_t = int32_t;
#endif

        size_t strlcpy(char *dst, const char *src, size_t siz);

    }

    void assert_impl(const char* file, int line, const char* msg);
    void assert2_impl(const char* file, int line, const char* function, const char* msg);

    void android_set_abort_message(const char *msg);

    size_t strlen_chk(const char *str, size_t max_len);
    const char* strchr_chk(const char* __s, int __ch, size_t __n);

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

    int prctl(bionic::prctl_num opt, unsigned long a2, unsigned long a3, unsigned long a4, unsigned long a5);

    uint32_t arc4random();

    void *__memcpy_chk(void *dst, const void *src, size_t size, size_t max_len);

    void *__memmove_chk(void *dst, const void *src, size_t size, size_t max_len);

    void *__memset_chk(void *dst, int c, size_t size, size_t max_len);

    int __vsprintf_chk(char* dst, int flags, size_t dst_len_from_compiler, const char* format, va_list va);

    char* __strcpy_chk(char* dst, const char* src, size_t dst_len);

    char* __strncpy_chk(char* dst, const char* src, size_t len, size_t dst_len);

    char* __strncpy_chk2(char* dst, const char* src, size_t n, size_t dst_len, size_t src_len);

    size_t ctype_get_mb_cur_max();

    int gettimeofday(bionic::timeval *tv, void *p);

    ssize_t __read_chk(int fd, void* buf, size_t count, size_t buf_size);

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

    void add_prctl_shimmed_symbols(std::vector<shimmed_symbol> &list);

    void add_locale_shimmed_symbols(std::vector<shimmed_symbol> &list);

    void add_setjmp_shimmed_symbols(std::vector<shimmed_symbol> &list);

    void add_misc_shimmed_symbols(std::vector<shimmed_symbol> &list);

}