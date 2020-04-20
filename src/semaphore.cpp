#include "semaphore.h"

#include <stdexcept>
#include "meta.h"

using namespace shim;

#ifndef __APPLE__

int shim::sem_init(shim::sem_t *sem, int pshared, unsigned int value) {
    return detail::make_c_wrapped<sem_t_resolver, int, unsigned int>(sem, ::sem_init, pshared, value);
}

int shim::sem_destroy(shim::sem_t *sem) {
    return detail::destroy_c_wrapped<sem_t_resolver>(sem, ::sem_destroy);
}

#endif

void shim::add_sem_shimmed_symbols(std::vector<shim::shimmed_symbol> &list) {
    list.insert(list.end(), {
        {"sem_init", sem_init},
        {"sem_destroy", sem_destroy},
        {"sem_wait", ArgRewritten(sem_wait)},
        {"sem_timedwait", ArgRewritten(sem_timedwait)},
        {"sem_post", ArgRewritten(sem_post)},
    });
}