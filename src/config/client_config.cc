/*GRB*

    Gerbera - https://gerbera.io/

    client_config.cc - this file is part of Gerbera.

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

/// \file client_config.cc

#include "client_config.h" // API
#include "content_manager.h"
#include "util/upnp_clients.h"
#include "util/upnp_quirks.h"

#include <utility>

ClientConfig::ClientConfig()
    : clientInfo(std::make_shared<struct ClientInfo>())
{
    clientInfo->matchType = ClientMatchType::None;
    clientInfo->type = ClientType::Unknown;
    clientInfo->flags = 0;
}

ClientConfig::ClientConfig(int flags, const std::string& ip, const std::string& userAgent)
    : clientInfo(std::make_shared<struct ClientInfo>())
{
    clientInfo->type = ClientType::Unknown;
    if (!ip.empty()) {
        clientInfo->matchType = ClientMatchType::IP;
        clientInfo->match = ip;
    } else if (!userAgent.empty()) {
        clientInfo->matchType = ClientMatchType::UserAgent;
        clientInfo->match = userAgent;
    } else {
        clientInfo->matchType = ClientMatchType::None;
    }
    clientInfo->flags = flags;
    clientInfo->name = fmt::format("Manual Setup for{}{}", !ip.empty() ? " IP " + ip : "", !userAgent.empty() ? " UserAgent " + userAgent : "");
}

void ClientConfigList::add(const std::shared_ptr<ClientConfig>& client, size_t index)
{
    AutoLock lock(mutex);
    _add(client, index);
}

void ClientConfigList::_add(const std::shared_ptr<ClientConfig>& client, size_t index)
{
    if (index == SIZE_MAX) {
        index = getEditSize();
        origSize = list.size() + 1;
        client->setOrig(true);
    }
    list.push_back(client);
    indexMap[index] = client;
}

size_t ClientConfigList::getEditSize() const
{
    if (indexMap.empty()) {
        return 0;
    }
    return (*std::max_element(indexMap.begin(), indexMap.end(), [&](auto a, auto b) { return (a.first < b.first); })).first + 1;
}

std::vector<std::shared_ptr<ClientConfig>> ClientConfigList::getArrayCopy()
{
    AutoLock lock(mutex);
    return list;
}

std::shared_ptr<ClientConfig> ClientConfigList::get(size_t id, bool edit)
{
    AutoLock lock(mutex);
    if (!edit) {
        if (id >= list.size())
            return nullptr;

        return list[id];
    }
    if (indexMap.find(id) != indexMap.end()) {
        return indexMap[id];
    }
    return nullptr;
}

void ClientConfigList::remove(size_t id, bool edit)
{
    AutoLock lock(mutex);

    if (!edit) {
        if (id >= list.size()) {
            log_debug("No such ID {}!", id);
            return;
        }

        list.erase(list.begin() + id);
        log_debug("ID {} removed!", id);
    } else {
        if (indexMap.find(id) == indexMap.end()) {
            log_debug("No such index ID {}!", id);
            return;
        }
        const auto& client = indexMap[id];
        auto entry = std::find_if(list.begin(), list.end(), [=](const auto& item) { return client->getIp() == item->getIp() && client->getUserAgent() == item->getUserAgent(); });
        list.erase(entry);
        if (id >= origSize) {
            indexMap.erase(id);
        }
        log_debug("ID {} removed!", id);
    }
}

std::string ClientConfig::mapClientType(ClientType clientType)
{
    std::string clientType_str;
    switch (clientType) {
    case ClientType::Unknown:
        clientType_str = "None";
        break;
    case ClientType::BubbleUPnP:
        clientType_str = "BubbleUPnP";
        break;
    case ClientType::SamsungAllShare:
        clientType_str = "SamsungAllShare";
        break;
    case ClientType::SamsungSeriesQ:
        clientType_str = "SamsungSeriesQ";
        break;
    case ClientType::SamsungBDP:
        clientType_str = "SamsungBDP";
        break;
    case ClientType::SamsungSeriesCDE:
        clientType_str = "SamsungSeriesCDE";
        break;
    case ClientType::SamsungBDJ5500:
        clientType_str = "SamsungBDJ5500";
        break;
    case ClientType::VLC:
        clientType_str = "VLC";
        break;
    case ClientType::StandardUPnP:
        clientType_str = "StandardUPnP";
        break;
    default:
        throw_std_runtime_error("illegal clientType given to mapClientType()");
    }
    return clientType_str;
}

std::string ClientConfig::mapMatchType(ClientMatchType matchType)
{
    std::string matchType_str;
    switch (matchType) {
    case ClientMatchType::None:
        matchType_str = "None";
        break;
    case ClientMatchType::UserAgent:
        matchType_str = "UserAgent";
        break;
    case ClientMatchType::IP:
        matchType_str = "IP";
        break;
    default:
        throw_std_runtime_error("illegal matchType given to mapMatchType()");
    }
    return matchType_str;
}

ClientType ClientConfig::remapClientType(const std::string& clientType)
{
    if (clientType == "BubbleUPnP") {
        return ClientType::BubbleUPnP;
    }
    if (clientType == "SamsungAllShare") {
        return ClientType::SamsungAllShare;
    }
    if (clientType == "SamsungSeriesQ") {
        return ClientType::SamsungSeriesQ;
    }
    if (clientType == "SamsungBDP") {
        return ClientType::SamsungBDP;
    }
    if (clientType == "SamsungSeriesCDE") {
        return ClientType::SamsungSeriesCDE;
    }
    if (clientType == "SamsungBDJ5500") {
        return ClientType::SamsungBDJ5500;
    }
    if (clientType == "StandardUPnP") {
        return ClientType::StandardUPnP;
    }
    if (clientType == "VLC") {
        return ClientType::VLC;
    }
    if (clientType == "None") {
        return ClientType::Unknown;
    }

    throw_std_runtime_error("clientType " + clientType + " invalid");
}

int ClientConfig::remapFlag(const std::string& flag)
{
    if (flag == "SAMSUNG") {
        return QUIRK_FLAG_SAMSUNG;
    }
    return stoiString(flag, 0, 0);
}

std::string ClientConfig::mapFlags(QuirkFlags flags)
{
    if (!flags)
        return "None";

    std::vector<std::string> myFlags;

    if (flags & QUIRK_FLAG_SAMSUNG) {
        myFlags.emplace_back("SAMSUNG");
        flags &= ~QUIRK_FLAG_SAMSUNG;
    }

    if (flags) {
        myFlags.push_back(fmt::format("{:#04x}", flags));
    }

    return join(myFlags, '|');
}

void ClientConfig::copyTo(const std::shared_ptr<ClientConfig>& copy) const
{
    copy->clientInfo->name = clientInfo->name;
    copy->clientInfo->type = clientInfo->type;
    copy->clientInfo->flags = clientInfo->flags;
    copy->clientInfo->matchType = clientInfo->matchType;
    copy->clientInfo->match = clientInfo->match;
}
