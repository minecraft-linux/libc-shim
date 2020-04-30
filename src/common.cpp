#include <libc_shim.h>

#include <log.h>
#include <stdexcept>
#include "common.h"
#include "pthreads.h"
#include "semaphore.h"
#include "network.h"
#include "dirent.h"
#include "cstdio.h"
#include "errno.h"
#include "ctype_data.h"
#include "stat.h"
#include "file_misc.h"
#include "sysconf.h"
#include <cmath>
#include <unistd.h>
#include <sys/time.h>
#include <cwctype>
#include <csignal>
#include <cstring>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/utsname.h>
#include <csetjmp>
#include <clocale>
#include <cerrno>
#include <utime.h>
#include <sys/uio.h>
#include <syslog.h>
#ifndef __APPLE__
#include <sys/prctl.h>
#include <sys/auxv.h>
#include <sys/utsname.h>
#endif
#ifdef __APPLE__
#include <xlocale.h>
#endif

using namespace shim;

#ifdef __i386__
extern "C" unsigned long __umoddi3(unsigned long a, unsigned long b);
extern "C" unsigned long __udivdi3(unsigned long a, unsigned long b);
extern "C" long __divdi3(long a, long b);
#endif

extern "C" int __cxa_atexit(void (*)(void*), void*, void*);
extern "C" void __cxa_finalize(void * d);

#ifdef USE_BIONIC_SETJMP
extern "C" void bionic_setjmp();
extern "C" void bionic_longjmp();
#endif

uintptr_t bionic::stack_chk_guard = []() {
#ifndef __APPLE__
    return *((uintptr_t*) getauxval(AT_RANDOM));
#else
    return 0;
#endif
}();

clockid_t bionic::to_host_clock_type(bionic::clock_type type) {
    switch (type) {
        case clock_type::REALTIME: return CLOCK_REALTIME;
        case clock_type::MONOTONIC: return CLOCK_MONOTONIC;
#ifdef __APPLE__
        case clock_type::BOOTTIME: return CLOCK_MONOTONIC;
#else
        case clock_type::BOOTTIME: return CLOCK_BOOTTIME;
#endif
        default: throw std::runtime_error("Unexpected clock type");
    }
}

int bionic::to_host_mmap_flags(bionic::mmap_flags flags) {
    if (((uint32_t) flags & ~((uint32_t) mmap_flags::FIXED | (uint32_t) mmap_flags::ANON |
        (uint32_t) mmap_flags::NORESERVE)) != 0)
        throw std::runtime_error("Used unsupported mmap flags");

    int ret = 0;
    if ((uint32_t) flags & (uint32_t) mmap_flags::FIXED)
        ret |= MAP_FILE;
    if ((uint32_t) flags & (uint32_t) mmap_flags::ANON)
        ret |= MAP_ANONYMOUS;
    if ((uint32_t) flags & (uint32_t) mmap_flags::NORESERVE)
        ret |= MAP_NORESERVE;
    return ret;
}

int bionic::to_host_rlimit_resource(shim::bionic::rlimit_resource r) {
    switch (r) {
        case rlimit_resource::NOFILE: return RLIMIT_NOFILE;
        default: throw std::runtime_error("Unknown rlimit resource");
    }
}

void bionic::on_stack_chk_fail() {
    fprintf(stderr, "stack corruption has been detected\n");
    abort();
}

void shim::assert_impl(const char *file, int line, const char *msg) {
    fprintf(stderr, "assert failed: %s:%i: %s\n", file, line, msg);
    abort();
}

void shim::assert2_impl(const char *file, int line, const char *function, const char *msg) {
    fprintf(stderr, "assert failed: %s:%i %s: %s\n", file, line, function, msg);
    abort();
}

void shim::android_set_abort_message(const char *msg) {
    fprintf(stderr, "abort message: %s\n", msg);
}

size_t shim::strlen_chk(const char *str, size_t max_len) {
    auto ret = strlen(str);
    if (ret >= max_len) {
        fprintf(stderr, "strlen_chk: string longer than expected\n");
        abort();
    }
    return ret;
}

#ifndef __LP64__
int shim::ftruncate(int fd, bionic::off_t len) {
    return ::ftruncate(fd, (::off_t) len);
}

ssize_t shim::pread(int fd, void *buf, size_t len, bionic::off_t off) {
    return ::pread(fd, buf, len, (::off_t) off);
}

ssize_t shim::pwrite(int fd, const void *buf, size_t len, bionic::off_t off) {
    return ::pwrite(fd, buf, len, (::off_t) off);
}
#endif

