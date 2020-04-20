#include "network.h"

#include <stdexcept>
#include <netinet/in.h>
#include <cstring>

using namespace shim;

bionic::ai_flags bionic::from_host_ai_flags(int flags) {
    uint32_t ret = 0;
    if (flags & AI_PASSIVE)
        ret |= (uint32_t) ai_flags::PASSIVE;
    if (flags & AI_CANONNAME)
        ret |= (uint32_t) ai_flags::CANONNAME;
    if (flags & AI_NUMERICHOST)
        ret |= (uint32_t) ai_flags::NUMERICHOST;
    return (bionic::ai_flags) ret;
}

int bionic::to_host_ai_flags(bionic::ai_flags flags) {
    int ret = 0;
    if ((uint32_t) flags & (uint32_t) ai_flags::PASSIVE)
        ret |= AI_PASSIVE;
    if ((uint32_t) flags & (uint32_t) ai_flags::CANONNAME)
        ret |= AI_CANONNAME;
    if ((uint32_t) flags & (uint32_t) ai_flags::NUMERICHOST)
        ret |= AI_NUMERICHOST;
    return ret;
}

bionic::af_family bionic::from_host_af_family(int family) {
    switch (family) {
        case AF_UNSPEC: return af_family::UNSPEC;
        case AF_INET: return af_family::INET;
        case AF_INET6: return af_family::INET6;
        default: throw std::runtime_error("Unknown af_family");
    }
}

int bionic::to_host_af_family(bionic::af_family family) {
    switch (family) {
        case af_family::UNSPEC: return AF_UNSPEC;
        case af_family::INET: return AF_INET;
        case af_family::INET6: return AF_INET6;
        default: throw std::runtime_error("Unknown af_family");
    }
}

bionic::socktype bionic::from_host_socktype(int socktype) {
    switch (socktype) {
        case SOCK_STREAM: return socktype::STREAM;
        case SOCK_DGRAM: return socktype::DGRAM;
        case SOCK_RAW: return socktype::RAW;
        default: throw std::runtime_error("Unknown socktype");
    }
}

int bionic::to_host_socktype(bionic::socktype socktype) {
    switch (socktype) {
        case socktype::STREAM: return SOCK_STREAM;
        case socktype::DGRAM: return SOCK_DGRAM;
        case socktype::RAW: return SOCK_RAW;
        default: throw std::runtime_error("Unknown socktype");
    }
}

bionic::ipproto bionic::from_host_ipproto(int proto) {
    switch (proto) {
        case IPPROTO_IP: return ipproto::IP;
        case IPPROTO_TCP: return ipproto::TCP;
        case IPPROTO_UDP: return ipproto::UDP;
        default: throw std::runtime_error("Unknown ipproto");
    }
}

int bionic::to_host_ipproto(bionic::ipproto proto) {
    switch (proto) {
        case ipproto::IP: return IPPROTO_IP;
        case ipproto::TCP: return IPPROTO_TCP;
        case ipproto::UDP: return IPPROTO_UDP;
        default: throw std::runtime_error("Unknown ipproto");
    }
}

void bionic::from_host(const ::sockaddr *in, bionic::sockaddr *out) {
    switch (in->sa_family) {
        case AF_UNSPEC:
            out->family = (uint16_t) af_family::UNSPEC;
            break;
        case AF_INET:
            out->family = (uint16_t) af_family::INET;
            ((bionic::sockaddr_in *) out)->port = ((::sockaddr_in*) in)->sin_port;
            ((bionic::sockaddr_in *) out)->addr = ((::sockaddr_in*) in)->sin_addr.s_addr;
            break;
        case AF_INET6:
            out->family = (uint16_t) af_family::INET6;
            ((bionic::sockaddr_in6 *) out)->port = ((::sockaddr_in6*) in)->sin6_port;
            ((bionic::sockaddr_in6 *) out)->flow_info = ((::sockaddr_in6*) in)->sin6_flowinfo;
            memcpy(((bionic::sockaddr_in6 *) out)->addr, ((::sockaddr_in6*) in)->sin6_addr.s6_addr, 16);
            ((bionic::sockaddr_in6 *) out)->scope = ((::sockaddr_in6*) in)->sin6_scope_id;
            break;
        default: throw std::runtime_error("Unknown socket family when converting sockaddr from host");
    }
}

