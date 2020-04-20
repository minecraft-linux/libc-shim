#pragma once

#include <cstdint>
#include <pthread.h>
#include <libc_shim.h>
#include "meta.h"
#include "argrewrite.h"
#include "common.h"

namespace shim {

    namespace bionic {

        struct pthread_mutex_t {
            ::pthread_mutex_t *wrapped;
#if defined(__LP64__)
            int64_t priv[4];
#endif
        };

        struct pthread_cond_t {
            ::pthread_cond_t *wrapped;
#if defined(__LP64__)
            int64_t priv[5];
#endif
        };

        struct pthread_rwlock_t {
            ::pthread_rwlock_t *wrapped;
#if defined(__LP64__)
            int64_t priv[6];
#endif
        };

        template <>
        auto to_host<pthread_mutex_t>(pthread_mutex_t const *m) { return m->wrapped; }
        template <>
        auto to_host<pthread_cond_t>(pthread_cond_t const *m) { return m->wrapped; }
        template <>
        auto to_host<pthread_rwlock_t>(pthread_rwlock_t const *m) { return m->wrapped; }

        enum class mutex_type : uint32_t {
            NORMAL = 0,
            RECURSIVE = 1,
            ERRORCHECK = 2,
            END = 2
        };

        static int to_host_mutex_type(mutex_type type);

        struct pthread_mutexattr_t {
            mutex_type type : 4;
        };

        struct pthread_condattr_t {
            bool shared : 1;
            clock_type clock : 2;
        };

        enum class sched_policy {
            OTHER = 0
        };

        static int to_host_sched_policy(sched_policy type);
        static sched_policy from_host_sched_policy(int type);

        struct pthread_attr_t {
            bool detached : 1;
            bool user_stack : 1;
            uint32_t filler : 32-2;
            void* stack_base;
            size_t stack_size;
            size_t guard_size;
            sched_policy sched_policy;
            int sched_priority;
#if defined(__LP64__)
            int64_t priv[2];
#endif
        };

        struct sched_param {
            int sched_priority;
        };

        struct pthread_cleanup_t {
            pthread_cleanup_t *prev;
            void (*routine)(void *);
            void *arg;
        };

        using pthread_key_t = uint32_t;
        using pthread_once_t = int;

        struct pthread_cleanup_holder {
            pthread_cleanup_t *current = nullptr;

            ~pthread_cleanup_holder();
        };

        extern thread_local pthread_cleanup_holder cleanup;

    }

    using pthread_attr_t = bionic::pthread_attr_t;

    using pthread_mutex_t_resolver = detail::wrapper_type_resolver<::pthread_mutex_t, bionic::pthread_mutex_t>;
    using pthread_mutex_t = pthread_mutex_t_resolver::type;
    using pthread_mutexattr_t = bionic::pthread_mutexattr_t;

    using pthread_cond_t_resolver = detail::wrapper_type_resolver<::pthread_cond_t, bionic::pthread_cond_t>;
    using pthread_cond_t = pthread_cond_t_resolver::type;
    using pthread_condattr_t = bionic::pthread_condattr_t;

    using pthread_rwlock_t_resolver = detail::wrapper_type_resolver<::pthread_rwlock_t, bionic::pthread_rwlock_t>;
    using pthread_rwlock_t = pthread_rwlock_t_resolver::type;

    using pthread_key_t = bionic::pthread_key_t;
    using pthread_once_t = bionic::pthread_once_t;

    struct host_pthread_attr {

        ::pthread_attr_t attr;

        host_pthread_attr(bionic::pthread_attr_t const *bionic_attr);

        ~host_pthread_attr();

    };

    struct host_mutexattr {

        ::pthread_mutexattr_t attr;

        host_mutexattr(bionic::pthread_mutexattr_t const *bionic_attr);

        ~host_mutexattr();

    };

    struct host_condattr {

        ::pthread_condattr_t attr;

        host_condattr(bionic::pthread_condattr_t const *bionic_attr);

        ~host_condattr();

    };

    int pthread_create(pthread_t *thread, pthread_attr_t const *attr, void* (*fn)(void *), void *arg);
    int pthread_setschedparam(pthread_t thread, bionic::sched_policy policy, const bionic::sched_param *param);
    int pthread_getschedparam(pthread_t thread, bionic::sched_policy *policy, bionic::sched_param *param);

    int pthread_attr_init(pthread_attr_t *attr);
    int pthread_attr_destroy(pthread_attr_t *attr);
    int pthread_attr_setdetachstate(pthread_attr_t *attr, int value);
    int pthread_attr_getdetachstate(pthread_attr_t *attr, int *value);
    int pthread_attr_setschedparam(pthread_attr_t *attr, const bionic::sched_param *value);
    int pthread_attr_getschedparam(pthread_attr_t *attr, bionic::sched_param *value);
    int pthread_attr_setstacksize(pthread_attr_t *attr, size_t value);
    int pthread_attr_getstacksize(pthread_attr_t *attr, size_t *value);

    int pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr);
    int pthread_mutex_destroy(pthread_mutex_t *mutex);

    int pthread_mutexattr_init(pthread_mutexattr_t *attr);
    int pthread_mutexattr_destroy(pthread_mutexattr_t *attr);
    int pthread_mutexattr_settype(pthread_mutexattr_t *attr, int type);
    int pthread_mutexattr_gettype(const pthread_mutexattr_t *attr, int *type);

    int pthread_cond_init(pthread_cond_t *cond, const pthread_condattr_t *attr);
    int pthread_cond_destroy(pthread_cond_t* cond);

    int pthread_condattr_init(pthread_condattr_t *attr);
    int pthread_condattr_destroy(pthread_condattr_t *attr);
    int pthread_condattr_setclock(pthread_condattr_t *attr, int clock);
    int pthread_condattr_getclock(const pthread_condattr_t *attr, int *clock);

    int pthread_rwlock_init(pthread_rwlock_t *rwlock, const pthread_rwlockattr_t *attr);
    int pthread_rwlock_destroy(pthread_rwlock_t* rwlock);

    int pthread_key_create(pthread_key_t *key, void (*destructor)(void*));
    int pthread_key_delete(pthread_key_t key);
    int pthread_setspecific(pthread_key_t key, const void *value);
    void *pthread_getspecific(pthread_key_t key);

    void pthread_cleanup_push_impl(bionic::pthread_cleanup_t *c, void (*cb)(void *), void *arg);
    void pthread_cleanup_pop_impl(bionic::pthread_cleanup_t *c, int execute);

    int pthread_once(pthread_once_t *control, void (*routine)());

    void add_pthread_shimmed_symbols(std::vector<shimmed_symbol> &list);

    namespace detail {

        template <>
        struct arg_rewrite<::pthread_mutex_t const *> : wrapper_resolved_const_ptr_rewriter<pthread_mutex_t_resolver> {};

        template <>
        struct arg_rewrite<::pthread_cond_t const *> : wrapper_resolved_const_ptr_rewriter<pthread_cond_t_resolver> {};

        template <>
        struct arg_rewrite<::pthread_rwlock_t const *> : wrapper_resolved_const_ptr_rewriter<pthread_rwlock_t_resolver> {};

    }

}