/*GRB*

    Gerbera - https://gerbera.io/
    
    client_config.h - this file is part of Gerbera.
    
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

/// \file client_config.cc

#include "client_config.h" // API
#include "content_manager.h"
#include "util/upnp_clients.h"

#include <utility>

ClientConfig::ClientConfig()
{
    clientInfo = std::make_shared<struct ClientInfo>();
    clientInfo->matchType = ClientMatchType::None;
    clientInfo->type = ClientType::Unknown;
    clientInfo->flags = 0;
}

ClientConfig::ClientConfig(int flags, std::string ip, std::string userAgent)
{
    clientInfo = std::make_shared<struct ClientInfo>();
    clientInfo->type = ClientType::StandardUPnP;
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

ClientConfigList::ClientConfigList()
{
}

void ClientConfigList::add(const std::shared_ptr<ClientConfig>& client)
{
    AutoLock lock(mutex);
    _add(client);
}

void ClientConfigList::_add(const std::shared_ptr<ClientConfig>& client)
{
    list.push_back(client);
}

std::vector<std::shared_ptr<ClientConfig>> ClientConfigList::getArrayCopy()
{
    AutoLock lock(mutex);
    return list;
}

std::shared_ptr<ClientConfig> ClientConfigList::get(size_t id)
{
    AutoLock lock(mutex);

    if (id >= list.size())
        return nullptr;

    return list[id];
}

void ClientConfigList::remove(size_t id)
{
    AutoLock lock(mutex);

    if (id >= list.size()) {
        log_debug("No such ID {}!", id);
        return;
    }

    list.erase(list.begin() + id);
    log_debug("ID {} removed!", id);
}

std::string ClientConfig::mapClientType(ClientType clientType)
{
    std::string clientType_str;
    switch (clientType) {
    case ClientType::Unknown:
        clientType_str = "Unknown";
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
    case ClientType::StandardUPnP:
        clientType_str = "StandardUPnP";
        break;
    default:
        throw_std_runtime_error("illegal clientType given to mapClientType()");
    }
    return clientType_str;
}

ClientType ClientConfig::remapClientType(const std::string& clientType)
{
    if (clientType == "BubbleUPnP") {
        return ClientType::BubbleUPnP;
    } else if (clientType == "SamsungAllShare") {
        return ClientType::SamsungAllShare;
    } else if (clientType == "SamsungSeriesQ") {
        return ClientType::SamsungSeriesQ;
    } else if (clientType == "SamsungBDP") {
        return ClientType::SamsungBDP;
    } else if (clientType == "SamsungSeriesCDE") {
        return ClientType::SamsungSeriesCDE;
    } else if (clientType == "SamsungBDJ5500") {
        return ClientType::SamsungBDJ5500;
    } else if (clientType == "StandardUPnP") {
        return ClientType::StandardUPnP;
    } else if (clientType == "Unknown") {
        return ClientType::Unknown;
    } else {
        throw_std_runtime_error("clientType " + clientType + " invalid");
    }
}

int ClientConfig::remapFlag(const std::string& flag)
{
    if (flag == "SAMSUNG") {
        return QUIRK_FLAG_SAMSUNG;
    } else {
        return stoi_string(flag);
    }
}

void ClientConfig::copyTo(const std::shared_ptr<ClientConfig>& copy) const
{
    copy->clientInfo->name = clientInfo->name;
    copy->clientInfo->type = clientInfo->type;
    copy->clientInfo->flags = clientInfo->flags;
    copy->clientInfo->matchType = clientInfo->matchType;
    copy->clientInfo->match = clientInfo->match;
}
