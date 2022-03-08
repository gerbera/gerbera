/*GRB*

    Gerbera - https://gerbera.io/

    upnp_clients.cc - this file is part of Gerbera.

    Copyright (C) 2020-2022 Gerbera Contributors

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
#include "database/database.h"
#include "util/tools.h"

#include <array>

#include <upnp.h>

std::shared_ptr<ClientStatusDetail> ClientStatusDetail::clone() const
{
    return std::make_shared<ClientStatusDetail>(group, itemId, playCount, lastPlayed.count(), lastPlayedPosition.count(), bookMarkPos.count());
}

ClientManager::ClientManager(const std::shared_ptr<Config>& config, const std::shared_ptr<Database>& database)
    : database(database)
    , cacheThreshold(config->getIntOption(CFG_CLIENTS_CACHE_THRESHOLD))
{
    refresh(config);
    if (this->database) {
        auto dbCache = this->database->getClients();
        for (auto&& entry : dbCache) {
            auto info = getInfoByAddr(&entry.addr);
            if (!info) {
                info = getInfoByType(entry.userAgent, ClientMatchType::UserAgent);
            }
            if (!info) {
                assert(clientInfo[0].type == ClientType::Unknown);
                info = &clientInfo[0];
            }
            entry.pInfo = info;
            cache.push_back(std::move(entry));
        }
    }
}

void ClientManager::refresh(const std::shared_ptr<Config>& config)
{
    // table of supported clients (reverse search, sequence of entries matters!)
    clientInfo = {
        // Used for not explicitly listed clients, must be first entry
        {
            "Unknown",
            DEFAULT_CLIENT_GROUP,
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
            DEFAULT_CLIENT_GROUP,
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
            DEFAULT_CLIENT_GROUP,
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
            DEFAULT_CLIENT_GROUP,
            ClientType::SamsungSeriesCDE,
            QUIRK_FLAG_SAMSUNG | QUIRK_FLAG_SAMSUNG_FEATURES | QUIRK_FLAG_SAMSUNG_HIDE_DYNAMIC,
            ClientMatchType::UserAgent,
            "SEC_HHP_",
        },

        // This is AllShare running on a PC. We don't want to respond with Samsung capabilities, or Windows (and AllShare) might get grumpy.
        // User-Agent(actionReq): DLNADOC/1.50 SEC_HHP_[PC]LPC001/1.0  MS-DeviceCaps/1024
        {
            "AllShare",
            DEFAULT_CLIENT_GROUP,
            ClientType::SamsungAllShare,
            QUIRK_FLAG_NONE,
            ClientMatchType::UserAgent,
            "SEC_HHP_[PC]",
        },

        // User-Agent(actionReq): DLNADOC/1.50 SEC_HHP_[TV] Samsung Q7 Series (49)/1.0
        {
            "Samsung Series [Q] TVs",
            DEFAULT_CLIENT_GROUP,
            ClientType::SamsungSeriesQ,
            QUIRK_FLAG_SAMSUNG | QUIRK_FLAG_SAMSUNG_FEATURES | QUIRK_FLAG_SAMSUNG_HIDE_DYNAMIC,
            ClientMatchType::UserAgent,
            "SEC_HHP_[TV] Samsung Q",
        },

        // User-Agent(actionReq): DLNADOC/1.50 SEC_HHP_BD-D5100/1.0
        {
            "Samsung Blu-ray Player BD-D5100",
            DEFAULT_CLIENT_GROUP,
            ClientType::SamsungBDP,
            QUIRK_FLAG_SAMSUNG | QUIRK_FLAG_SAMSUNG_FEATURES,
            ClientMatchType::UserAgent,
            "SEC_HHP_BD",
        },

        // User-Agent: ?
        {
            "Samsung Blu-ray Player J5500",
            DEFAULT_CLIENT_GROUP,
            ClientType::SamsungBDJ5500,
            QUIRK_FLAG_SAMSUNG | QUIRK_FLAG_SAMSUNG_FEATURES,
            ClientMatchType::UserAgent,
            "[BD]J5500",
        },

        // User-Agent: ?
        {
            "Dual CR 510",
            DEFAULT_CLIENT_GROUP,
            ClientType::IRadio,
            QUIRK_FLAG_IRADIO,
            ClientMatchType::UserAgent,
            "EC-IRADIO/1.0",
        },
    };

    auto clientConfigList = config->getClientConfigListOption(CFG_CLIENTS_LIST);
    for (std::size_t i = 0; i < clientConfigList->size(); i++) {
        auto clientConfig = clientConfigList->get(i);
        clientInfo.push_back(clientConfig->getClientInfo());
    }
}

void ClientManager::addClientByDiscovery(const struct sockaddr_storage* addr, const std::string& userAgent, const std::string& descLocation)
{
#if 0 // only needed if UserAgent is not good enough
    const ClientInfo* info = nullptr;

    auto descXml = downloadDescription(descLocation);
    if (descXml) {
        // TODO: search for FriendlyName + ModelName
        //(void)getInfoByType(userAgent, ClientMatchType::FriendlyName, &info);
    }
#endif
}

const ClientInfo* ClientManager::getInfo(const struct sockaddr_storage* addr, const std::string& userAgent)
{
    // 1. by IP address
    auto info = getInfoByAddr(addr);
    if (!info) {
        // 2. by User-Agent
        info = getInfoByType(userAgent, ClientMatchType::UserAgent);
    }

    // update IP or User-Agent match in cache
    if (info) {
        updateCache(addr, userAgent, info);
    } else {
        // 3. by cache
        // HINT: most clients do not report exactly the same User-Agent for UPnP services and file request.
        info = getInfoByCache(addr);
    }

    if (!info) {
        // always return something, 'Unknown' if we do not know better
        assert(clientInfo[0].type == ClientType::Unknown);
        info = &clientInfo[0];

        // also add to cache, for web-ui proposes only
        updateCache(addr, userAgent, info);
    }

    log_debug("client info: {} '{}' -> '{}' as {}", sockAddrGetNameInfo(reinterpret_cast<const struct sockaddr*>(addr)), userAgent, info->name, ClientConfig::mapClientType(info->type));
    return info;
}

const ClientInfo* ClientManager::getInfoByAddr(const struct sockaddr_storage* addr) const
{
    auto it = std::find_if(clientInfo.begin(), clientInfo.end(), [=](auto&& c) {
        if (c.matchType != ClientMatchType::IP)
            return false;

        if (c.match.find(':') != std::string::npos) {
            // IPv6
            struct sockaddr_in6 clientAddr = {};
            clientAddr.sin6_family = AF_INET6;
            if ((inet_pton(AF_INET6, c.match.c_str(), &clientAddr.sin6_addr) == 1) && (sockAddrCmpAddr(reinterpret_cast<const struct sockaddr*>(&clientAddr), reinterpret_cast<const struct sockaddr*>(addr)) == 0))
                return true;
        } else if (c.match.find('.') != std::string::npos) {
            // IPv4
            struct sockaddr_in clientAddr = {};
            clientAddr.sin_family = AF_INET;
            clientAddr.sin_addr.s_addr = inet_addr(c.match.c_str());
            if (sockAddrCmpAddr(reinterpret_cast<const struct sockaddr*>(&clientAddr), reinterpret_cast<const struct sockaddr*>(addr)) == 0)
                return true;
        }

        return false;
    });

    if (it != clientInfo.end()) {
        auto ip = getHostName(reinterpret_cast<const struct sockaddr*>(addr));
        log_debug("found client by IP (ip='{}')", ip);
        return &(*it);
    }

    return nullptr;
}

const ClientInfo* ClientManager::getInfoByType(const std::string& match, ClientMatchType type) const
{
    if (!match.empty()) {
        auto it = std::find_if(clientInfo.rbegin(), clientInfo.rend(), [=](auto&& c) { return c.matchType == type && match.find(c.match) != std::string::npos; });
        if (it != clientInfo.rend()) {
            log_debug("found client by type (match='{}')", match);
            return &(*it);
        }
    }

    return nullptr;
}

const ClientInfo* ClientManager::getInfoByCache(const struct sockaddr_storage* addr) const
{
    AutoLock lock(mutex);

    auto it = std::find_if(cache.begin(), cache.end(), [=](auto&& entry) //
        { return sockAddrCmpAddr(reinterpret_cast<const struct sockaddr*>(&entry.addr), reinterpret_cast<const struct sockaddr*>(addr)) == 0; });

    if (it != cache.end()) {
        auto hostName = getHostName(reinterpret_cast<const struct sockaddr*>(&it->addr));
        log_debug("found client by cache (hostname='{}')", hostName);
        return it->pInfo;
    }

    return nullptr;
}

void ClientManager::updateCache(const struct sockaddr_storage* addr, const std::string& userAgent, const ClientInfo* pInfo)
{
    AutoLock lock(mutex);

    // house cleaning, remove old entries
    auto now = currentTime();
    cache.erase(std::remove_if(cache.begin(), cache.end(),
                    [this, now](auto&& entry) { return entry.last + cacheThreshold < now; }),
        cache.end());

    auto it = std::find_if(cache.begin(), cache.end(), [=](auto&& entry) //
        { return sockAddrCmpAddr(reinterpret_cast<const struct sockaddr*>(&entry.addr), reinterpret_cast<const struct sockaddr*>(addr)) == 0; });

    if (it != cache.end()) {
        it->last = now;
        if (it->pInfo != pInfo) {
            // client info changed, update all
            it->age = now;
            it->userAgent = userAgent;
            it->pInfo = pInfo;
        }
    } else {
        // add new client
        cache.emplace_back(*addr, userAgent, now, now, pInfo);
    }
    if (this->database)
        this->database->saveClients(cache);
}

std::unique_ptr<pugi::xml_document> ClientManager::downloadDescription(const std::string& location)
{
    auto xml = std::make_unique<pugi::xml_document>();
#if defined(USING_NPUPNP)
    std::string description, ct;
    int errCode = UpnpDownloadUrlItem(location, description, ct);
    if (errCode != UPNP_E_SUCCESS) {
        log_debug("Error obtaining client description from {} -- error = {}", location, errCode);
        return xml;
    }
    auto ret = xml->load_string(description.c_str());
#else
    IXML_Document* descDoc = nullptr;
    int errCode = UpnpDownloadXmlDoc(location.c_str(), &descDoc);
    if (errCode != UPNP_E_SUCCESS) {
        log_debug("Error obtaining client description from {} -- error = {}", location, errCode);
        return xml;
    }

    DOMString cxml = ixmlPrintDocument(descDoc);
    auto ret = xml->load_string(cxml);

    ixmlFreeDOMString(cxml);
    ixmlDocument_free(descDoc);
#endif

    if (ret.status != pugi::xml_parse_status::status_ok) {
        log_debug("Unable to parse xml client description from {}", location);
    }

    return xml;
}
