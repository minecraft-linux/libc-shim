#include <cerrno>
#include <stdexcept>
#include <limits.h>
#include "pthreads.h"

using namespace shim;

thread_local bionic::pthread_cleanup_holder bionic::cleanup;

bionic::pthread_cleanup_holder::~pthread_cleanup_holder() {
    auto p = current;
    while (p) {
        p->routine(p->arg);
        p = p->prev;
    }
}

int bionic::to_host_mutex_type(bionic::mutex_type type) {
    switch (type) {
        case mutex_type::NORMAL: return PTHREAD_MUTEX_NORMAL;
        case mutex_type::RECURSIVE: return PTHREAD_MUTEX_RECURSIVE;
        case mutex_type::ERRORCHECK: return PTHREAD_MUTEX_ERRORCHECK;
        default: throw std::runtime_error("Invalid mutex type");
    }
}

host_pthread_attr::host_pthread_attr(shim::pthread_attr_t const *bionic_attr) {
    ::pthread_attr_init(&attr);
    if (bionic_attr) {
        ::pthread_attr_setdetachstate(&attr, bionic_attr->detached ? PTHREAD_CREATE_DETACHED : PTHREAD_CREATE_JOINABLE);
        if (bionic_attr->stack_size)
            ::pthread_attr_setstacksize(&attr, bionic_attr->stack_size);
        if (bionic_attr->sched_priority) {
            sched_param param;
            ::pthread_attr_getschedparam(&attr, &param);
            param.sched_priority = bionic_attr->sched_priority;
            ::pthread_attr_setschedparam(&attr, &param);
        }
    }
}

host_pthread_attr::~host_pthread_attr() {
    ::pthread_attr_destroy(&attr);
}

host_mutexattr::host_mutexattr(pthread_mutexattr_t const *bionic_attr) {
    ::pthread_mutexattr_init(&attr);
    if (bionic_attr)
        ::pthread_mutexattr_settype(&attr, bionic::to_host_mutex_type(bionic_attr->type));
}

host_mutexattr::~host_mutexattr() {
    ::pthread_mutexattr_destroy(&attr);
}

host_condattr::host_condattr(pthread_condattr_t const *bionic_attr) {
    ::pthread_condattr_init(&attr);
    if (bionic_attr)
        ::pthread_condattr_setclock(&attr, bionic::to_host_clock_type(bionic_attr->clock));
}

host_condattr::~host_condattr() {
    ::pthread_condattr_destroy(&attr);
}

template <typename Resolver, typename... Args>
int make_wrapped(typename Resolver::type *object, int (*constructor)(typename Resolver::host_type *, Args...), Args... args) {
    if constexpr (Resolver::is_wrapped) {
        object->wrapped = new typename Resolver::host_type;
        int ret = constructor(object->wrapped, args...);
        if (!ret)
            delete object->wrapped;
        return ret;
    } else {
        return constructor(&object, args...);
    }
}

template <typename Resolver>
int destroy_wrapped(typename Resolver::type *object, int (*destructor)(typename Resolver::host_type *)) {
    if constexpr (Resolver::is_wrapped) {
        int ret = destructor(object->wrapped);
        free(object->wrapped);
        return ret;
    } else {
        return destructor(&object);
    }
}

static_assert(sizeof(pthread_t) <= sizeof(long), "pthread_t larger than bionic's not supported");

int shim::pthread_create(pthread_t *thread, const shim::pthread_attr_t *attr, void *(*fn)(void *), void *arg) {
    host_pthread_attr hattr (attr);
    return pthread_create(thread, &hattr.attr, fn, arg);
}

int shim::pthread_attr_init(pthread_attr_t *attr) {
    *attr = pthread_attr_t{false, false, 0, nullptr, 0, 0, 0, 0};
    return 0;
}

int shim::pthread_attr_destroy(shim::pthread_attr_t *attr) {
    return 0;
}

int shim::pthread_attr_setdetachstate(shim::pthread_attr_t *attr, int value) {
    if (value != 0 && value != 1)
        return EINVAL;
    attr->detached = value;
    return 0;
}

int shim::pthread_attr_getdetachstate(shim::pthread_attr_t *attr, int *value) {
    *value = attr->detached;
    return 0;
}

int shim::pthread_attr_setschedparam(shim::pthread_attr_t *attr, const shim::bionic::sched_param *value) {
    attr->sched_priority = value->sched_priority;
    return 0;
}

int shim::pthread_attr_getschedparam(shim::pthread_attr_t *attr, shim::bionic::sched_param *value) {
    value->sched_priority = attr->sched_priority;
    return 0;
}

int shim::pthread_attr_setstacksize(shim::pthread_attr_t *attr, size_t value) {
    if (value < PTHREAD_STACK_MIN)
        return 0;
    attr->stack_size = value;
    return 0;
}

int shim::pthread_attr_getstacksize(shim::pthread_attr_t *attr, size_t *value) {
    *value = attr->stack_size;
    return 0;
}

int shim::pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr) {
    host_mutexattr hattr (attr);
    return make_wrapped<pthread_mutex_t_resolver, const ::pthread_mutexattr_t *>(mutex, &::pthread_mutex_init, &hattr.attr);
}

int shim::pthread_mutex_destroy(pthread_mutex_t *mutex) {
    return destroy_wrapped<pthread_mutex_t_resolver>(mutex, &::pthread_mutex_destroy);
}

int shim::pthread_mutexattr_init(pthread_mutexattr_t *attr) {
    *attr = pthread_mutexattr_t{bionic::mutex_type::NORMAL};
    return 0;
}

