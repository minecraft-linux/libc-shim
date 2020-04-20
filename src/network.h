#pragma once

#include <libc_shim.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include "argrewrite.h"

namespace shim {

    namespace bionic {

        using socklen_t = int;

        enum class ai_flags : int {
            PASSIVE = 1,
            CANONNAME = 2,
            NUMERICHOST = 4
        };

        enum class af_family : int {
            UNSPEC = 0,
            INET = 2,
            INET6 = 10
        };

        enum class socktype : int {
            STREAM = 1,
            DGRAM = 2,
            RAW = 3
        };

        enum class ipproto : int {
            IP = 0,
            TCP = 6,
            UDP = 17
        };

        ai_flags from_host_ai_flags(int flags);

        int to_host_ai_flags(ai_flags flags);

        af_family from_host_af_family(int family);

        int to_host_af_family(af_family family);

        socktype from_host_socktype(int socktype);

        int to_host_socktype(socktype socktype);

        ipproto from_host_ipproto(int proto);

        int to_host_ipproto(ipproto proto);

        struct sockaddr {
            uint16_t family;
        };
        struct sockaddr_in : sockaddr {
            uint16_t port;
            uint32_t addr;
            char filler[8];
        };
        struct sockaddr_in6 : sockaddr {
            uint16_t port;
            uint32_t flow_info;
            uint8_t addr[16];
            uint32_t scope;
        };

        void from_host(const ::sockaddr *in, sockaddr *out);

        void to_host(const sockaddr *in, ::sockaddr *out);

        size_t get_host_len(const sockaddr *in);

        size_t get_bionic_len(const ::sockaddr *in);

        struct addrinfo {
            ai_flags ai_flags;
            af_family ai_family;
            socktype ai_socktype;
            ipproto ai_protocol;
            uint32_t ai_addrlen;
            char *ai_canonname;
            struct sockaddr *ai_addr;
            struct addrinfo *ai_next;
        };

        addrinfo *from_host_alloc(const ::addrinfo *in);

        void free_bionic_list(addrinfo *list);

        ::addrinfo *to_host_alloc(const addrinfo *in);

        void free_host_list(::addrinfo *list);

        /* nameinfo */

        enum class nameinfo_flags : int {
            NOFQDN = 1,
            NUMERICHOST = 2,
            NAMEREQD = 4,
            NUMERICSERV = 8,
            DGRAM = 16
        };

        int to_host_nameinfo_flags(nameinfo_flags flags);

    }

    int getaddrinfo(const char *node, const char *service, const bionic::addrinfo *hints, bionic::addrinfo **res);

    void freeaddrinfo(bionic::addrinfo *ai);

    int getnameinfo(const ::sockaddr *addr, socklen_t addrlen, char *host, socklen_t hostlen,
            char *serv, socklen_t servlen, bionic::nameinfo_flags flags);

    void add_network_shimmed_symbols(std::vector<shimmed_symbol> &list);

    namespace detail {

        template <>
        struct arg_rewrite<const ::sockaddr *> {
            using source = bionic::sockaddr *;

            sockaddr_storage stor;

            ::sockaddr *before(source src) {
                bionic::to_host(src, (::sockaddr *) &stor);
                return (::sockaddr *) &stor;
            }
            void after(source src) {
            }
        };

    }

}