/*GRB*
Gerbera - https://gerbera.io/

    grb_net.h - this file is part of Gerbera.

    Copyright (C) 2022-2024 Gerbera Contributors

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

/// \file grb_net.h

#ifndef __GRB_NET_H__
#define __GRB_NET_H__

#include <memory>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>

class GrbNet {
private:
    struct sockaddr_storage sockAddr = {};
    std::string hostName;

public:
    explicit GrbNet(const std::string& addr, int af = AF_INET);
    explicit GrbNet(const struct sockaddr_storage* addr);

    void setPort(in_port_t port);
    in_port_t getPort() const;
    int getAdressFamily() const;
    struct sockaddr_storage readAddr() { return sockAddr; };
    std::string getHostName();
    void setHostName(const std::string& hn) { hostName = hn; };
    bool equals(const std::string& match) const;
    bool equals(const std::shared_ptr<GrbNet>& other) const;
    std::string getNameInfo(bool withPort = true) const;

    /// \brief Finds the Interface with the specified IP address.
    /// \param ip i.e. 192.168.4.56.
    /// \return Interface name or nullptr if IP was not found.
    static std::string ipToInterface(std::string_view ip);

    /// \brief Render browser friendly uri (handle IPv6)
    static std::string renderWebUri(std::string_view ip, in_port_t port);
};

#endif // __GRB_NET_H__