int shim::clock_gettime(bionic::clock_type clock, struct timespec *ts) {
    return ::clock_gettime(bionic::to_host_clock_type(clock), ts);
}

void* shim::memalign(size_t alignment, size_t size) {
    void* ret;
    if (posix_memalign(&ret, alignment, size) != 0)
        return nullptr;
    return ret;
}

void *shim::mmap(void *addr, size_t length, int prot, bionic::mmap_flags flags, int fd, bionic::off_t offset) {
    return ::mmap(addr, length, prot, bionic::to_host_mmap_flags(flags), fd, (::off_t) offset);
}

int shim::getrusage(int who, void *usage) {
    Log::warn("Shim/Common", "getrusage is unsupported");
    return -1;
}

int shim::getrlimit(bionic::rlimit_resource res, bionic::rlimit *info) {
    ::rlimit hinfo {};
    int ret = ::getrlimit(bionic::to_host_rlimit_resource(res), &hinfo);
    info->rlim_cur = hinfo.rlim_cur;
    info->rlim_max = hinfo.rlim_max;
    return ret;
}

int shim::prctl(bionic::prctl_num opt, unsigned long a2, unsigned long a3, unsigned long a4, unsigned long a5) {
#ifdef __linux__
    return ::prctl((int) opt, a2, a3, a4, a5);
#else
    switch (opt) {
        case bionic::prctl_num::SET_NAME:
            return pthread_setname_np((const char *) a2);
        default:
            Log::error("Shim/Common", "Unexpected prctl: %i", opt);
            return EINVAL;
    }
#endif
}

uint32_t shim::arc4random() {
    return 0; // TODO:
}

void* shim::__memcpy_chk(void *dst, const void *src, size_t size, size_t max_len) {
    if (size > max_len) {
        fprintf(stderr, "detected copy past buffer size");
        abort();
    }
    return ::memcpy(dst, src, size);
}

size_t shim::ctype_get_mb_cur_max() {
    return MB_CUR_MAX;
}

int shim::gettimeofday(bionic::timeval *tv, void *p) {
    if (p)
        throw std::runtime_error("gettimeofday adtimezone is not supported");
    timeval htv {};
    int ret = ::gettimeofday(&htv, nullptr);
    tv->tv_sec = htv.tv_sec;
    tv->tv_usec = htv.tv_usec;
    return ret;
}


void shim::add_common_shimmed_symbols(std::vector<shim::shimmed_symbol> &list) {
    list.insert(list.end(), {
#ifdef __APPLE__
        {"__errno", bionic::errno_with_translation},
        {"__set_errno", bionic::set_errno_with_translation},
#else
        {"__errno", __errno_location},
        {"__set_errno", bionic::set_errno_without_translation},
#endif

        {"__stack_chk_fail", &bionic::on_stack_chk_fail},
        {"__stack_chk_guard", &bionic::stack_chk_guard},

        {"__assert", assert_impl},
        {"__assert2", assert2_impl},

        {"android_set_abort_message", android_set_abort_message},

        {"__cxa_atexit", ::__cxa_atexit},
        {"__cxa_finalize", ::__cxa_finalize}
    });
}

void shim::add_stdlib_shimmed_symbols(std::vector<shim::shimmed_symbol> &list) {
    list.insert(list.end(), {
        {"abort", abort},
        {"atexit", atexit},
        {"exit", exit},
        {"_Exit", _Exit},

        {"system", system},

        {"getenv", getenv},
        {"putenv", putenv},
        {"setenv", setenv},
        {"unsetenv", unsetenv},


        {"random", random},
        {"srandom", srandom},
        {"initstate", initstate},
        {"setstate", setstate},

        {"rand", rand},
        {"srand", srand},
        {"rand_r", rand_r},
        {"drand48", drand48},
        {"erand48", erand48},
        {"lrand48", lrand48},
        {"nrand48", nrand48},
        {"mrand48", mrand48},
        {"jrand48", jrand48},
        {"srand48", srand48},
        {"seed48", seed48},
        {"lcong48", lcong48},

        {"atof", atof},
        {"atoi", atoi},
        {"atol", atol},
        {"atoll", atoll},
        {"strtod", strtod},
        {"strtof", strtof},
        {"strtold", strtold},
        {"strtol", strtol},
        {"strtoul", strtoul},
        {"strtoul_l", strtoul_l},
        {"strtoq", strtoq},
        {"strtouq", strtouq},
        {"strtoll", strtoll},
        {"strtoll_l", strtoll_l},
        {"strtoull", strtoull},
        {"strtoull_l", strtoull_l},
        {"strtof_l", strtof_l},
        {"strtold_l", strtold_l},

        {"realpath", realpath},
        {"bsearch", bsearch},
        {"qsort", qsort},
        {"mblen", mblen},
        {"mbtowc", mbtowc},
        {"wctomb", wctomb},
        {"mbstowcs", mbstowcs},
        {"wcstombs", wcstombs},
        {"wcsrtombs", wcsrtombs}
    });
}

