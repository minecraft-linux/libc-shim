#pragma once

#include <cstdint>
#include <pthread.h>
#include <libc_shim.h>
#include "meta.h"

namespace shim {

    namespace bionic {

        struct pthread_mutex_t {
            ::pthread_mutex_t *wrapped;
#if defined(__LP64__)
            int64_t priv[4];
#endif
        };

        enum class mutex_type : uint32_t {
            NORMAL = 0,
            RECURSIVE = 1,
            ERRORCHECK = 2,
            END = 2
        };

        static int to_host_mutex_type(mutex_type type);

        struct pthread_mutexattr_t {
            mutex_type type : 3;
        };

    }

    using pthread_mutex_t_resolver = detail::wrapper_type_resolver<::pthread_mutex_t, bionic::pthread_mutex_t>;
    using pthread_mutex_t = pthread_mutex_t_resolver::type;
    using pthread_mutexattr_t = bionic::pthread_mutexattr_t;

    struct host_mutexattr {

        ::pthread_mutexattr_t attr;

        host_mutexattr(bionic::pthread_mutexattr_t const &bionic_attr);

        ~host_mutexattr();

    };

    int pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr);
    int pthread_mutex_destroy(pthread_mutex_t *mutex);

    int pthread_mutexattr_init(pthread_mutexattr_t *attr);
    int pthread_mutexattr_destroy(pthread_mutexattr_t *attr);
    int pthread_mutexattr_gettype(const pthread_mutexattr_t *attr, int *type);
    int pthread_mutexattr_settype(pthread_mutexattr_t *attr, int type);


    void add_pthread_shimmed_symbols(std::vector<shimmed_symbol> &list);

}