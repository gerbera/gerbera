/*GRB*
Gerbera - https://gerbera.io/

    grb_net.cc - this file is part of Gerbera.

    Copyright (C) 2022 Gerbera Contributors

    Gerbera is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.

    Gerbera is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Gerbera.  If not, see <http://www.gnu.org/licenses/>.
*/

/// \file grb_net.cc

#include "grb_net.h" // API

#include <arpa/inet.h>
#include <ifaddrs.h>
#include <netdb.h>

#ifdef SOLARIS
#include <sys/sockio.h>
#endif

#include "common.h"
#include "tools.h"

#define M_SOCK_ADDR_IN_PTR(sa) reinterpret_cast<struct sockaddr_in*>(sa)
#define M_SOCK_ADDR_IN_ADDR(sa) M_SOCK_ADDR_IN_PTR(sa)->sin_addr
#define M_SOCK_ADDR_IN6_PTR(sa) reinterpret_cast<struct sockaddr_in6*>(sa)
#define M_SOCK_ADDR_IN6_ADDR(sa) M_SOCK_ADDR_IN6_PTR(sa)->sin6_addr

/// \brief Compare sockaddr
/// inspired by: http://www.opensource.apple.com/source/postfix/postfix-197/postfix/src/util/sock_addr.c
#define SOCK_ADDR_IN_PTR(sa) reinterpret_cast<const struct sockaddr_in*>(sa)
#define SOCK_ADDR_IN_ADDR(sa) SOCK_ADDR_IN_PTR(sa)->sin_addr
#define SOCK_ADDR_IN6_PTR(sa) reinterpret_cast<const struct sockaddr_in6*>(sa)
#define SOCK_ADDR_IN6_ADDR(sa) SOCK_ADDR_IN6_PTR(sa)->sin6_addr
static int sockAddrCmpAddr(const struct sockaddr* sa, const struct sockaddr* sb)
{
    if (sa->sa_family != sb->sa_family)
        return sa->sa_family - sb->sa_family;

    if (sa->sa_family == AF_INET) {
        return SOCK_ADDR_IN_ADDR(sa).s_addr - SOCK_ADDR_IN_ADDR(sb).s_addr;
    }

    if (sa->sa_family == AF_INET6) {
        return std::memcmp(&(SOCK_ADDR_IN6_ADDR(sa)), &(SOCK_ADDR_IN6_ADDR(sb)), sizeof(SOCK_ADDR_IN6_ADDR(sa)));
    }

    throw_std_runtime_error("Unsupported address family: {}", sa->sa_family);
}

GrbNet::GrbNet(const std::string& addr, int af)
{
    reinterpret_cast<struct sockaddr*>(&sockAddr)->sa_family = af;
    int err = 0;
    if (af == AF_INET) {
        err = inet_pton(af, addr.c_str(), &(M_SOCK_ADDR_IN_ADDR(&sockAddr)));
    } else if (af == AF_INET6) {
        err = inet_pton(af, addr.c_str(), &(M_SOCK_ADDR_IN6_ADDR(&sockAddr)));
    }
    if (err != 1)
        log_error("Could not parse address {}", addr);
}

GrbNet::GrbNet(const struct sockaddr_storage* addr)
{
    std::memcpy(&sockAddr, addr, sizeof(sockAddr));
}

void GrbNet::setPort(in_port_t port)
{
    reinterpret_cast<struct sockaddr_in*>(&sockAddr)->sin_port = port;
}

in_port_t GrbNet::getPort() const
{
    return reinterpret_cast<const struct sockaddr_in*>(&sockAddr)->sin_port;
}

int GrbNet::getAdressFamily() const
{
    return reinterpret_cast<const struct sockaddr*>(&sockAddr)->sa_family;
}

