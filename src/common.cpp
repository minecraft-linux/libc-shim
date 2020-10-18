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
#if __MAC_OS_X_VERSION_MIN_REQUIRED < 101200
// for macOS 10.10 - 10.11
#include <mach/clock.h>
#include <mach/mach.h>
#endif
#endif
#include <inttypes.h>

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
        (uint32_t) mmap_flags::NORESERVE | (uint32_t) mmap_flags::PRIVATE)) != 0)
        throw std::runtime_error("Used unsupported mmap flags");

    int ret = 0;
    if ((uint32_t) flags & (uint32_t) mmap_flags::PRIVATE)
        ret |= MAP_PRIVATE;
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

const char* shim::strchr_chk(const char* __s, int __ch, size_t __n) {
    return strchr(__s, __ch);
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
#if defined(__APPLE__) && __MAC_OS_X_VERSION_MIN_REQUIRED < 101200
    if(::clock_gettime != NULL) {
#endif
    return ::clock_gettime(bionic::to_host_clock_type(clock), ts);
#if defined(__APPLE__) && __MAC_OS_X_VERSION_MIN_REQUIRED < 101200
    } else {
        // fallback if weak symbol is nullptr < macOS 10.12
        clock_serv_t cclock;
        mach_timespec_t mts;
        if (host_get_clock_service(mach_host_self(), clock == bionic::clock_type::MONOTONIC ? SYSTEM_CLOCK : CALENDAR_CLOCK, &cclock) != KERN_SUCCESS) {
            return -1;
        }
        kern_return_t r = clock_get_time(cclock, &mts);
        mach_port_deallocate(mach_task_self(), cclock);
        if (r != KERN_SUCCESS) {
            return -1;
        }
        ts->tv_sec = mts.tv_sec;
        ts->tv_nsec = mts.tv_nsec;
        return 0;
    }
#endif
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

void* shim::__memmove_chk(void *dst, const void *src, size_t size, size_t max_len) {
    if (size > max_len) {
        fprintf(stderr, "detected copy past buffer size");
        abort();
    }
    return ::memmove(dst, src, size);
}

void* shim::__memset_chk(void *dst, int c, size_t size, size_t max_len) {
    if (size > max_len) {
        fprintf(stderr, "detected set past buffer size");
        abort();
    }
    return ::memset(dst, c, size);
}

int shim::__vsprintf_chk(char* dst, int flags, size_t dst_len_from_compiler, const char* format, va_list va) {
    return vsprintf(dst, format, va);
}

char* shim::__strcpy_chk(char* dst, const char* src, size_t dst_len) {
  return strcpy(dst, src);
}

char* shim::__strncpy_chk(char* dst, const char* src, size_t len, size_t dst_len) {
  return strncpy(dst, src, len);
}

char* shim::__strncpy_chk2(char* dst, const char* src, size_t n, size_t dst_len, size_t src_len) {
    return strncpy(dst, src, n);
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

ssize_t shim::__read_chk(int fd, void *buf, size_t count, size_t buf_size) {
    return read(fd, buf, count);
}


void shim::add_common_shimmed_symbols(std::vector<shim::shimmed_symbol> &list) {
    list.insert(list.end(), {
        {"__errno", bionic::get_errno},
        {"__set_errno", bionic::set_errno},

        {"__stack_chk_fail", &bionic::on_stack_chk_fail},
        {"__stack_chk_guard", &bionic::stack_chk_guard},

        {"__assert", assert_impl},
        {"__assert2", assert2_impl},

        {"android_set_abort_message", android_set_abort_message},

        {"__cxa_atexit", ::__cxa_atexit},
        {"__cxa_finalize", ::__cxa_finalize},

        {"setpriority", setpriority}
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
        {"putenv", WithErrnoUpdate(putenv)},
        {"setenv", WithErrnoUpdate(setenv)},
        {"unsetenv", WithErrnoUpdate(unsetenv)},


        {"random", random},
        {"srandom", srandom},
        {"initstate", initstate},
        {"setstate", WithErrnoUpdate(setstate)},

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
        {"strtod", WithErrnoUpdate(strtod)},
        {"strtof", WithErrnoUpdate(strtof)},
        {"strtold", WithErrnoUpdate(strtold)},
        {"strtol", WithErrnoUpdate(strtol)},
        {"strtoul", WithErrnoUpdate(strtoul)},
        {"strtoul_l", WithErrnoUpdate(strtoul_l)},
        {"strtoq", WithErrnoUpdate(strtoq)},
        {"strtouq", WithErrnoUpdate(strtouq)},
        {"strtoll", WithErrnoUpdate(strtoll)},
        {"strtoll_l", WithErrnoUpdate(strtoll_l)},
        {"strtoull", WithErrnoUpdate(strtoull)},
        {"strtoull_l", WithErrnoUpdate(strtoull_l)},
        {"strtof_l", WithErrnoUpdate(strtof_l)},
        {"strtold_l", WithErrnoUpdate(strtold_l)},
        {"strtoumax", WithErrnoUpdate(strtoumax)},
        {"strtoimax", WithErrnoUpdate(strtoimax)},

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
        {"access", WithErrnoUpdate(::access)},
        {"lseek", WithErrnoUpdate(::lseek)},
        {"close", WithErrnoUpdate(::close)},
        {"read", WithErrnoUpdate(::read)},
        {"__read_chk", __read_chk},
        {"write", WithErrnoUpdate(::write)},
        {"pipe", WithErrnoUpdate(::pipe)},
        {"alarm", WithErrnoUpdate(::alarm)},
        {"sleep", WithErrnoUpdate(::sleep)},
        {"usleep", WithErrnoUpdate(::usleep)},
        {"pause", WithErrnoUpdate(::pause)},
        {"chown", WithErrnoUpdate(::chown)},
        {"fchown", WithErrnoUpdate(::fchown)},
        {"lchown", WithErrnoUpdate(::lchown)},
        {"chdir", WithErrnoUpdate(::chdir)},
        {"fchdir", WithErrnoUpdate(::fchdir)},
        {"getcwd", WithErrnoUpdate(::getcwd)},
        {"dup", WithErrnoUpdate(::dup)},
        {"dup2", WithErrnoUpdate(::dup2)},
        {"execv", WithErrnoUpdate(::execv)},
        {"execle", ::execle},
        {"execl", ::execl},
        {"execvp", ::execvp},
        {"execlp", ::execlp},
        {"nice", WithErrnoUpdate(::nice)},
        {"_exit", ::_exit},
        {"getuid", WithErrnoUpdate(::getuid)},
        {"getpid", WithErrnoUpdate(::getpid)},
        {"getgid", WithErrnoUpdate(::getgid)},
        {"getppid", WithErrnoUpdate(::getppid)},
        {"getpgrp", WithErrnoUpdate(::getpgrp)},
        {"geteuid", WithErrnoUpdate(::geteuid)},
        {"getegid", WithErrnoUpdate(::getegid)},
        {"fork", WithErrnoUpdate(::fork)},
        {"vfork", WithErrnoUpdate(::vfork)},
        {"isatty", WithErrnoUpdate(::isatty)},
        {"link", WithErrnoUpdate(::link)},
        {"symlink", WithErrnoUpdate(::symlink)},
        {"readlink", WithErrnoUpdate(::readlink)},
        {"unlink", WithErrnoUpdate(::unlink)},
        {"rmdir", WithErrnoUpdate(::rmdir)},
        {"gethostname", WithErrnoUpdate(::gethostname)},
        {"fsync", WithErrnoUpdate(::fsync)},
        {"sync", WithErrnoUpdate(::sync)},
        {"getpagesize", ::getpagesize},
        {"getdtablesize", ::getdtablesize},
        {"syscall", ::syscall},
        {"lockf", WithErrnoUpdate(::lockf)},
        {"swab", ::swab},

        /* Use our impl or fallback to system */
        {"ftruncate", WithErrnoUpdate(ftruncate)},
        {"pread", WithErrnoUpdate(pread)},
        {"pwrite", WithErrnoUpdate(pwrite)},
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
        {"__memmove_chk", __memmove_chk},
        {"memset", ::memset},
        {"__memset_chk", ::__memset_chk},
        {"memmem", ::memmem},
        {"__vsprintf_chk", __vsprintf_chk},
        {"strchr", (char *(*)(char *, int)) ::strchr},
        {"strrchr", (char *(*)(char *, int)) ::strrchr},
        {"strlen", ::strlen},
        {"__strlen_chk", strlen_chk},
        {"__strchr_chk", strchr_chk},
        {"strcmp", ::strcmp},
        {"strcpy", ::strcpy},
        {"__strcpy_chk", __strcpy_chk},
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
        {"__strncpy_chk", __strncpy_chk},
        {"__strncpy_chk2", __strncpy_chk2},
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
        {"mmap", WithErrnoUpdate(mmap)},
        {"munmap", WithErrnoUpdate(::munmap)},
        {"mprotect", WithErrnoUpdate(::mprotect)},
        {"madvise", WithErrnoUpdate(::madvise)},
        {"msync", WithErrnoUpdate(::msync)},
        {"mlock", WithErrnoUpdate(::mlock)},
        {"munlock", WithErrnoUpdate(::munlock)},
    });
}

void shim::add_resource_shimmed_symbols(std::vector<shim::shimmed_symbol> &list) {
    list.insert(list.end(), {
        /* sys/resource.h */
        {"getrusage", WithErrnoUpdate(getrusage)},
        {"getrlimit", WithErrnoUpdate(getrlimit)}
    });
}

void shim::add_prctl_shimmed_symbols(std::vector<shim::shimmed_symbol> &list) {
    list.push_back({"prctl", WithErrnoUpdate(prctl)});
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