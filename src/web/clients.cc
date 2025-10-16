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

/// @file web/clients.cc
#define GRB_LOG_FAC GrbLogFacility::web

#include "pages.h" // API

#include "config/result/client_config.h"
#include "content/content.h"
#include "context.h"
#include "database/database.h"
#include "upnp/client_manager.h"
#include "upnp/clients.h"
#include "upnp/headers.h"
#include "util/grb_net.h"

static std::string secondsToString(const std::chrono::seconds& t)
{
    return grbLocaltime("{:%a %b %d %H:%M:%S %Y}", t);
}

const std::string_view Web::Clients::PAGE = "clients";

bool Web::Clients::processPageAction(Json::Value& element, const std::string& action)
{
    // Delete client
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
    Json::Value clients(Json::arrayValue);

    auto&& clientArr = content->getContext()->getClients()->getClientList();
    for (auto&& obj : clientArr) {
        Json::Value item;
        item["ip"] = obj.addr->getNameInfo();
        item["host"] = obj.addr->getHostName();
        item["time"] = secondsToString(obj.age);
        item["last"] = secondsToString(obj.last);
        item["userAgent"] = obj.userAgent;
        item["name"] = obj.pInfo->name;
        item["group"] = obj.pInfo->group;
        item["match"] = obj.pInfo->match;
        item["allowed"] = obj.pInfo->isAllowed;
        auto flags = ClientConfig::mapFlags(obj.pInfo->flags);
        replaceAllString(flags, "|", " | ");
        item["flags"] = flags;
        item["matchType"] = ClientConfig::mapMatchType(obj.pInfo->matchType).data();
        item["clientType"] = ClientConfig::mapClientType(obj.pInfo->type).data();
        if (obj.headers) {
            Json::Value headers(Json::arrayValue);
            for (auto&& [key, value] : obj.headers->getHeaders()) {
                Json::Value header;
                header["key"] = key;
                header["value"] = value;
                headers.append(header);
            }
            item["headers"] = headers;
        }
        clients.append(item);
    }
    element["clients"] = clients;

    // Return current list of groups
    Json::Value groups(Json::arrayValue);

    auto&& groupArr = content->getContext()->getDatabase()->getClientGroupStats();
    for (auto&& obj : groupArr) {
        Json::Value item;
        item["name"] = obj.at("name");
        item["count"] = obj.at("count");
        item["playCount"] = obj.at("playCount");
        item["bookmarks"] = obj.at("bookmarks");
        item["last"] = obj.at("last");
        groups.append(item);
    }
    element["groups"] = groups;

    return true;
}
