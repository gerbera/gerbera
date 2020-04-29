/*GRB*

    Gerbera - https://gerbera.io/
    
    client_config.h - this file is part of Gerbera.
    
    Copyright (C) 2020 Gerbera Contributors
    
    Gerbera is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.

    Gerbera is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Gerbera.  If not, see <http://www.gnu.org/licenses/>.

    $Id$
*/

/// \file client_config.h
///\brief Definitions of the ClientConfig classes.

#ifndef __CLIENTCONFIG_H__
#define __CLIENTCONFIG_H__

#include <filesystem>
#include <mutex>
#include "util/upnp_clients.h"

// forward declaration
class ClientConfig;

class ClientConfigList {
public:
    explicit ClientConfigList();

    /// \brief Adds a new ClientConfig to the list.
    ///
    /// The scanID of the directory is invalidated and set to
    /// the index in the AutoscanList.
    ///
    /// \param dir ClientConfig to add to the list.
    /// \return scanID of the newly added ClientConfig
    void add(const std::shared_ptr<ClientConfig>& client);

    std::shared_ptr<ClientConfig> get(size_t id);

    size_t size() const { return list.size(); }

    /// \brief removes the ClientConfig given by its scan ID
    void remove(size_t id);

    /// \brief returns a copy of the autoscan list in the form of an array
    std::vector<std::shared_ptr<ClientConfig>> getArrayCopy();

protected:
    std::recursive_mutex mutex;
    using AutoLock = std::lock_guard<std::recursive_mutex>;

    std::vector<std::shared_ptr<ClientConfig>> list;
    void _add(const std::shared_ptr<ClientConfig>& client);
};

/// \brief Provides information about one manual client.
class ClientConfig {
public:
    ClientConfig();

    /// \brief Creates a new ClientConfig object.
    /// \param flags quirks flags
    /// \param ip ip address
    /// \param userAgent user agent
    /// \param clientType client type
    ClientConfig(int flags, std::string ip, std::string userAgent, ClientType clientType);

    ClientInfo* getClientInfo() { return &clientInfo; }

    ClientType getClientType() const { return clientInfo.type; }

    void setClientType(ClientType type) { this->clientInfo.type = type; }

    int getFlags() const { return clientInfo.flags; }

    void setFlags(int flags) { this->clientInfo.flags = flags; }

    std::string getIp() const { return ip; }

    void setIp(std::string ip) { this->ip = ip; }

    std::string getUserAgent() const { return userAgent; }

    void setUserAgent(std::string userAgent) { this->userAgent = userAgent; }

    /// \brief copies all properties to another object
    void copyTo(const std::shared_ptr<ClientConfig>& copy) const;

    /* helpers for clientType stuff */
    static std::string mapClientType(ClientType clientType);
    static ClientType remapClientType(const std::string& clientType);

protected:
    std::string ip;
    std::string userAgent;
    
    struct ClientInfo clientInfo;
};

#endif