bool GrbNet::equals(const std::string& match) const
{
    if (match.find(':') != std::string::npos) {
        // IPv6
        struct sockaddr_in6 clientAddr = {};
        clientAddr.sin6_family = AF_INET6;
        if ((inet_pton(AF_INET6, match.c_str(), &clientAddr.sin6_addr) == 1) && (sockAddrCmpAddr(reinterpret_cast<const struct sockaddr*>(&clientAddr), reinterpret_cast<const struct sockaddr*>(&sockAddr)) == 0))
            return true;
    } else if (match.find('.') != std::string::npos) {
        // IPv4
        struct sockaddr_in clientAddr = {};
        clientAddr.sin_family = AF_INET;
        clientAddr.sin_addr.s_addr = inet_addr(match.c_str());
        if (sockAddrCmpAddr(reinterpret_cast<const struct sockaddr*>(&clientAddr), reinterpret_cast<const struct sockaddr*>(&sockAddr)) == 0)
            return true;
    }
    return false;
}

bool GrbNet::equals(const std::shared_ptr<GrbNet>& other) const
{
    return sockAddrCmpAddr(reinterpret_cast<const struct sockaddr*>(&other->sockAddr), reinterpret_cast<const struct sockaddr*>(&sockAddr)) == 0;
}

std::string GrbNet::getHostName() const
{
    auto addr = reinterpret_cast<const struct sockaddr*>(&sockAddr);
    char hoststr[NI_MAXHOST];
    char portstr[NI_MAXSERV];
    int len = addr->sa_family == AF_INET6 ? sizeof(struct sockaddr_in6) : sizeof(struct sockaddr_in);
    int ret = getnameinfo(addr, len, hoststr, sizeof(hoststr), portstr, sizeof(portstr), NI_NOFQDN);
    if (ret != 0) {
        log_debug("could not determine getnameinfo: {}", std::strerror(errno));
    }

    return hoststr;
}

std::string GrbNet::getNameInfo(bool withPort) const
{
    auto addr = reinterpret_cast<const struct sockaddr*>(&sockAddr);
    char hoststr[NI_MAXHOST];
    char portstr[NI_MAXSERV];
    int len = addr->sa_family == AF_INET6 ? sizeof(struct sockaddr_in6) : sizeof(struct sockaddr_in);
    int ret = getnameinfo(addr, len, hoststr, sizeof(hoststr), portstr, sizeof(portstr), NI_NUMERICHOST | NI_NUMERICSERV);
    if (ret != 0) {
        throw_fmt_system_error("could not determine getnameinfo");
    }

    return withPort ? fmt::format("{}:{}", hoststr, portstr) : hoststr;
}

std::string GrbNet::ipToInterface(std::string_view ip)
{
    if (ip.empty()) {
        return {};
    }

    log_warning("Please provide an interface name instead! LibUPnP does not support specifying an IP in current versions.");
    log_info("Attempting to map {} to an interface", ip);

    struct ifaddrs* ifaddr;
    int family, s;
    char host[NI_MAXHOST];

    if (getifaddrs(&ifaddr) == -1) {
        log_error("Could not getifaddrs: {}", std::strerror(errno));
    }

    for (auto ifa = ifaddr; ifa; ifa = ifa->ifa_next) {
        if (!ifa->ifa_addr)
            continue;

        family = ifa->ifa_addr->sa_family;
        std::string name = ifa->ifa_name;

        if (family == AF_INET || family == AF_INET6) {
            s = getnameinfo(ifa->ifa_addr,
                (family == AF_INET) ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6),
                host, NI_MAXHOST, nullptr, 0, NI_NUMERICHOST);
            if (s != 0) {
                log_error("getnameinfo() failed: {}", gai_strerror(s));
                return {};
            }

            std::string ipaddr = host;
            // IPv6 link locals come back as fe80::351d:d7f4:6b17:3396%eth0
            if (startswith(ipaddr, ip)) {
                return name;
            }
        }
    }

    freeifaddrs(ifaddr);
    log_warning("Failed to find interface for IP: {}", ip);
    return {};
}