void shim::add_malloc_shimmed_symbols(std::vector<shim::shimmed_symbol> &list) {
    list.insert(list.end(), {
        {"malloc", ::malloc},
        {"free", ::free},
        {"calloc", ::calloc},
        {"realloc", ::realloc},
        {"valloc", ::valloc},
        {"memalign", memalign},
        {"posix_memalign", ::posix_memalign},
        {"_Znwj", (void *(*)(size_t)) ::operator new},
        {"_ZdlPv", (void (*)(void *)) ::operator delete},
    });
}

void shim::add_ctype_shimmed_symbols(std::vector<shim::shimmed_symbol> &list) {
    list.insert(list.end(), {
        {"_tolower_tab_", &_tolower_tab_},
        {"_toupper_tab_", &_toupper_tab_},
        {"_ctype_", &_ctype_},
        {"isalnum", isalnum},
        {"isalpha", isalpha},
        {"isblank", isblank},
        {"iscntrl", iscntrl},
        {"isdigit", isdigit},
        {"isgraph", isgraph},
        {"islower", islower},
        {"isprint", isprint},
        {"ispunct", ispunct},
        {"isspace", isspace},
        {"isupper", isupper},
        {"isxdigit", isxdigit},

        {"tolower", ::tolower},
        {"toupper", ::toupper},

        {"__ctype_get_mb_cur_max", ctype_get_mb_cur_max}
    });
}

void shim::add_math_shimmed_symbols(std::vector<shim::shimmed_symbol> &list) {
    list.insert(list.end(), {
#ifdef __i386__
        {"__umoddi3", __umoddi3},
        {"__udivdi3", __udivdi3},
        {"__divdi3", __divdi3},
#endif
        {"ldexp", (double (*)(double, int)) ::ldexp},
    });
}

void shim::add_time_shimmed_symbols(std::vector<shim::shimmed_symbol> &list) {
    list.insert(list.end(), {
        /* sys/time.h */
        {"gettimeofday", gettimeofday},

        /* time.h */
        {"clock", ::clock},
        {"time", ::time},
        {"difftime", ::difftime},
        {"mktime", ::mktime},
        {"strftime", ::strftime},
        {"strptime", ::strptime},
        {"strftime_l", ::strftime_l},
        {"strptime_l", ::strptime_l},
        {"gmtime", ::gmtime},
        {"gmtime_r", ::gmtime_r},
        {"localtime", ::localtime},
        {"localtime_r", ::localtime_r},
        {"asctime", ::asctime},
        {"ctime", ::ctime},
        {"asctime_r", ::asctime_r},
        {"ctime_r", ::ctime_r},
        {"tzname", ::tzname},
        {"tzset", ::tzset},
        {"daylight", &::daylight},
        {"timezone", &::timezone},
        {"nanosleep", ::nanosleep},
        {"clock_gettime", clock_gettime},
    });
}
void shim::add_sched_shimmed_symbols(std::vector<shim::shimmed_symbol> &list) {
    list.insert(list.end(), {
        {"sched_yield", ::sched_yield},
    });
}

