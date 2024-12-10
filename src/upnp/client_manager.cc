/*GRB*

    Gerbera - https://gerbera.io/

    upnp/client_manager.cc - this file is part of Gerbera.

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

/// \file upnp/client_manager.cc
/// client info initially taken from https://sourceforge.net/p/minidlna/git/ci/master/tree/clients.cc
#define GRB_LOG_FAC GrbLogFacility::clients

#include "client_manager.h" // API

#include "clients.h"
#include "config/config.h"
#include "config/config_val.h"
#include "config/result/client_config.h"
#include "database/database.h"
#include "util/grb_net.h"
#include "util/logger.h"

#include <algorithm>
#include <sys/socket.h>
#include <upnp.h>

ClientManager::ClientManager(std::shared_ptr<Config> config, std::shared_ptr<Database> database)
    : database(std::move(database))
    , config(std::move(config))
    , cacheThreshold(this->config->getIntOption(ConfigVal::CLIENTS_CACHE_THRESHOLD))
{
    refresh();
    if (this->database) {
        auto dbCache = this->database->getClients();
        for (auto&& entry : dbCache) {
            auto info = getInfoByAddr(entry.addr);
            if (!info) {
                info = getInfoByType(entry.userAgent, ClientMatchType::UserAgent);
            }
            if (!info) {
                assert(clientProfile[0].type == ClientType::Unknown);
                info = clientProfile.data();
            }
            entry.pInfo = info;
            cache.push_back(std::move(entry));
        }
    }
}

void ClientManager::refresh()
{
    // table of supported clients (reverse search, sequence of entries matters!)
    clientProfile = {
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
            QUIRK_FLAG_SAMSUNG | QUIRK_FLAG_SAMSUNG_FEATURES | QUIRK_FLAG_SAMSUNG_HIDE_DYNAMIC | QUIRK_FLAG_DCM10,
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

        // User-Agent: FSL DLNADOC/1.50 UPnP Stack/1.0
        {
            "Technisat DigitRadio 580",
            DEFAULT_CLIENT_GROUP,
            ClientType::FSL,
            QUIRK_FLAG_SIMPLE_DATE,
            ClientMatchType::UserAgent,
            "FSL DLNADOC/1.50",
        },

        // User-Agent(actionReq): Panasonic MIL DLNA CP UPnP/1.0 DLNADOC/1.50
        {
            "Panasonic TV",
            DEFAULT_CLIENT_GROUP,
            ClientType::PanasonicTV,
            QUIRK_FLAG_PANASONIC,
            ClientMatchType::UserAgent,
            "Panasonic MIL DLNA CP",
        },

        // User-Agent(actionReq): Allegro-Software-WebClient/5.40b1 DLNADOC/1.50
        {
            "Bose Soundtouch",
            DEFAULT_CLIENT_GROUP,
            ClientType::BoseSoundtouch,
            QUIRK_FLAG_STRICTXML,
            ClientMatchType::UserAgent,
            "Allegro-Software-WebClient",
        },
    };

    auto configList = config->getClientConfigListOption(ConfigVal::CLIENTS_LIST);
    if (!configList)
        return;

    auto defaultGroup = configList->getGroup(DEFAULT_CLIENT_GROUP);
    if (defaultGroup) {
        for (auto&& cp : clientProfile) {
            cp.groupConfig = defaultGroup;
        }
    }
    auto clientConfigList = EDIT_CAST(EditHelperClientConfig, configList);
    for (std::size_t i = 0; i < clientConfigList->size(); i++) {
        auto clientConfig = clientConfigList->get(i);
        clientProfile.push_back(clientConfig->getClientProfile());
    }
}

static constexpr std::array matchTypes {
    std::pair(ClientMatchType::FriendlyName, "friendlyName"),
    std::pair(ClientMatchType::ModelName, "modelName"),
    std::pair(ClientMatchType::Manufacturer, "manufacturer"),
};

void ClientManager::addClientByDiscovery(const std::shared_ptr<GrbNet>& addr, const std::string& userAgent, const std::string& descLocation)
{
    const ClientObservation* client = getInfo(addr, userAgent);
    if (!client || (client->pInfo && client->pInfo->matchType == ClientMatchType::None)) {
        auto descXml = downloadDescription(descLocation);
        if (descXml) {
            pugi::xpath_node rootNode = descXml->document_element();
            if (rootNode.node() && std::string(rootNode.node().name()) == "root") {
                pugi::xpath_node deviceNode = rootNode.node().select_node("device");
                if (deviceNode && deviceNode.node()) {
                    for (auto&& [mType, mNode] : matchTypes) {
                        pugi::xpath_node deviceProp = deviceNode.node().select_node(mNode);
                        if (deviceProp && deviceProp.node()) {
                            auto info = getInfoByType(deviceProp.node().text().as_string(), mType);
                            if (info) {
                                updateCache(addr, userAgent, info);
                            }
                        }
                    }
                }
            }
        }
    }
}

const ClientObservation* ClientManager::getInfo(const std::shared_ptr<GrbNet>& addr, const std::string& userAgent) const
{
    // 1. by IP address
    auto info = getInfoByAddr(addr);
    if (!info) {
        // 2. by User-Agent
        info = getInfoByType(userAgent, ClientMatchType::UserAgent);
    }

    // update IP or User-Agent match in cache
    if (info) {
        return updateCache(addr, userAgent, info);
    }
    // 3. by cache
    // HINT: most clients do not report exactly the same User-Agent for UPnP services and file request.
    auto client = getInfoByCache(addr);

    if (client) {
        return client;
    }

    // always return something, 'Unknown' if we do not know better
    assert(clientProfile[0].type == ClientType::Unknown);
    info = clientProfile.data();

    // also add to cache, for web-ui proposes only
    return updateCache(addr, userAgent, info);
}

const ClientProfile* ClientManager::getInfoByAddr(const std::shared_ptr<GrbNet>& addr) const
{
    auto it = std::find_if(clientProfile.begin(), clientProfile.end(), [=](auto&& c) {
        if (c.matchType != ClientMatchType::IP)
            return false;
        return addr->equals(c.match);
    });

    if (it != clientProfile.end()) {
        log_debug("found client by IP (ip='{}')", addr->getHostName());
        return &(*it);
    }

    return nullptr;
}

const ClientProfile* ClientManager::getInfoByType(const std::string& match, ClientMatchType type) const
{
    if (!match.empty()) {
        auto it = std::find_if(clientProfile.rbegin(), clientProfile.rend(), [=](auto&& c) //
            { return c.matchType == type && match.find(c.match) != std::string::npos; });
        if (it != clientProfile.rend()) {
            log_debug("found client by type (match='{}')", match);
            return &(*it);
        }
    }

    return nullptr;
}

const ClientObservation* ClientManager::getInfoByCache(const std::shared_ptr<GrbNet>& addr) const
{
    AutoLock lock(mutex);

    auto it = std::find_if(cache.begin(), cache.end(), [=](auto&& entry) //
        { return entry.addr->equals(addr); });

    if (it != cache.end()) {
        log_debug("found client by cache (hostname='{}')", it->addr->getHostName());
        return &(*it);
    }

    return nullptr;
}

void ClientManager::removeClient(const std::string& clientIp)
{
    AutoLock lock(mutex);
    cache.erase(std::remove_if(cache.begin(), cache.end(),
                    [this, clientIp](auto&& entry) { return entry.addr && entry.addr->equals(clientIp); }),
        cache.end());
    if (this->database)
        this->database->saveClients(cache);
}

const ClientObservation* ClientManager::updateCache(const std::shared_ptr<GrbNet>& addr, const std::string& userAgent, const ClientProfile* pInfo) const
{
    AutoLock lock(mutex);

    // house cleaning, remove old entries
    auto now = currentTime();
    cache.erase(std::remove_if(cache.begin(), cache.end(),
                    [this, now](auto&& entry) { return entry.last + cacheThreshold < now; }),
        cache.end());

    auto it = std::find_if(cache.begin(), cache.end(), [=](auto&& entry) //
        { return entry.addr->equals(addr); });

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
        cache.emplace_back(addr, userAgent, now, now, pInfo);
        it = std::find_if(cache.begin(), cache.end(), [=](auto&& entry) //
            { return entry.addr->equals(addr); });
    }
    if (this->database)
        this->database->saveClients(cache);
    log_debug("client info: {} '{}' -> '{}' as {} with {}", addr->getNameInfo(), userAgent, pInfo->name, ClientConfig::mapClientType(pInfo->type), ClientConfig::mapFlags(pInfo->flags));
    return &(*it);
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
