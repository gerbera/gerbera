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

#include <utility>
#include "util/upnp_clients.h"
#include "content_manager.h"

ClientConfig::ClientConfig()
{
    clientInfo.flags = 0;
    ip = nullptr;
    userAgent = nullptr;
    clientInfo.type = ClientType::Unknown;
}

ClientConfig::ClientConfig(int flags, std::string ip, std::string userAgent, ClientType clientType)
    : ip(ip)
    , userAgent(userAgent)
{
    clientInfo.type = clientType;
    clientInfo.flags = flags;
    clientInfo.name = fmt::format("Manual Setup for{}{}", !ip.empty() ? " IP " + ip : "", !userAgent.empty() ? " UserAgent " + userAgent : "");
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

void ClientConfig::copyTo(const std::shared_ptr<ClientConfig>& copy) const
{
    copy->ip = ip;
    copy->userAgent = userAgent;
    copy->clientInfo.name = clientInfo.name;
    copy->clientInfo.type = clientInfo.type;
    copy->clientInfo.flags = clientInfo.flags;
    copy->clientInfo.matchType = clientInfo.matchType;
    copy->clientInfo.match = clientInfo.match;
}