void shim::add_unistd_shimmed_symbols(std::vector<shim::shimmed_symbol> &list) {
    list.insert(list.end(), {
        {"access", ::access},
        {"lseek", ::lseek},
        {"close", ::close},
        {"read", ::read},
        {"write", ::write},
        {"pipe", ::pipe},
        {"alarm", ::alarm},
        {"sleep", ::sleep},
        {"usleep", ::usleep},
        {"pause", ::pause},
        {"chown", ::chown},
        {"fchown", ::fchown},
        {"lchown", ::lchown},
        {"chdir", ::chdir},
        {"fchdir", ::fchdir},
        {"getcwd", ::getcwd},
        {"dup", ::dup},
        {"dup2", ::dup2},
        {"execv", ::execv},
        {"execle", ::execle},
        {"execl", ::execl},
        {"execvp", ::execvp},
        {"execlp", ::execlp},
        {"nice", ::nice},
        {"_exit", ::_exit},
        {"getuid", ::getuid},
        {"getpid", ::getpid},
        {"getgid", ::getgid},
        {"getppid", ::getppid},
        {"getpgrp", ::getpgrp},
        {"geteuid", ::geteuid},
        {"getegid", ::getegid},
        {"fork", ::fork},
        {"vfork", ::vfork},
        {"isatty", ::isatty},
        {"link", ::link},
        {"symlink", ::symlink},
        {"readlink", ::readlink},
        {"unlink", ::unlink},
        {"rmdir", ::rmdir},
        {"gethostname", ::gethostname},
        {"fsync", ::fsync},
        {"sync", ::sync},
        {"getpagesize", ::getpagesize},
        {"getdtablesize", ::getdtablesize},
        {"syscall", ::syscall},
        {"lockf", ::lockf},
        {"swab", ::swab},

        /* Use our impl or fallback to system */
        {"ftruncate", ftruncate},
        {"pread", pread},
        {"pwrite", pwrite},
    });
}

void shim::add_signal_shimmed_symbols(std::vector<shim::shimmed_symbol> &list) {
    list.insert(list.end(), {
        {"signal", ::signal},
        {"bsd_signal", ::signal},
        {"kill", ::kill},
        {"killpg", ::killpg},
        {"raise", ::raise},
        {"sigaction", ::sigaction},
        {"sigprocmask", ::sigprocmask},
        {"sigemptyset", ::sigemptyset},
        {"sigaddset", ::sigaddset}
    });
}

void shim::add_string_shimmed_symbols(std::vector<shim::shimmed_symbol> &list) {
    list.insert(list.end(), {
        /* string.h */
        {"memccpy", ::memccpy},
        {"memchr", (void *(*)(void *, int, size_t)) ::memchr},
        {"memcmp", (int (*)(const void *, const void *, size_t)) ::memcmp},
        {"memcpy", ::memcpy},
        {"__memcpy_chk", __memcpy_chk},
        {"memmove", ::memmove},
        {"memset", ::memset},
        {"memmem", ::memmem},
        {"strchr", (char *(*)(char *, int)) ::strchr},
        {"strrchr", (char *(*)(char *, int)) ::strrchr},
        {"strlen", ::strlen},
        {"__strlen_chk", strlen_chk},
        {"strcmp", ::strcmp},
        {"strcpy", ::strcpy},
        {"strcat", ::strcat},
        {"strdup", ::strdup},
        {"strstr", (char *(*)(char *, const char *)) ::strstr},
        {"strtok", ::strtok},
        {"strtok_r", ::strtok_r},
        {"strerror", strerror},
        {"strerror_r", strerror_r},
        {"strnlen", ::strnlen},
        {"strncat", ::strncat},
        {"strndup", ::strndup},
        {"strncmp", ::strncmp},
        {"strncpy", ::strncpy},
        {"strlcpy", bionic::strlcpy},
        {"strcspn", ::strcspn},
        {"strpbrk", (char *(*)(char *, const char *)) ::strpbrk},
        {"strsep", ::strsep},
        {"strspn", ::strspn},
        {"strsignal", ::strsignal},
        {"strcoll", ::strcoll},
        {"strxfrm", ::strxfrm},

        /* strings.h */
        {"bcmp", ::bcmp},
        {"bcopy", ::bcopy},
        {"bzero", ::bzero},
        {"ffs", ::ffs},
        {"index", ::index},
        {"rindex", ::rindex},
        {"strcasecmp", ::strcasecmp},
        {"strncasecmp", ::strncasecmp},
    });
}

