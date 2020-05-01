#pragma once

#include <cstddef>

#ifdef __APPLE__
#define ERRNO_TRANSLATION
#endif

namespace shim {

    namespace bionic {

#ifdef ERRNO_TRANSLATION
        extern int errno_value;
#endif

        int translate_errno_from_host(int err);

        int translate_errno_to_host(int err);

        int *get_errno();

        void set_errno(int err);

#ifdef ERRNO_TRANSLATION
        void update_errno();
#else
        inline void update_errno() {}
#endif

    }

    char *strerror(int err);

    int strerror_r(int err, char* buf, size_t len);


    namespace detail {

        template <typename T>
        struct errno_update_helper;
        template <typename Ret, typename ...Args>
        struct errno_update_helper<Ret (Args...)> {
            template <Ret (*Ptr)(Args...)>
            static Ret wrapper(Args... args) {
                auto i = make_destroy_invoker ([] { bionic::update_errno(); });
                return Ptr(args...);
            }
        };
        template <typename Ret, typename ...Args>
        struct errno_update_helper<Ret (Args...) noexcept> : errno_update_helper<Ret (Args...)> {
        };

    }

}

#ifdef ERRNO_TRANSLATION
#define WithErrnoUpdate(ptr) (&shim::detail::errno_update_helper<typeof(ptr)>::wrapper<ptr>)
#else
#define WithErrnoUpdate(ptr) ptr
#endif