int shim::pthread_mutexattr_destroy(pthread_mutexattr_t *attr) {
    return 0;
}

int shim::pthread_mutexattr_settype(pthread_mutexattr_t *attr, int type) {
    if (type < 0 || type > (int) bionic::mutex_type::END)
        return EINVAL;
    attr->type = (bionic::mutex_type) type;
    return 0;
}

int shim::pthread_mutexattr_gettype(const pthread_mutexattr_t *attr, int *type) {
    *type = (int) attr->type;
    return 0;
}

int shim::pthread_cond_init(pthread_cond_t *cond, const shim::pthread_condattr_t *attr) {
    host_condattr hattr (attr);
    return make_wrapped<pthread_cond_t_resolver, const ::pthread_condattr_t *>(cond, &::pthread_cond_init, &hattr.attr);
}

int shim::pthread_cond_destroy(pthread_cond_t *cond) {
    return destroy_wrapped<pthread_cond_t_resolver>(cond, &::pthread_cond_destroy);
}

int shim::pthread_condattr_init(pthread_condattr_t *attr) {
    *attr = {false, bionic::clock_type::MONOTONIC};
    return 0;
}

int shim::pthread_condattr_destroy(pthread_condattr_t *attr) {
    return 0;
}

int shim::pthread_condattr_setclock(pthread_condattr_t *attr, int clock) {
    if (clock != (int) bionic::clock_type::MONOTONIC &&
        clock != (int) bionic::clock_type::REALTIME)
        return EINVAL;
    attr->clock = (bionic::clock_type) clock;
    return 0;
}

int shim::pthread_condattr_getclock(const pthread_condattr_t *attr, int *clock) {
    *clock = (int) attr->clock;
    return 0;
}

int shim::pthread_rwlock_init(pthread_rwlock_t *rwlock, const pthread_rwlockattr_t *attr) {
    if (attr != nullptr)
        throw std::runtime_error("non-NULL rwlock attr is currently not supported");
    return make_wrapped<pthread_rwlock_t_resolver, const ::pthread_rwlockattr_t *>(rwlock, &::pthread_rwlock_init, nullptr);
}

int shim::pthread_rwlock_destroy(pthread_rwlock_t *rwlock) {
    return destroy_wrapped<pthread_rwlock_t_resolver>(rwlock, &::pthread_rwlock_destroy);
}

void shim::pthread_cleanup_push_impl(shim::bionic::pthread_cleanup_t *c, void (*cb)(void *), void *arg) {
    c->prev = bionic::cleanup.current;
    c->routine = cb;
    c->arg = arg;
    bionic::cleanup.current = c;
}

void shim::pthread_cleanup_pop_impl(shim::bionic::pthread_cleanup_t *c, int execute) {
    bionic::cleanup.current = c->prev;
    if (execute)
        c->routine(c->arg);
}

void shim::add_pthread_shimmed_symbols(std::vector<shimmed_symbol> &list) {
    list.insert(list.end(), {
        {"pthread_create", pthread_create},
        {"pthread_attr_init", pthread_attr_init},
        {"pthread_attr_destroy", pthread_attr_destroy},
        {"pthread_attr_setdetachstate", pthread_attr_setdetachstate},
        {"pthread_attr_getdetachstate", pthread_attr_getdetachstate},
        {"pthread_attr_setschedparam", pthread_attr_setschedparam},
        {"pthread_attr_getschedparam", pthread_attr_getschedparam},
        {"pthread_attr_setstacksize", pthread_attr_setstacksize},
        {"pthread_attr_getstacksize", pthread_attr_getstacksize},

        {"pthread_mutex_init", pthread_mutex_init},
        {"pthread_mutex_destroy", pthread_mutex_destroy},
        {"pthread_mutex_lock", ArgRewritten(::pthread_mutex_lock)},
        {"pthread_mutex_unlock", ArgRewritten(::pthread_mutex_unlock)},
        {"pthread_mutex_trylock", ArgRewritten(::pthread_mutex_trylock)},
        {"pthread_mutexattr_init", pthread_mutexattr_init},
        {"pthread_mutexattr_destroy", pthread_mutexattr_destroy},
        {"pthread_mutexattr_settype", pthread_mutexattr_settype},
        {"pthread_mutexattr_gettype", pthread_mutexattr_gettype},

        {"pthread_cond_init", pthread_cond_init},
        {"pthread_cond_destroy", pthread_cond_destroy},
        {"pthread_cond_wait", ArgRewritten(::pthread_cond_wait)},
        {"pthread_cond_broadcast", ArgRewritten(::pthread_cond_broadcast)},
        {"pthread_cond_timedwait", ArgRewritten(::pthread_cond_timedwait)},
        {"pthread_condattr_init", pthread_condattr_init},
        {"pthread_condattr_destroy", pthread_condattr_destroy},
        {"pthread_condattr_setclock", pthread_condattr_setclock},
        {"pthread_condattr_getclock", pthread_condattr_getclock},

        {"pthread_rwlock_init", pthread_rwlock_init},
        {"pthread_rwlock_destroy", pthread_rwlock_destroy},
        {"pthread_rwlock_rdlock", ArgRewritten(::pthread_rwlock_rdlock)},
        {"pthread_rwlock_wrlock", ArgRewritten(::pthread_rwlock_wrlock)},
        {"pthread_rwlock_unlock", ArgRewritten(::pthread_rwlock_unlock)},

        {"__pthread_cleanup_push", pthread_cleanup_push_impl},
        {"__pthread_cleanup_pop", pthread_cleanup_pop_impl},
    });
}