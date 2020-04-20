#include <cerrno>
#include <stdexcept>
#include "pthreads.h"

using namespace shim;

int bionic::to_host_mutex_type(bionic::mutex_type type) {
    switch (type) {
        case mutex_type::NORMAL: return PTHREAD_MUTEX_NORMAL;
        case mutex_type::RECURSIVE: return PTHREAD_MUTEX_RECURSIVE;
        case mutex_type::ERRORCHECK: return PTHREAD_MUTEX_ERRORCHECK;
        default: throw std::runtime_error("Invalid mutex type");
    }
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

int shim::pthread_condattr_destroy(shim::pthread_condattr_t *attr) {
    return 0;
}

int shim::pthread_condattr_setclock(shim::pthread_condattr_t *attr, int clock) {
    if (clock != (int) bionic::clock_type::MONOTONIC &&
        clock != (int) bionic::clock_type::REALTIME)
        return EINVAL;
    attr->clock = (bionic::clock_type) clock;
    return 0;
}

int shim::pthread_condattr_getclock(const shim::pthread_condattr_t *attr, int *clock) {
    *clock = (int) attr->clock;
    return 0;
}

void shim::add_pthread_shimmed_symbols(std::vector<shimmed_symbol> &list) {
    list.insert(list.end(), {
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
    });
}