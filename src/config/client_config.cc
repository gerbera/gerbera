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

#include <array>
#include <numeric>

#include "util/upnp_clients.h"
#include "util/upnp_quirks.h"

ClientConfig::ClientConfig(int flags, std::string_view group, std::string_view ip, std::string_view userAgent, int captionInfoCount, int stringLimit)
{
    clientInfo.type = ClientType::Unknown;
    if (!ip.empty()) {
        clientInfo.matchType = ClientMatchType::IP;
        clientInfo.match = ip;
    } else if (!userAgent.empty()) {
        clientInfo.matchType = ClientMatchType::UserAgent;
        clientInfo.match = userAgent;
    } else {
        clientInfo.matchType = ClientMatchType::None;
    }
    clientInfo.group = group;
    clientInfo.flags = flags;
    clientInfo.captionInfoCount = captionInfoCount;
    clientInfo.stringLimit = stringLimit;
    auto sIP = ip.empty() ? "" : fmt::format(" IP {}", ip);
    auto sUA = userAgent.empty() ? "" : fmt::format(" UserAgent {}", userAgent);
    clientInfo.name = fmt::format("Manual Setup for{}{}", sIP, sUA);
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

static constexpr auto clientTypes = std::array {
    std::pair("None", ClientType::Unknown),
    std::pair("BubbleUPnP", ClientType::BubbleUPnP),
    std::pair("SamsungAllShare", ClientType::SamsungAllShare),
    std::pair("SamsungSeriesQ", ClientType::SamsungSeriesQ),
    std::pair("SamsungBDP", ClientType::SamsungBDP),
    std::pair("SamsungSeriesCDE", ClientType::SamsungSeriesCDE),
    std::pair("SamsungBDJ5500", ClientType::SamsungBDJ5500),
    std::pair("EC-IRadio", ClientType::IRadio),
    std::pair("PanasonicTV", ClientType::PanasonicTV),
    std::pair("StandardUPnP", ClientType::StandardUPnP),
};

std::string_view ClientConfig::mapClientType(ClientType clientType)
{
    for (auto [cLabel, cType] : clientTypes) {
        if (clientType == cType) {
            return cLabel;
        }
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

static constexpr auto quirkFlags = std::array {
    std::pair("SAMSUNG", QUIRK_FLAG_SAMSUNG),
    std::pair("SAMSUNG_BOOKMARK_SEC", QUIRK_FLAG_SAMSUNG_BOOKMARK_SEC),
    std::pair("SAMSUNG_BOOKMARK_MSEC", QUIRK_FLAG_SAMSUNG_BOOKMARK_MSEC),
    std::pair("SAMSUNG_FEATURES", QUIRK_FLAG_SAMSUNG_FEATURES),
    std::pair("SAMSUNG_HIDE_DYNAMIC", QUIRK_FLAG_SAMSUNG_HIDE_DYNAMIC),
    std::pair("IRADIO", QUIRK_FLAG_IRADIO),
    std::pair("PV_SUBTITLES", QUIRK_FLAG_PV_SUBTITLES),
    std::pair("PANASONIC", QUIRK_FLAG_PANASONIC),
};

int ClientConfig::remapFlag(const std::string& flag)
{
    for (auto [qLabel, qFlag] : quirkFlags) {
        if (flag == qLabel) {
            return qFlag;
        }
    }
    return stoiString(flag, 0, 0);
}

int ClientConfig::makeFlags(const std::string& optValue)
{
    std::vector<std::string> flagsVector = splitString(optValue, '|', false);
    return std::accumulate(flagsVector.begin(), flagsVector.end(), 0, [](auto flg, auto&& i) { return flg | ClientConfig::remapFlag(trimString(i)); });
}

std::string ClientConfig::mapFlags(QuirkFlags flags)
{
    if (!flags)
        return "None";

    std::vector<std::string> myFlags;

    for (auto [qLabel, qFlag] : quirkFlags) {
        if (flags & qFlag) {
            myFlags.emplace_back(qLabel);
            flags &= ~qFlag;
        }
    }

    if (flags) {
        myFlags.push_back(fmt::format("{:#04x}", flags));
    }

    return fmt::format("{}", fmt::join(myFlags, " | "));
}
