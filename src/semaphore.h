#pragma once

#include <libc_shim.h>
#include "meta.h"
#include "argrewrite.h"
#ifndef __APPLE__
#include <semaphore.h>
#else
#include <mach/semaphore.h>
#endif

namespace shim {

#ifndef __APPLE__
    using host_sem_t = ::sem_t;
#else
    using host_sem_t = ::semaphore_t;
#endif

    namespace bionic {

        struct sem_t {
            host_sem_t *wrapped;
#if defined(__LP64__)
            int64_t priv[4];
#endif
        };

    }

    using sem_t_resolver = detail::wrapper_type_resolver<host_sem_t, bionic::sem_t>;
    using sem_t = sem_t_resolver::type;

    int sem_init(sem_t *sem, int pshared, unsigned int value);
    int sem_destroy(sem_t *sem);

#ifdef __APPLE__
    int sem_wait(sem_t *sem);
    int sem_timedwait(sem_t *sem, const struct timespec *abs_timeout);
    int sem_post(sem_t *sem);
#endif

    void add_sem_shimmed_symbols(std::vector<shimmed_symbol> &list);


    namespace detail {

        template <>
        struct arg_rewrite<sem_t *> : wrapper_resolved_const_ptr_rewriter<sem_t_resolver> {};

    }

}