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

#ifndef __UPNP_CLIENTS_H__
#define __UPNP_CLIENTS_H__

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
    StandardUPnP
};

// specify what must match
enum class ClientMatchType {
    None,
    UserAgent, // received via UpnpActionRequest, UpnpFileInfo and UpnpDiscovery (all might be slitely different)
    //FriendlyName,
    //ModelName,
    IP, // use client's network address
};

struct ClientInfo {
    std::string name; // used for logging/debugging proposes only
    ClientType type;
    QuirkFlags flags;

    // to match the client
    ClientMatchType matchType;
    std::string match;
};

struct ClientCacheEntry {
    struct sockaddr_storage addr;
    std::string userAgent;
    std::chrono::seconds last;
    std::chrono::seconds age;
    const struct ClientInfo* pInfo;
};

class Clients {
public:
    explicit Clients(const std::shared_ptr<Config>& config);

    // always return something, 'Unknown' if we do not know better
    void getInfo(const struct sockaddr_storage* addr, const std::string& userAgent, const ClientInfo** ppInfo);

    void addClientByDiscovery(const struct sockaddr_storage* addr, const std::string& userAgent, const std::string& descLocation);
    std::shared_ptr<std::vector<ClientCacheEntry>> getClientList() { return cache; }

private:
    bool getInfoByAddr(const struct sockaddr_storage* addr, const ClientInfo** ppInfo);
    bool getInfoByType(const std::string& match, ClientMatchType type, const ClientInfo** ppInfo);

    bool getInfoByCache(const struct sockaddr_storage* addr, const ClientInfo** ppInfo);
    void updateCache(const struct sockaddr_storage* addr, const std::string& userAgent, const ClientInfo* pInfo);

    static bool downloadDescription(const std::string& location, std::unique_ptr<pugi::xml_document> xml);

    std::mutex mutex;
    using AutoLock = std::lock_guard<std::mutex>;
    std::shared_ptr<std::vector<ClientCacheEntry>> cache;

    // table of supported clients (reverse search, sequence of entries matters!)
    std::vector<ClientInfo> clientInfo {
        // Used for not explicitly listed clients, must be first entry
        {
            "Unknown",
            ClientType::Unknown,
            QUIRK_FLAG_NONE,
            ClientMatchType::None,
            "",
        },

        // Gerbera, FRITZ!Box, Windows 10, etc...
        // User-Agent(actionReq): Linux/5.4.0-4-amd64, UPnP/1.0, Portable SDK for UPnP devices/1.8.6
        // User-Agent(actionReq): FRITZ!Box 5490 UPnP/1.0 AVM FRITZ!Box 5490 151.07.12
        // User-Agent(actionReq): Microsoft-Windows/10.0 UPnP/1.0 Microsoft-DLNA DLNADOC/1.50
        {
            "Standard UPnP",
            ClientType::StandardUPnP,
            QUIRK_FLAG_NONE,
            ClientMatchType::UserAgent,
            "UPnP/1.0",
        },

        // User-Agent(discovery): Linux/3.18.91-14843133-QB28034466 UPnP/1.0 BubbleUPnP/3.4.4
        // User-Agent(fileInfo ): BubbleUPnP UPnP/1.1
        // User-Agent(actionReq): Android/8.0.0 UPnP/1.0 BubbleUPnP/3.4.4
        {
            "BubbleUPnP",
            ClientType::BubbleUPnP,
            QUIRK_FLAG_NONE,
            ClientMatchType::UserAgent,
            "BubbleUPnP",
        },

        // User-Agent(actionReq): DLNADOC/1.50 SEC_HHP_[TV]UE40D7000/1.0
        // User-Agent(actionReq): DLNADOC/1.50 SEC_HHP_ Family TV/1.0
        // User-Agent(actionReq): DLNADOC/1.50 SEC_HHP_[TV] UE65JU7000/1.0 UPnP/1.0
        {
            "Samsung other TVs",
            ClientType::SamsungSeriesCDE,
            QUIRK_FLAG_SAMSUNG,
            ClientMatchType::UserAgent,
            "SEC_HHP_",
        },

        // This is AllShare running on a PC. We don't want to respond with Samsung capabilities, or Windows (and AllShare) might get grumpy.
        // User-Agent(actionReq): DLNADOC/1.50 SEC_HHP_[PC]LPC001/1.0  MS-DeviceCaps/1024
        {
            "AllShare",
            ClientType::SamsungAllShare,
            QUIRK_FLAG_NONE,
            ClientMatchType::UserAgent,
            "SEC_HHP_[PC]",
        },

        // User-Agent(actionReq): DLNADOC/1.50 SEC_HHP_[TV] Samsung Q7 Series (49)/1.0
        {
            "Samsung Series [Q] TVs",
            ClientType::SamsungSeriesQ,
            QUIRK_FLAG_SAMSUNG,
            ClientMatchType::UserAgent,
            "SEC_HHP_[TV] Samsung Q",
        },

        // User-Agent(actionReq): DLNADOC/1.50 SEC_HHP_BD-D5100/1.0
        {
            "Samsung Blu-ray Player BD-D5100",
            ClientType::SamsungBDP,
            QUIRK_FLAG_SAMSUNG,
            ClientMatchType::UserAgent,
            "SEC_HHP_BD",
        },

        // User-Agent: ?
        {
            "Samsung Blu-ray Player J5500",
            ClientType::SamsungBDJ5500,
            QUIRK_FLAG_SAMSUNG,
            ClientMatchType::UserAgent,
            "[BD]J5500",
        },
    };
};

#endif // __UPNP_CLIENTS_H__
