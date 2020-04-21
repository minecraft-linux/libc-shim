#include "file_misc.h"

#include <sys/ioctl.h>
#include <net/if.h>
#include <stdexcept>
#include <string.h>
#include "network.h"

using namespace shim;

int shim::ioctl(int fd, bionic::ioctl_index cmd, void *arg) {
    switch (cmd) {
        case bionic::ioctl_index::FILE_NBIO:
            return ::ioctl(fd, FIONBIO, arg);
        case bionic::ioctl_index::SOCKET_CGIFCONF: {
            auto buf = (bionic::ifconf *) arg;
            size_t cnt = buf->len / sizeof(bionic::ifreq);
            ifconf *hbuf = new ifconf[cnt * sizeof(::ifreq)];
            int ret = ::ioctl(fd, SIOCGIFCONF, hbuf);
            if (ret < 0)
                return ret;
            cnt = hbuf->ifc_len / sizeof(::ifreq);
            buf->len = cnt * sizeof(bionic::ifreq);
            for (size_t i = 0; i < cnt; i++) {
                strncpy(buf->req->name, hbuf->ifc_req[i].ifr_name, 16);
                hbuf->ifc_req[i].ifr_name[15] = 0;

                bionic::from_host(&hbuf->ifc_req[i].ifr_addr, &buf->req->addr);
            }
            delete[] hbuf;
            return ret;
        }
        default:
            throw std::runtime_error("Unsupported ioctl");
    }
}

void shim::add_ioctl_shimmed_symbols(std::vector<shim::shimmed_symbol> &list) {
    list.push_back({"ioctl", ioctl});
}