void shim::add_wchar_shimmed_symbols(std::vector<shim::shimmed_symbol> &list) {
    list.insert(list.end(), {
        /* wchar.h */
        {"wcslen", ::wcslen},
        {"wctob", ::wctob},
        {"btowc", ::btowc},
        {"wmemchr", (wchar_t *(*)(wchar_t *, wchar_t, size_t)) ::wmemchr},
        {"wmemcmp", ::wmemcmp},
        {"wmemcpy", ::wmemcpy},
        {"wmemset", ::wmemset},
        {"wmemmove", ::wmemmove},
        {"wcrtomb", ::wcrtomb},
        {"mbrtowc", ::mbrtowc},
        {"wcscoll", ::wcscoll},
        {"wcsxfrm", ::wcsxfrm},
        {"wcsftime", ::wcsftime},
        {"mbsrtowcs", ::mbsrtowcs},
        {"mbsnrtowcs", ::mbsnrtowcs},
        {"wcsnrtombs", ::wcsnrtombs},
        {"mbrlen", mbrlen},
        {"wcstol", wcstol},
        {"wcstoul", wcstoul},
        {"wcstoll", wcstoll},
        {"wcstoull", wcstoull},
        {"wcstof", wcstof},
        {"wcstod", wcstod},
        {"wcstold", wcstold},
        {"swprintf", swprintf},

        /* wctype.h */
        {"wctype", ::wctype},
        {"iswspace", ::iswspace},
        {"iswctype", ::iswctype},
        {"towlower", ::towlower},
        {"towupper", ::towupper},

        {"iswlower",  iswlower},
        {"iswprint",  iswprint},
        {"iswblank",  iswblank},
        {"iswcntrl",  iswcntrl},
        {"iswupper",  iswupper},
        {"iswalpha",  iswalpha},
        {"iswdigit",  iswdigit},
        {"iswpunct",  iswpunct},
        {"iswxdigit", iswxdigit}
    });
}

void shim::add_mman_shimmed_symbols(std::vector<shim::shimmed_symbol> &list) {
    list.insert(list.end(), {
        /* sys/mman.h */
        {"mmap", mmap},
        {"munmap", ::munmap},
        {"mprotect", ::mprotect},
        {"madvise", ::madvise},
        {"msync", ::msync},
        {"mlock", ::mlock},
        {"munlock", ::munlock},
    });
}

void shim::add_resource_shimmed_symbols(std::vector<shim::shimmed_symbol> &list) {
    list.insert(list.end(), {
        /* sys/resource.h */
        {"getrusage", getrusage},
        {"getrlimit", getrlimit}
    });
}

void shim::add_prctl_shimmed_symbols(std::vector<shim::shimmed_symbol> &list) {
    list.push_back({"prctl", prctl});
}

void shim::add_locale_shimmed_symbols(std::vector<shim::shimmed_symbol> &list) {
    list.insert(list.end(), {
        {"newlocale", newlocale},
        {"uselocale", uselocale},
        {"freelocale", freelocale},
        {"setlocale", setlocale},
        {"localeconv", localeconv}
    });
}

void shim::add_setjmp_shimmed_symbols(std::vector<shim::shimmed_symbol> &list) {
    list.insert(list.end(), {
#ifdef USE_BIONIC_SETJMP
        {"setjmp", bionic_setjmp},
        {"longjmp", bionic_longjmp},
#else
        {"setjmp", _setjmp},
        {"longjmp", longjmp},
#endif
    });
}

void shim::add_misc_shimmed_symbols(std::vector<shim::shimmed_symbol> &list) {
    list.insert(list.end(), {
        {"uname", uname}, // TODO: This may be wrong?

        {"utime", utime},

        {"writev", writev},

        {"openlog", openlog},
        {"closelog", closelog},
        {"syslog", syslog},

        {"arc4random", arc4random},
    });
}

std::vector<shimmed_symbol> shim::get_shimmed_symbols() {
    std::vector<shimmed_symbol> ret;
    add_common_shimmed_symbols(ret);
    add_stdlib_shimmed_symbols(ret);
    add_malloc_shimmed_symbols(ret);
    add_ctype_shimmed_symbols(ret);
    add_math_shimmed_symbols(ret);
    add_time_shimmed_symbols(ret);
    add_sched_shimmed_symbols(ret);
    add_unistd_shimmed_symbols(ret);
    add_signal_shimmed_symbols(ret);
    add_string_shimmed_symbols(ret);
    add_wchar_shimmed_symbols(ret);
    add_pthread_shimmed_symbols(ret);
    add_sem_shimmed_symbols(ret);
    add_network_shimmed_symbols(ret);
    add_dirent_shimmed_symbols(ret);
    add_stat_shimmed_symbols(ret);
    add_cstdio_shimmed_symbols(ret);
    add_mman_shimmed_symbols(ret);
    add_resource_shimmed_symbols(ret);
    add_prctl_shimmed_symbols(ret);
    add_locale_shimmed_symbols(ret);
    add_setjmp_shimmed_symbols(ret);
    add_ioctl_shimmed_symbols(ret);
    add_fcntl_shimmed_symbols(ret);
    add_poll_select_shimmed_symbols(ret);
    add_epoll_shimmed_symbols(ret);
    add_misc_shimmed_symbols(ret);
    add_sysconf_shimmed_symbols(ret);
    add_eventfd_shimmed_symbols(ret);
    return ret;
}