#pragma once

namespace shim {

    namespace detail {

        template <bool Cond, typename IfTrue, typename IfFalse>
        struct type_resolver;

        template <typename IfTrue, typename IfFalse>
        struct type_resolver<true, IfTrue, IfFalse> {
            using type = IfTrue;
        };

        template <typename IfTrue, typename IfFalse>
        struct type_resolver<false, IfTrue, IfFalse> {
            using type = IfFalse;
        };

        template <typename Host, typename Wrapper>
        struct wrapper_type_resolver {
            static constexpr bool is_wrapped = sizeof(Host) > sizeof(Wrapper);
            using host_type = Host;
            using wrapper_type = Wrapper;
            using type = typename type_resolver<is_wrapped, Wrapper, Host>::type;
        };



        template <typename Resolver, typename... Args>
        int make_c_wrapped(typename Resolver::type *object, int (*constructor)(typename Resolver::host_type *, Args...), Args... args) {
            if constexpr (Resolver::is_wrapped) {
                object->wrapped = new typename Resolver::host_type;
                int ret = constructor(object->wrapped, args...);
                if (ret)
                    delete object->wrapped;
                return ret;
            } else {
                return constructor(object, args...);
            }
        }

        template <typename Resolver>
        int destroy_c_wrapped(typename Resolver::type *object, int (*destructor)(typename Resolver::host_type *)) {
            if constexpr (Resolver::is_wrapped) {
                int ret = destructor(object->wrapped);
                free(object->wrapped);
                return ret;
            } else {
                return destructor(object);
            }
        }

    }

}