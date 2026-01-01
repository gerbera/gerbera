/*GRB*
Gerbera - https://gerbera.io/

    grb_net.cc - this file is part of Gerbera.

    Copyright (C) 2022-2026 Gerbera Contributors

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

/// @file util/grb_net.cc
#define GRB_LOG_FAC GrbLogFacility::clients
#include "grb_net.h" // API

#include "exceptions.h"
#include "logger.h"
#include "tools.h"

#include <arpa/inet.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <regex>

#ifdef SOLARIS
#include <sys/sockio.h>
#endif

#define SOCK_ADDR_PTR(sa) reinterpret_cast<const struct sockaddr*>(sa)

#define M_SOCK_ADDR_IN_PTR(sa) reinterpret_cast<struct sockaddr_in*>(sa)
#define M_SOCK_ADDR_IN_ADDR(sa) M_SOCK_ADDR_IN_PTR(sa)->sin_addr
#define M_SOCK_ADDR_IN6_PTR(sa) reinterpret_cast<struct sockaddr_in6*>(sa)
#define M_SOCK_ADDR_IN6_ADDR(sa) M_SOCK_ADDR_IN6_PTR(sa)->sin6_addr

/// @brief Compare sockaddr
/// inspired by: http://www.opensource.apple.com/source/postfix/postfix-197/postfix/src/util/sock_addr.c
#define SOCK_ADDR_IN_PTR(sa) reinterpret_cast<const struct sockaddr_in*>(sa)
#define SOCK_ADDR_IN6_PTR(sa) reinterpret_cast<const struct sockaddr_in6*>(sa)

static int sockAddrCmpAddr(
    const struct sockaddr* sa,
    const struct sockaddr* sb,
    int prefix = -1)
{
    if (sa->sa_family != sb->sa_family)
        return sa->sa_family - sb->sa_family;

    if (sa->sa_family == AF_INET) {
        auto sin_a = SOCK_ADDR_IN_PTR(sa);
        auto sin_b = SOCK_ADDR_IN_PTR(sb);
        uint32_t addr_a = ntohl(sin_a->sin_addr.s_addr);
        uint32_t addr_b = ntohl(sin_b->sin_addr.s_addr);

        uint32_t netmask = 0;
        if (prefix > 0 && prefix < 33) {
            netmask = prefix == 32 ? 0xFFFFFFFF : ~((1U << (32 - prefix)) - 1);
        }

        if (netmask > 0 && (addr_a & netmask) == (addr_b & netmask)) {
            return 0;
        }
        return (addr_a < addr_b) ? -1 : (addr_a > addr_b ? 1 : 0);
    }

    if (sa->sa_family == AF_INET6) {
        auto sin6_a = SOCK_ADDR_IN6_PTR(sa);
        auto sin6_b = SOCK_ADDR_IN6_PTR(sb);

        if (prefix > 0 && prefix < 129) {
            int full_bytes = prefix / 8;
            int rem_bits = prefix % 8;

            // Compare full bytes
            int res = std::memcmp(sin6_a->sin6_addr.s6_addr, sin6_b->sin6_addr.s6_addr, full_bytes);
            if (res != 0)
                return res;

            // If there are remaining bits, compare them
            if (rem_bits > 0) {
                uint8_t mask = (~0U) << (8 - rem_bits);
                uint8_t a = sin6_a->sin6_addr.s6_addr[full_bytes] & mask;
                uint8_t b = sin6_b->sin6_addr.s6_addr[full_bytes] & mask;
                if (a != b)
                    return (a < b) ? -1 : 1;
            }
            return 0;
        }
        // Compare all 16 bytes
        int res = std::memcmp(sin6_a->sin6_addr.s6_addr, sin6_b->sin6_addr.s6_addr, 16);
        return res;
    }

    throw_std_runtime_error("Unsupported address family: {}", sa->sa_family);
}

GrbNet::GrbNet(std::string addr, int af)
{
    if (addr.find(':') != std::string::npos) {
        af = AF_INET6;
        auto re = std::regex("%.*$");
        addr = std::regex_replace(addr, re, "");
    }
    reinterpret_cast<struct sockaddr*>(&sockAddr)->sa_family = af;
    int err = 0;
    if (af == AF_INET) {
        err = inet_pton(af, addr.c_str(), &(M_SOCK_ADDR_IN_ADDR(&sockAddr)));
    } else if (af == AF_INET6) {
        err = inet_pton(af, addr.c_str(), &(M_SOCK_ADDR_IN6_ADDR(&sockAddr)));
    }
    if (err != 1) {
        log_error("Could not parse address {} (error = {})", addr, err);
    }
}

GrbNet::GrbNet(const struct sockaddr_storage* addr)
{
    sockAddr = *addr;
}

void GrbNet::setPort(in_port_t port)
{
    M_SOCK_ADDR_IN_PTR(&sockAddr)->sin_port = port;
}

in_port_t GrbNet::getPort() const
{
    return SOCK_ADDR_IN_PTR(&sockAddr)->sin_port;
}

int GrbNet::getAdressFamily() const
{
    return SOCK_ADDR_PTR(&sockAddr)->sa_family;
}

bool GrbNet::equals(const std::string& match) const
{
    auto net = splitString(match, '/');
    const auto& ip = net.front();
    if (ip.find(':') != std::string::npos) {
        auto prefix = stoiString(net.size() > 1 ? net[1] : "128", 128);
        // IPv6
        struct sockaddr_in6 clientAddr = {};
        clientAddr.sin6_family = AF_INET6;
        if ((inet_pton(AF_INET6, ip.c_str(), &clientAddr.sin6_addr) == 1) && (sockAddrCmpAddr(SOCK_ADDR_PTR(&clientAddr), SOCK_ADDR_PTR(&sockAddr), prefix) == 0))
            return true;
    } else if (ip.find('.') != std::string::npos) {

        auto prefix = stoiString(net.size() > 1 ? net[1] : "32", 32);
        // IPv4
        struct sockaddr_in clientAddr = {};
        clientAddr.sin_family = AF_INET;
        clientAddr.sin_addr.s_addr = inet_addr(ip.c_str());
        if (sockAddrCmpAddr(SOCK_ADDR_PTR(&clientAddr), SOCK_ADDR_PTR(&sockAddr), prefix) == 0)
            return true;
    }
    return false;
}

bool GrbNet::equals(const std::shared_ptr<GrbNet>& other) const
{
    return sockAddrCmpAddr(SOCK_ADDR_PTR(&other->sockAddr), SOCK_ADDR_PTR(&sockAddr)) == 0;
}

std::string GrbNet::getHostName()
{
    if (!hostName.empty())
        return hostName;

    auto addr = SOCK_ADDR_PTR(&sockAddr);
    char hoststr[NI_MAXHOST] = "";
    char portstr[NI_MAXSERV] = "";
    int len = addr->sa_family == AF_INET6 ? sizeof(struct sockaddr_in6) : sizeof(struct sockaddr_in);
    int ret = getnameinfo(addr, len, hoststr, sizeof(hoststr), portstr, sizeof(portstr), NI_NOFQDN);
    if (ret != 0) {
        log_warning("Could not determine getnameinfo: {}", std::strerror(errno));
        hoststr[0] = '\0';
    }
    hostName = hoststr;
    if (hostName.empty()) {
        ret = getnameinfo(addr, len, hoststr, sizeof(hoststr), portstr, sizeof(portstr), NI_NUMERICHOST);
        if (ret != 0) {
            log_warning("Could not determine getnameinfo: {}", std::strerror(errno));
            hoststr[0] = '\0';
            return hostName;
        }
        hostName = hoststr;
    }

    return hoststr;
}

std::string GrbNet::getNameInfo(bool withPort) const
{
    auto addr = SOCK_ADDR_PTR(&sockAddr);
    char hoststr[NI_MAXHOST] = "";
    char portstr[NI_MAXSERV] = "";
    int len = addr->sa_family == AF_INET6 ? sizeof(struct sockaddr_in6) : sizeof(struct sockaddr_in);
    int ret = getnameinfo(addr, len, hoststr, sizeof(hoststr), portstr, sizeof(portstr), NI_NUMERICHOST | NI_NUMERICSERV);
    if (ret != 0) {
        throw_fmt_system_error("could not determine getnameinfo");
    }

    return withPort ? fmt::format("{}:{}", hoststr, portstr) : hoststr;
}

std::string GrbNet::ipToInterface(const std::string& ip)
{
    if (ip.empty()) {
        return {};
    }

    log_warning("Please provide an interface name instead! LibUPnP does not support specifying an IP in current versions.");
    log_info("Attempting to map {} to an interface", ip);

    struct ifaddrs* ifaddr;
    int family, s;
    char host[NI_MAXHOST] = "";

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

std::string GrbNet::renderWebUri(const std::string& ip, in_port_t port)
{
    if (port == 0) {
        if (ip.find(':') != std::string_view::npos) {
            return fmt::format("[{}]", ip);
        }
        return fmt::format("{}", ip);
    }
    if (ip.find(':') != std::string_view::npos) {
        return fmt::format("[{}]:{}", ip, port);
    }
    return fmt::format("{}:{}", ip, port);
}
