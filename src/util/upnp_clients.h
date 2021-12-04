/*GRB*

    Gerbera - https://gerbera.io/

    upnp_clients.h - this file is part of Gerbera.

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

/// \file upnp_clients.h
/// \brief Definition of the Clients class.
/// inspired by https://sourceforge.net/p/minidlna/git/ci/master/tree/clients.h

#pragma once

#include <chrono>
#include <memory>
#include <mutex>
#include <pugixml.hpp>
#include <sys/socket.h>
#include <vector>

#include "util/upnp_quirks.h"

// forward declaration
class Config;

// specific customer products
enum class ClientType {
    Unknown = 0, // if not listed otherwise
    BubbleUPnP,
    SamsungAllShare,
    SamsungSeriesQ,
    SamsungBDP,
    SamsungSeriesCDE,
    SamsungBDJ5500,
    IRadio,
    StandardUPnP
};

// specify what must match
enum class ClientMatchType {
    None,
    UserAgent, // received via UpnpActionRequest, UpnpFileInfo and UpnpDiscovery (all might be slitely different)
    // FriendlyName,
    // ModelName,
    IP, // use client's network address
};

struct ClientInfo {
    std::string name { "Unknown" }; // used for logging/debugging proposes only
    ClientType type { ClientType::Unknown };
    QuirkFlags flags { QUIRK_FLAG_NONE };

    // to match the client
    ClientMatchType matchType { ClientMatchType::None };
    std::string match;
};

struct ClientCacheEntry {
    ClientCacheEntry(const struct sockaddr_storage& addr, std::string userAgent, std::chrono::seconds last, std::chrono::seconds age, const struct ClientInfo* pInfo)
        : addr(addr)
        , userAgent(std::move(userAgent))
        , last(last)
        , age(age)
        , pInfo(pInfo)
    {
    }

    struct sockaddr_storage addr;
    std::string userAgent;
    std::chrono::seconds last;
    std::chrono::seconds age;
    const struct ClientInfo* pInfo;
};

class Clients {
public:
    explicit Clients(const std::shared_ptr<Config>& config);
    void refresh(const std::shared_ptr<Config>& config);

    // always return something, 'Unknown' if we do not know better
    const ClientInfo* getInfo(const struct sockaddr_storage* addr, const std::string& userAgent);

    void addClientByDiscovery(const struct sockaddr_storage* addr, const std::string& userAgent, const std::string& descLocation);
    const std::vector<ClientCacheEntry>& getClientList() const { return cache; }

private:
    const ClientInfo* getInfoByAddr(const struct sockaddr_storage* addr);
    const ClientInfo* getInfoByType(const std::string& match, ClientMatchType type);

    const ClientInfo* getInfoByCache(const struct sockaddr_storage* addr);
    void updateCache(const struct sockaddr_storage* addr, std::string userAgent, const ClientInfo* pInfo);

    static std::unique_ptr<pugi::xml_document> downloadDescription(const std::string& location);

    std::mutex mutex;
    using AutoLock = std::scoped_lock<std::mutex>;
    std::vector<ClientCacheEntry> cache;

    std::vector<ClientInfo> clientInfo;
};