void bionic::to_host(const bionic::sockaddr *in, ::sockaddr *out) {
    switch ((af_family) in->family) {
        case af_family::UNSPEC:
            out->sa_family = AF_UNSPEC;
            break;
        case af_family::INET:
            out->sa_family = AF_INET;
            ((::sockaddr_in*) out)->sin_port = ((bionic::sockaddr_in *) in)->port;
            ((::sockaddr_in*) out)->sin_addr.s_addr = ((bionic::sockaddr_in *) in)->addr;
            break;
        case af_family::INET6:
            out->sa_family = AF_INET6;
            ((::sockaddr_in6*) out)->sin6_port = ((bionic::sockaddr_in6 *) in)->port;
            ((::sockaddr_in6*) out)->sin6_flowinfo = ((bionic::sockaddr_in6 *) in)->flow_info;
            memcpy(((::sockaddr_in6*) out)->sin6_addr.s6_addr, ((bionic::sockaddr_in6 *) in)->addr, 16);
            ((::sockaddr_in6*) out)->sin6_scope_id = ((bionic::sockaddr_in6 *) in)->scope;
            break;
        default: throw std::runtime_error("Unknown socket family when converting sockaddr to host");
    }
}

size_t bionic::get_host_len(const bionic::sockaddr *in) {
    if (!in)
        return 0;
    switch ((af_family) in->family) {
        case af_family::INET: return sizeof(::sockaddr_in);
        case af_family::INET6: return sizeof(::sockaddr_in6);
        default: throw std::runtime_error("Unknown socket family when converting sockaddr to host");
    }
}

size_t bionic::get_bionic_len(const ::sockaddr *in) {
    if (!in)
        return 0;
    switch (in->sa_family) {
        case AF_INET: return sizeof(bionic::sockaddr_in);
        case AF_INET6: return sizeof(bionic::sockaddr_in6);
        default: throw std::runtime_error("Unknown socket family when converting sockaddr from host");
    }
}

bionic::addrinfo* bionic::from_host_alloc(const ::addrinfo *in) {
    if (!in)
        return nullptr;

    auto out = new addrinfo;
    out->ai_flags = from_host_ai_flags(in->ai_flags);
    out->ai_family = from_host_af_family(in->ai_family);
    out->ai_socktype = from_host_socktype(in->ai_socktype);
    out->ai_protocol = from_host_ipproto(in->ai_protocol);
    out->ai_addrlen = get_bionic_len(in->ai_addr);
    out->ai_addr = nullptr;
    if (out->ai_addrlen) {
        out->ai_addr = (bionic::sockaddr *) malloc(out->ai_addrlen);
        from_host(in->ai_addr, out->ai_addr);
    }
    out->ai_canonname = in->ai_canonname ? strdup(in->ai_canonname) : nullptr;

    out->ai_next = from_host_alloc(in->ai_next);
    return out;
}

void bionic::free_bionic_list(bionic::addrinfo *list) {
    while (list) {
        free(list->ai_addr);
        free(list->ai_canonname);
        auto tmp = list;
        list = list->ai_next;
        delete tmp;
    }
}

::addrinfo* bionic::to_host_alloc(const bionic::addrinfo *in) {
    if (!in)
        return nullptr;

    auto out = new ::addrinfo;
    out->ai_flags = to_host_ai_flags(in->ai_flags);
    out->ai_family = to_host_af_family(in->ai_family);
    out->ai_socktype = to_host_socktype(in->ai_socktype);
    out->ai_protocol = to_host_ipproto(in->ai_protocol);
    out->ai_addrlen = get_host_len(in->ai_addr);
    out->ai_addr = nullptr;
    if (out->ai_addrlen) {
        out->ai_addr = (::sockaddr *) malloc(out->ai_addrlen);
        to_host(in->ai_addr, out->ai_addr);
    }
    out->ai_canonname = in->ai_canonname;

    out->ai_next = to_host_alloc(in->ai_next);
    return out;
}

void bionic::free_host_list(::addrinfo *list) {
    while (list) {
        free(list->ai_addr);
        auto tmp = list;
        list = list->ai_next;
        delete tmp;
    }
}

