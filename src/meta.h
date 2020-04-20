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

    }

}