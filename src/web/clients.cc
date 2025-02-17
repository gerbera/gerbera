/*GRB*

    Gerbera - https://gerbera.io/

    web/clients.cc - this file is part of Gerbera.

    Copyright (C) 2020-2025 Gerbera Contributors

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

/// \file web/clients.cc
#define GRB_LOG_FAC GrbLogFacility::web

#include "pages.h" // API

#include "config/result/client_config.h"
#include "content/content.h"
#include "context.h"
#include "database/database.h"
#include "upnp/client_manager.h"
#include "upnp/clients.h"
#include "upnp/xml_builder.h"
#include "util/grb_net.h"
#include "util/xml_to_json.h"

#include <fmt/chrono.h>

static std::string secondsToString(const std::chrono::seconds& t)
{
    return fmt::format("{:%a %b %d %H:%M:%S %Y}", fmt::localtime(t.count()));
}

const std::string Web::Clients::PAGE = "clients";

void Web::Clients::processPageAction(pugi::xml_node& element)
{
    // Delete client
    std::string action = param("action");
    if (action == "delete") {
        std::string clientIp = param("client_id");
        if (!clientIp.empty()) {
            log_debug("Deleting client with {} IP address", clientIp);
            content->getContext()->getClients()->removeClient(clientIp);
        } else {
            log_warning("Cannot delete client without IP address");
        }
    }

    // Return current list of clients
    auto clients = element.append_child("clients");
    xml2Json->setArrayName(clients, "client");

    auto&& clientArr = content->getContext()->getClients()->getClientList();
    for (auto&& obj : clientArr) {
        auto item = clients.append_child("client");
        auto ip = obj.addr->getNameInfo();
        item.append_attribute("ip") = ip.c_str();
        auto hostName = obj.addr->getHostName();
        item.append_attribute("host") = hostName.c_str();
        item.append_attribute("time") = secondsToString(obj.age).c_str();
        item.append_attribute("last") = secondsToString(obj.last).c_str();
        item.append_attribute("userAgent") = obj.userAgent.c_str();
        item.append_attribute("name") = obj.pInfo->name.c_str();
        item.append_attribute("group") = obj.pInfo->group.c_str();
        item.append_attribute("match") = obj.pInfo->match.c_str();
        auto flags = ClientConfig::mapFlags(obj.pInfo->flags);
        replaceAllString(flags, "|", " | ");
        item.append_attribute("flags") = flags.c_str();
        item.append_attribute("matchType") = ClientConfig::mapMatchType(obj.pInfo->matchType).data();
        item.append_attribute("clientType") = ClientConfig::mapClientType(obj.pInfo->type).data();
    }

    // Return current list of groups
    auto groups = element.append_child("groups");
    xml2Json->setArrayName(groups, "group");
    auto&& groupArr = content->getContext()->getDatabase()->getClientGroupStats();
    for (auto&& obj : groupArr) {
        auto item = groups.append_child("group");
        item.append_attribute("name") = obj.at("name").c_str();
        item.append_attribute("count") = obj.at("count").c_str();
        item.append_attribute("playCount") = obj.at("playCount").c_str();
        item.append_attribute("bookmarks") = obj.at("bookmarks").c_str();
        item.append_attribute("last") = obj.at("last").c_str();
    }
}
