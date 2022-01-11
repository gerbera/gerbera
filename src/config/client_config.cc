/*GRB*

    Gerbera - https://gerbera.io/

    client_config.cc - this file is part of Gerbera.

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

/// \file client_config.cc

#include "client_config.h" // API

#include <numeric>

#include "content/content_manager.h"
#include "util/upnp_clients.h"
#include "util/upnp_quirks.h"

ClientConfig::ClientConfig()
    : clientInfo(std::make_unique<ClientInfo>())
{
}

ClientConfig::ClientConfig(int flags, std::string_view ip, std::string_view userAgent, int captionInfoCount)
{
    auto cInfo = ClientInfo();
    cInfo.type = ClientType::Unknown;
    if (!ip.empty()) {
        cInfo.matchType = ClientMatchType::IP;
        cInfo.match = ip;
    } else if (!userAgent.empty()) {
        cInfo.matchType = ClientMatchType::UserAgent;
        cInfo.match = userAgent;
    } else {
        cInfo.matchType = ClientMatchType::None;
    }
    cInfo.flags = flags;
    cInfo.captionInfoCount = captionInfoCount;
    auto sIP = ip.empty() ? "" : fmt::format(" IP {}", ip);
    auto sUA = userAgent.empty() ? "" : fmt::format(" UserAgent {}", userAgent);
    cInfo.name = fmt::format("Manual Setup for{}{}", sIP, sUA);
    clientInfo = std::make_unique<ClientInfo>(std::move(cInfo));
}

void ClientConfigList::add(const std::shared_ptr<ClientConfig>& client, std::size_t index)
{
    AutoLock lock(mutex);
    _add(client, index);
}

void ClientConfigList::_add(const std::shared_ptr<ClientConfig>& client, std::size_t index)
{
    if (index == std::numeric_limits<std::size_t>::max()) {
        index = getEditSize();
        origSize = list.size() + 1;
        client->setOrig(true);
    }
    list.push_back(client);
    indexMap[index] = client;
}

std::size_t ClientConfigList::getEditSize() const
{
    if (indexMap.empty()) {
        return 0;
    }
    return std::max_element(indexMap.begin(), indexMap.end(), [](auto a, auto b) { return (a.first < b.first); })->first + 1;
}

std::vector<std::shared_ptr<ClientConfig>> ClientConfigList::getArrayCopy() const
{
    AutoLock lock(mutex);
    return list;
}

std::shared_ptr<ClientConfig> ClientConfigList::get(std::size_t id, bool edit) const
{
    AutoLock lock(mutex);
    if (!edit) {
        if (id >= list.size())
            return nullptr;

        return list[id];
    }
    if (indexMap.find(id) != indexMap.end()) {
        return indexMap.at(id);
    }
    return nullptr;
}

void ClientConfigList::remove(std::size_t id, bool edit)
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
    case ClientType::IRadio:
        return "EC-IRadio";
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
    if (flag == "SAMSUNG_FEATURES") {
        return QUIRK_FLAG_SAMSUNG_FEATURES;
    }
    if (flag == "SAMSUNG_HIDE_DYNAMIC") {
        return QUIRK_FLAG_SAMSUNG_HIDE_DYNAMIC;
    }
    if (flag == "IRADIO") {
        return QUIRK_FLAG_IRADIO;
    }

    return stoiString(flag, 0, 0);
}

int ClientConfig::makeFlags(const std::string& optValue)
{
    std::vector<std::string> flagsVector = splitString(optValue, '|', false);
    return std::accumulate(flagsVector.begin(), flagsVector.end(), 0, [](auto flg, auto&& i) { return flg | ClientConfig::remapFlag(i); });
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
    if (flags & QUIRK_FLAG_SAMSUNG_FEATURES) {
        myFlags.emplace_back("SAMSUNG_FEATURES");
        flags &= ~QUIRK_FLAG_SAMSUNG_FEATURES;
    }
    if (flags & QUIRK_FLAG_SAMSUNG_HIDE_DYNAMIC) {
        myFlags.emplace_back("SAMSUNG_HIDE_DYNAMIC");
        flags &= ~QUIRK_FLAG_SAMSUNG_HIDE_DYNAMIC;
    }
    if (flags & QUIRK_FLAG_IRADIO) {
        myFlags.emplace_back("IRADIO");
        flags &= ~QUIRK_FLAG_IRADIO;
    }

    if (flags) {
        myFlags.push_back(fmt::format("{:#04x}", flags));
    }

    return fmt::format("{}", fmt::join(myFlags, "|"));
}
