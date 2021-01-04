/*GRB*

    Gerbera - https://gerbera.io/
    
    upnp_clients.cc - this file is part of Gerbera.
    
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

/// \file upnp_clients.cc
/// client info initially taken from https://sourceforge.net/p/minidlna/git/ci/master/tree/clients.cc

#include "upnp_clients.h" // API
#include "config/client_config.h"
#include "config/config.h"
#include "util/tools.h"

#include <upnp.h>

// table of supported clients (sequence of entries matters!)
std::vector<struct ClientInfo> Clients::clientInfo = std::vector<struct ClientInfo> {
    // Used for not explicitly listed clients, must be first entry
    {
        "Unknown",
        ClientType::Unknown,
        QUIRK_FLAG_NONE,
        ClientMatchType::None,
        "" },

    // User-Agent(discovery): Linux/3.18.91-14843133-QB28034466 UPnP/1.0 BubbleUPnP/3.4.4
    // User-Agent(fileInfo ): BubbleUPnP UPnP/1.1
    // User-Agent(actionReq): Android/8.0.0 UPnP/1.0 BubbleUPnP/3.4.4
    {
        "BubbleUPnP",
        ClientType::BubbleUPnP,
        QUIRK_FLAG_NONE,
        ClientMatchType::UserAgent,
        "BubbleUPnP" },

    // This is AllShare running on a PC. We don't want to respond with Samsung capabilities, or Windows (and AllShare) might get grumpy.
    // User-Agent: DLNADOC/1.50 SEC_HHP_[PC]LPC001/1.0  MS-DeviceCaps/1024
    {
        "AllShare",
        ClientType::SamsungAllShare,
        QUIRK_FLAG_NONE,
        ClientMatchType::UserAgent,
        "SEC_HHP_[PC]" },

    // User-Agent: DLNADOC/1.50 SEC_HHP_[TV] Samsung Q7 Series (49)/1.0
    {
        "Samsung Series [Q] TVs",
        ClientType::SamsungSeriesQ,
        QUIRK_FLAG_SAMSUNG,
        ClientMatchType::UserAgent,
        "SEC_HHP_[TV] Samsung Q" },

    // User-Agent: DLNADOC/1.50 SEC_HHP_BD-D5100/1.0
    {
        "Samsung Blu-ray Player BD-D5100",
        ClientType::SamsungBDP,
        QUIRK_FLAG_SAMSUNG,
        ClientMatchType::UserAgent,
        "SEC_HHP_BD" },

    // User-Agent: DLNADOC/1.50 SEC_HHP_[TV]UE40D7000/1.0
    // User-Agent: DLNADOC/1.50 SEC_HHP_ Family TV/1.0
    // User-Agent: DLNADOC/1.50 SEC_HHP_[TV] UE65JU7000/1.0 UPnP/1.0
    {
        "Samsung other TVs",
        ClientType::SamsungSeriesCDE,
        QUIRK_FLAG_SAMSUNG,
        ClientMatchType::UserAgent,
        "SEC_HHP_" },

    // User-Agent: ?
    {
        "Samsung Blu-ray Player J5500",
        ClientType::SamsungBDJ5500,
        QUIRK_FLAG_SAMSUNG,
        ClientMatchType::UserAgent,
        "[BD]J5500" },

    // User-Agent: VLC/3.0.11.1 LibVLC/3.0.11.1
    {
        "VLC",
        ClientType::VLC,
        QUIRK_FLAG_NONE,
        ClientMatchType::UserAgent,
        "LibVLC" },

    // Gerbera, FRITZ!Box, etc...
    // User-Agent: Linux/5.4.0-4-amd64, UPnP/1.0, Portable SDK for UPnP devices/1.8.6
    // User-Agent: FRITZ!Box 5490 UPnP/1.0 AVM FRITZ!Box 5490 151.07.12
    {
        "Standard UPnP",
        ClientType::StandardUPnP,
        QUIRK_FLAG_NONE,
        ClientMatchType::UserAgent,
        "UPnP/1.0" }
};

void Clients::addClientInfo(const std::shared_ptr<ClientInfo>& newClientInfo)
{
    clientInfo.push_back(*newClientInfo);
}

void Clients::addClientByDiscovery(const struct sockaddr_storage* addr, const std::string& userAgent, const std::string& descLocation)
{
#if 0 // only needed if UserAgent is not good enough
    const ClientInfo* info = nullptr;

    std::unique_ptr<pugi::xml_document> descXml;
    if (downloadDescription(descLocation, descXml)) {
        // TODO: search for FriendlyName + ModelName
        //(void)getInfoByType(userAgent, ClientMatchType::FriendlyName, &info);
    }

    {
        AutoLock lock(mutex);

        // even if not detect type we add entry, to log 'Found client' not to often
        bool found = false;
        for (auto& entry : *cache) {
            if (sockAddrCmpAddr((const struct sockaddr*)&entry.addr, (const struct sockaddr*)addr) != 0)
                continue;
            entry.age = std::chrono::steady_clock::now();
            if (entry.pInfo != info) {
                assert(clientInfo[0].type == ClientType::Unknown);
                log_debug("client update: {} '{}' -> '{}'", sockAddrGetNameInfo((const struct sockaddr*)addr), userAgent, info ? info->name : clientInfo[0].name);
            }
            entry.pInfo = info;
            found = true;
            break;
        }

        if (!found) {
            assert(clientInfo[0].type == ClientType::Unknown);
            log_debug("client add: {} '{}' -> '{}'", sockAddrGetNameInfo((const struct sockaddr*)addr), userAgent, info ? info->name : clientInfo[0].name);
            auto add = ClientCacheEntry();
            add.addr = *addr;
            add.age = std::chrono::steady_clock::now();
            add.pInfo = info;
            cache->push_back(add);
        }
    }
#endif
}

void Clients::getInfo(const struct sockaddr_storage* addr, const std::string& userAgent, const ClientInfo** ppInfo)
{
    const ClientInfo* info = nullptr;

    // by IP address
    bool found = getInfoByAddr(addr, &info);

    if (!found) {
        // by userAgent
        found = getInfoByType(userAgent, ClientMatchType::UserAgent, &info);
    }

    // by addClientByDiscovery
    if (!found) {
        // always return something, 'Unknown' if we do not know better
        assert(clientInfo[0].type == ClientType::Unknown);
        info = &clientInfo[0];

#if 0 // only needed if UserAgent is not good enough
        AutoLock lock(mutex);

        // house cleaning, remove old entries
        auto now = std::chrono::steady_clock::now();
        cache->erase(
            std::remove_if(cache->begin(), cache->end(),
                [now](const auto& entry) { return entry.age + std::chrono::hours(1) < now; }),
            cache->end());

        for (const auto& entry : *cache) {
            if (sockAddrCmpAddr((struct sockaddr*)&entry.addr, (struct sockaddr*)addr) != 0)
                continue;
            if (entry.pInfo) {
                info = entry.pInfo;
            }
            break;
        }
#endif
    }

    if (info) {
        auto it = std::find_if(cache->begin(), cache->end(), [=](const auto& entry) //
            { return sockAddrCmpAddr(reinterpret_cast<const struct sockaddr*>(&entry.addr), reinterpret_cast<const struct sockaddr*>(addr)) == 0 && entry.userAgent == userAgent; });

        if (it != cache->end()) {
            it->last = std::chrono::steady_clock::now();
            it->userAgent = userAgent;
            it->hostName = getHostName(reinterpret_cast<const struct sockaddr*>(addr));
        } else {
            auto add = ClientCacheEntry();
            add.addr = *addr;
            add.hostName = getHostName(reinterpret_cast<const struct sockaddr*>(addr));
            add.last = std::chrono::steady_clock::now();
            add.age = std::chrono::steady_clock::now();
            add.userAgent = userAgent;
            add.pInfo = info;
            cache->push_back(add);
        }
    }

    *ppInfo = info;
    log_debug("client info: {} '{}' -> '{}' as {}", sockAddrGetNameInfo(reinterpret_cast<const struct sockaddr*>(addr)), userAgent, (*ppInfo)->name, ClientConfig::mapClientType((*ppInfo)->type));
}

bool Clients::getInfoByAddr(const struct sockaddr_storage* addr, const ClientInfo** ppInfo)
{
    auto it = std::find_if(clientInfo.begin(), clientInfo.end(), [=](const auto& c) //
        { return !c.match.empty() && c.matchType == ClientMatchType::IP; });

    if (it != clientInfo.end()) {
        if (it->match.find('.') != std::string::npos) {
            struct sockaddr_in clientAddr;
            clientAddr.sin_family = AF_INET;
            clientAddr.sin_addr.s_addr = inet_addr(it->match.c_str());
            if (sockAddrCmpAddr(reinterpret_cast<const struct sockaddr*>(&clientAddr), reinterpret_cast<const struct sockaddr*>(addr)) == 0) {
                *ppInfo = &(*it);
                return true;
            }
        } else if (it->match.find(':') != std::string::npos) {
            struct sockaddr_in6 clientAddr;
            clientAddr.sin6_family = AF_INET6;
            if (inet_pton(AF_INET6, it->match.c_str(), &clientAddr.sin6_addr) == 1) {
                if (sockAddrCmpAddr(reinterpret_cast<const struct sockaddr*>(&clientAddr), reinterpret_cast<const struct sockaddr*>(addr)) == 0) {
                    *ppInfo = &(*it);
                    return true;
                }
            }
        }
    }

    *ppInfo = nullptr;
    return false;
}

bool Clients::getInfoByType(const std::string& match, ClientMatchType type, const ClientInfo** ppInfo)
{
    auto it = std::find_if(clientInfo.begin(), clientInfo.end(), [=](const auto& c) //
        { return c.matchType == type && match.find(c.match) != std::string::npos; });

    if (it != clientInfo.end()) {
        *ppInfo = &(*it);
        return true;
    }

    *ppInfo = nullptr;
    return false;
}

bool Clients::downloadDescription(const std::string& location, std::unique_ptr<pugi::xml_document>& xml)
{
#if defined(USING_NPUPNP)
    std::string description, ct;
    int errCode = UpnpDownloadUrlItem(location, description, ct);
    if (errCode != UPNP_E_SUCCESS) {
        log_debug("Error obtaining client description from {} -- error = {}", location, errCode);
        return false;
    }
    xml = std::make_unique<pugi::xml_document>();
    auto ret = xml->load_string(description.c_str());
#else
    IXML_Document* descDoc = nullptr;
    int errCode = UpnpDownloadXmlDoc(location.c_str(), &descDoc);
    if (errCode != UPNP_E_SUCCESS) {
        log_debug("Error obtaining client description from {} -- error = {}", location, errCode);
        return false;
    }

    DOMString cxml = ixmlPrintDocument(descDoc);
    xml = std::make_unique<pugi::xml_document>();
    auto ret = xml->load_string(cxml);

    ixmlFreeDOMString(cxml);
    ixmlDocument_free(descDoc);
#endif

    if (ret.status != pugi::xml_parse_status::status_ok) {
        log_debug("Unable to parse xml client description from {}", location);
        return false;
    }

    return true;
}

std::mutex Clients::mutex;
const std::shared_ptr<std::vector<struct ClientCacheEntry>> Clients::cache = std::make_shared<std::vector<struct ClientCacheEntry>>();
