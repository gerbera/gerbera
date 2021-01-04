/*GRB*

    Gerbera - https://gerbera.io/

    client_config.h - this file is part of Gerbera.

    Copyright (C) 2020-2021 Gerbera Contributors

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

#include "util/upnp_clients.h"
#include "util/upnp_quirks.h"
#include <filesystem>
#include <map>
#include <mutex>

// forward declaration
class ClientConfig;

class ClientConfigList {
public:
    explicit ClientConfigList() = default;

    /// \brief Adds a new ClientConfig to the list.
    ///
    /// \param dir ClientConfig to add to the list.
    /// \return scanID of the newly added ClientConfig
    void add(const std::shared_ptr<ClientConfig>& client, size_t index = SIZE_MAX);

    std::shared_ptr<ClientConfig> get(size_t id, bool edit = false);

    size_t getEditSize() const;

    size_t size() const { return list.size(); }

    /// \brief removes the ClientConfig given by its scan ID
    void remove(size_t id, bool edit = false);

    /// \brief returns a copy of the client config list in the form of an array
    std::vector<std::shared_ptr<ClientConfig>> getArrayCopy();

protected:
    size_t origSize;
    std::map<size_t, std::shared_ptr<ClientConfig>> indexMap;

    std::recursive_mutex mutex;
    using AutoLock = std::lock_guard<std::recursive_mutex>;

    std::vector<std::shared_ptr<ClientConfig>> list;
    void _add(const std::shared_ptr<ClientConfig>& client, size_t index);
};

/// \brief Provides information about one manual client.
class ClientConfig {
public:
    ClientConfig();

    /// \brief Creates a new ClientConfig object.
    /// \param flags quirks flags
    /// \param ip ip address
    /// \param userAgent user agent
    ClientConfig(int flags, const std::string& ip, const std::string& userAgent);

    std::shared_ptr<struct ClientInfo> getClientInfo() { return clientInfo; }

    int getFlags() const { return clientInfo->flags; }
    void setFlags(int flags) { this->clientInfo->flags = flags; }

    std::string getIp() const { return (this->clientInfo->matchType == ClientMatchType::IP) ? this->clientInfo->match : ""; }
    void setIp(const std::string& ip)
    {
        this->clientInfo->matchType = ClientMatchType::IP;
        this->clientInfo->match = ip;
    }

    std::string getUserAgent() const { return (this->clientInfo->matchType == ClientMatchType::UserAgent) ? this->clientInfo->match : ""; }
    void setUserAgent(const std::string& userAgent)
    {
        this->clientInfo->matchType = ClientMatchType::UserAgent;
        this->clientInfo->match = userAgent;
    }

    /// \brief copies all properties to another object
    void copyTo(const std::shared_ptr<ClientConfig>& copy) const;

    /* helpers for clientType stuff */
    static std::string mapClientType(ClientType clientType);
    static std::string mapMatchType(ClientMatchType matchType);
    static ClientType remapClientType(const std::string& clientType);
    static int remapFlag(const std::string& flag);
    static std::string mapFlags(QuirkFlags flags);

    void setOrig(bool orig) { this->isOrig = orig; }

    bool getOrig() const { return isOrig; }

protected:
    bool isOrig { false };
    std::shared_ptr<struct ClientInfo> clientInfo;
};

#endif