int bionic::to_host_nameinfo_flags(bionic::nameinfo_flags flags) {
    int ret = 0;
    if ((uint32_t) flags & (uint32_t) nameinfo_flags::NOFQDN)
        ret |= NI_NOFQDN;
    if ((uint32_t) flags & (uint32_t) nameinfo_flags::NUMERICHOST)
        ret |= NI_NUMERICHOST;
    if ((uint32_t) flags & (uint32_t) nameinfo_flags::NAMEREQD)
        ret |= NI_NAMEREQD;
    if ((uint32_t) flags & (uint32_t) nameinfo_flags::NUMERICSERV)
        ret |= NI_NUMERICSERV;
    if ((uint32_t) flags & (uint32_t) nameinfo_flags::DGRAM)
        ret |= NI_DGRAM;
    return ret;
}

int shim::socket(bionic::af_family domain, bionic::socktype type, bionic::ipproto proto) {
    return ::socket(bionic::to_host_af_family(domain), bionic::to_host_socktype(type), bionic::to_host_ipproto(proto));
}

int shim::getaddrinfo(const char *node, const char *service, const bionic::addrinfo *hints, bionic::addrinfo **res) {
    auto hhints = bionic::to_host_alloc(hints);
    ::addrinfo *hres;
    int ret = ::getaddrinfo(node, service, hhints, &hres);
    bionic::free_host_list(hhints);
    if (ret != 0)
        return ret;
    *res = bionic::from_host_alloc(hres);
    freeaddrinfo(hres);
    return 0;
}

void shim::freeaddrinfo(bionic::addrinfo *ai) {
    bionic::free_bionic_list(ai);
}

int shim::getnameinfo(const ::sockaddr *addr, socklen_t addrlen, char *host, socklen_t hostlen,
        char *serv, socklen_t servlen, bionic::nameinfo_flags flags) {
    return ::getnameinfo(addr, addrlen, host, hostlen, serv, servlen,
            bionic::to_host_nameinfo_flags(flags));
}

ssize_t shim::sendto(int sockfd, const void *buf, size_t len, int flags, const ::sockaddr *addr, socklen_t addrlen) {
    if (flags != 0)
        throw std::runtime_error("sendto with unsupported flags");
    return ::sendto(sockfd, buf, len, flags, addr, addrlen);
}

ssize_t shim::recvfrom(int sockfd, void *buf, size_t len, int flags, bionic::sockaddr *addr, socklen_t *addrlen) {
    if (flags != 0)
        throw std::runtime_error("recvfrom with unsupported flags");
    if (addr == nullptr)
        return ::recvfrom(sockfd, buf, len, flags, nullptr, nullptr);
    sockaddr_storage haddr;
    socklen_t haddrlen = sizeof(sockaddr_storage);
    int ret = ::recvfrom(sockfd, buf, len, flags, (::sockaddr *) &haddr, &haddrlen);
    if (ret < 0)
        return ret;
    if (bionic::get_bionic_len((::sockaddr *) &haddr) > *addrlen)
        throw std::runtime_error("recvfrom with buffer not big enough");
    bionic::from_host((::sockaddr *) &haddr, addr);
    *addrlen = bionic::get_bionic_len((::sockaddr *) &haddr);
    return ret;
}

int shim::getsockname(int sockfd, shim::bionic::sockaddr *addr, socklen_t *addrlen) {
    sockaddr_storage haddr;
    socklen_t haddrlen = sizeof(sockaddr_storage);
    int ret = ::getsockname(sockfd, (::sockaddr *) &haddr, &haddrlen);
    if (ret != 0)
        return ret;
    if (bionic::get_bionic_len((::sockaddr *) &haddr) > *addrlen)
        throw std::runtime_error("getsockname with buffer not big enough");
    bionic::from_host((::sockaddr *) &haddr, addr);
    *addrlen = bionic::get_bionic_len((::sockaddr *) &haddr);
    return 0;
}

void shim::add_network_shimmed_symbols(std::vector<shim::shimmed_symbol> &list) {
    list.insert(list.end(), {
        {"socket", socket},
        {"bind", &detail::arg_rewrite_helper<int (int, const ::sockaddr *, socklen_t)>::rewrite<::bind>},
        {"connect", &detail::arg_rewrite_helper<int (int, const ::sockaddr *, socklen_t)>::rewrite<::connect>},
        {"sendto", AutoArgRewritten(sendto)},
        {"recvfrom", recvfrom},
        {"getsockname", getsockname},

        {"getaddrinfo", getaddrinfo},
        {"freeaddrinfo", freeaddrinfo},
        {"getnameinfo", AutoArgRewritten(getnameinfo)},
    });
}