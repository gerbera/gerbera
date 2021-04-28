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

#include "content/content_manager.h"
#include "util/upnp_clients.h"
#include "util/upnp_quirks.h"

ClientConfig::ClientConfig()
    : clientInfo(std::make_shared<ClientInfo>())
{
    clientInfo->matchType = ClientMatchType::None;
    clientInfo->type = ClientType::Unknown;
    clientInfo->flags = 0;
}

ClientConfig::ClientConfig(int flags, const std::string& ip, const std::string& userAgent)
    : clientInfo(std::make_shared<ClientInfo>())
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
    if (index == std::numeric_limits<std::size_t>::max()) {
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
        auto&& client = indexMap[id];
        auto entry = std::find_if(list.begin(), list.end(), [ip = client->getIp(), user = client->getUserAgent()](auto&& item) { return ip == item->getIp() && user == item->getUserAgent(); });
        list.erase(entry);
        if (id >= origSize) {
            indexMap.erase(id);
        }
        log_debug("ID {} removed!", id);
    }
}

std::string_view ClientConfig::mapClientType(ClientType clientType)
{
    switch (clientType) {
    case ClientType::Unknown:
        return "None";
    case ClientType::BubbleUPnP:
        return "BubbleUPnP";
    case ClientType::SamsungAllShare:
        return "SamsungAllShare";
    case ClientType::SamsungSeriesQ:
        return "SamsungSeriesQ";
    case ClientType::SamsungBDP:
        return "SamsungBDP";
    case ClientType::SamsungSeriesCDE:
        return "SamsungSeriesCDE";
    case ClientType::SamsungBDJ5500:
        return "SamsungBDJ5500";
    case ClientType::StandardUPnP:
        return "StandardUPnP";
    }
    throw_std_runtime_error("illegal clientType given to mapClientType()");
}

std::string_view ClientConfig::mapMatchType(ClientMatchType matchType)
{
    switch (matchType) {
    case ClientMatchType::None:
        return "None";
    case ClientMatchType::UserAgent:
        return "UserAgent";
    case ClientMatchType::IP:
        return "IP";
    }
    throw_std_runtime_error("illegal matchType given to mapMatchType()");
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
    if (clientType == "None") {
        return ClientType::Unknown;
    }

    throw_std_runtime_error("clientType {} invalid", clientType.c_str());
}

int ClientConfig::remapFlag(const std::string& flag)
{
    if (flag == "SAMSUNG") {
        return QUIRK_FLAG_SAMSUNG;
    }
    if (flag == "SAMSUNG_BOOKMARK_SEC") {
        return QUIRK_FLAG_SAMSUNG_BOOKMARK_SEC;
    }
    if (flag == "SAMSUNG_BOOKMARK_MSEC") {
        return QUIRK_FLAG_SAMSUNG_BOOKMARK_MSEC;
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
    if (flags & QUIRK_FLAG_SAMSUNG_BOOKMARK_SEC) {
        myFlags.emplace_back("SAMSUNG_BOOKMARK_SEC");
        flags &= ~QUIRK_FLAG_SAMSUNG_BOOKMARK_SEC;
    }
    if (flags & QUIRK_FLAG_SAMSUNG_BOOKMARK_MSEC) {
        myFlags.emplace_back("SAMSUNG_BOOKMARK_MSEC");
        flags &= ~QUIRK_FLAG_SAMSUNG_BOOKMARK_MSEC;
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
