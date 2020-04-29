/*GRB*

    Gerbera - https://gerbera.io/
    
    upnp_clients.h - this file is part of Gerbera.
    
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

/// \file upnp_clients.h
/// \brief Definition of the Clients class.
/// inspired by https://sourceforge.net/p/minidlna/git/ci/master/tree/clients.h

#ifndef __UPNP_CLIENTS_H__
#define __UPNP_CLIENTS_H__

#include <chrono>
#include <memory>
#include <mutex>
#include <pugixml.hpp>
#include <sys/socket.h>
#include <vector>

#include "util/upnp_quirks.h"

// specific customer products
enum class ClientType {
    Unknown = 0, // if not listed otherwise
    BubbleUPnP,
    SamsungAllShare,
    SamsungSeriesQ,
    SamsungBDP,
    SamsungSeriesCDE,
    SamsungBDJ5500,
    StandardUPnP
};

// specify what must match
enum class ClientMatchType {
    None,
    UserAgent, // received via UpnpActionRequest, UpnpFileInfo and UpnpDiscovery (all might be slitely different)
    //FriendlyName,
    //ModelName,
};

struct ClientInfo {
    std::string name; // used for logging/debugging proposes only
    ClientType type;
    QuirkFlags flags;

    // to match the client
    ClientMatchType matchType;
    const char* match;
};

struct ClientCacheEntry {
    struct sockaddr_storage addr;
    std::chrono::time_point<std::chrono::steady_clock> age;

    const struct ClientInfo* pInfo;
};

class Clients {
public:
    static void addClientByDiscovery(const struct sockaddr_storage* addr, const std::string& userAgent, const std::string& descLocation);

    // always return something, 'Unknown' if we do not know better
    static void getInfo(std::shared_ptr<Config> config, const struct sockaddr_storage* addr, const std::string& userAgent, const ClientInfo** ppInfo);

private:
    static bool getInfoByType(const std::string& match, ClientMatchType type, const ClientInfo** ppInfo);
    static bool getInfoByConfig(const struct sockaddr_storage* addr, const std::string& userAgent, std::shared_ptr<Config> config, const ClientInfo** ppInfo);
    static bool downloadDescription(const std::string& location, std::unique_ptr<pugi::xml_document>& xml);

private:
    static std::mutex mutex;
    using AutoLock = std::lock_guard<std::mutex>;
    static std::unique_ptr<std::vector<struct ClientCacheEntry>> cache;
};

#endif // __UPNP_CLIENTS_H__
