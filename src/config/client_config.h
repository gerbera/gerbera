/*GRB*

    Gerbera - https://gerbera.io/

    client_config.h - this file is part of Gerbera.

    Copyright (C) 2020-2024 Gerbera Contributors

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

#include <map>
#include <mutex>
#include <vector>

#include "util/upnp_clients.h"
#include "util/upnp_quirks.h"

// forward declaration
class ClientConfig;

class ClientConfigList {
public:
    /// \brief Adds a new ClientConfig to the list.
    ///
    /// \param dir ClientConfig to add to the list.
    /// \return scanID of the newly added ClientConfig
    void add(const std::shared_ptr<ClientConfig>& client, std::size_t index = std::numeric_limits<std::size_t>::max());

    std::shared_ptr<ClientConfig> get(std::size_t id, bool edit = false) const;

    std::size_t getEditSize() const;

    std::size_t size() const { return list.size(); }

    /// \brief removes the ClientConfig given by its scan ID
    void remove(std::size_t id, bool edit = false);

    /// \brief returns a copy of the client config list in the form of an array
    std::vector<std::shared_ptr<ClientConfig>> getArrayCopy() const;

protected:
    std::size_t origSize {};
    std::map<std::size_t, std::shared_ptr<ClientConfig>> indexMap;

    mutable std::recursive_mutex mutex;
    using AutoLock = std::scoped_lock<std::recursive_mutex>;

    std::vector<std::shared_ptr<ClientConfig>> list;
    void _add(const std::shared_ptr<ClientConfig>& client, std::size_t index);
};

/// \brief Provides information about one manual client.
class ClientConfig {
public:
    ClientConfig() = default;

    /// \brief Creates a new ClientConfig object.
    /// \param flags quirks flags
    /// \param ip ip address
    /// \param userAgent user agent
    ClientConfig(int flags, std::string_view group, std::string_view ip, std::string_view userAgent, const std::map<std::string, std::string>& mimeMappings, int captionInfoCount, int stringLimit, bool multiValue);

    const ClientInfo& getClientInfo() const { return clientInfo; }

    int getFlags() const { return clientInfo.flags; }
    void setFlags(int flags) { this->clientInfo.flags = flags; }

    std::map<std::string, std::string> getMimeMapings() const { return this->clientInfo.mimeMappings; }

    int getCaptionInfoCount() const { return this->clientInfo.captionInfoCount; }
    void setCaptionInfoCount(int captionInfoCount)
    {
        this->clientInfo.captionInfoCount = captionInfoCount;
    }

    int getStringLimit() const { return this->clientInfo.stringLimit; }
    void setStringLimit(int stringLimit)
    {
        this->clientInfo.stringLimit = stringLimit;
    }

    bool getMultiValue() const { return this->clientInfo.multiValue; }
    void setMultiValue(bool multiValue)
    {
        this->clientInfo.multiValue = multiValue;
    }

    std::string getIp() const { return (this->clientInfo.matchType == ClientMatchType::IP) ? this->clientInfo.match : ""; }
    void setIp(std::string_view ip)
    {
        this->clientInfo.matchType = ClientMatchType::IP;
        this->clientInfo.match = ip;
    }

    std::string getGroup() const { return this->clientInfo.group; }
    void setGroup(std::string_view group)
    {
        this->clientInfo.group = group;
    }

    std::string getUserAgent() const { return (this->clientInfo.matchType == ClientMatchType::UserAgent) ? this->clientInfo.match : ""; }
    void setUserAgent(std::string_view userAgent)
    {
        this->clientInfo.matchType = ClientMatchType::UserAgent;
        this->clientInfo.match = userAgent;
    }

    /* helpers for clientType stuff */
    static std::string_view mapClientType(ClientType clientType);
    static std::string_view mapMatchType(ClientMatchType matchType);
    static int remapFlag(const std::string& flag);
    static int makeFlags(const std::string& optValue);

    static std::string mapFlags(QuirkFlags flags);

    void setOrig(bool orig) { this->isOrig = orig; }

    bool getOrig() const { return isOrig; }

protected:
    bool isOrig {};
    ClientInfo clientInfo {};
};

#endif
