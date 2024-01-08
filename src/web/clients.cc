/*GRB*

    Gerbera - https://gerbera.io/

    upnp_clients.cc - this file is part of Gerbera.

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

/// \file clients.cc
#define LOG_FAC log_facility_t::web

#include "pages.h" // API

#include "config/client_config.h"
#include "content/content_manager.h"
#include "context.h"
#include "database/database.h"
#include "upnp_xml.h"
#include "util/grb_net.h"
#include "util/upnp_clients.h"
#include "util/xml_to_json.h"

#include <fmt/chrono.h>

static std::string secondsToString(const std::chrono::seconds& t)
{
    return fmt::format("{:%a %b %d %H:%M:%S %Y}", fmt::localtime(t.count()));
}

void Web::Clients::process()
{
    checkRequest();
    auto root = xmlDoc->document_element();

    auto clients = root.append_child("clients");
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

    auto groups = root.append_child("groups");
    xml2Json->setArrayName(groups, "group");
    auto&& groupArr = content->getContext()->getDatabase()->getClientGroupStats();
    for (auto&& obj : groupArr) {
        auto item = groups.append_child("group");
        item.append_attribute("name") = obj.at("name").c_str();
        item.append_attribute("count") = obj.at("count").c_str();
        item.append_attribute("playCount") = obj.at("playCount").c_str();
        item.append_attribute("bookmarks") = obj.at("bookmarks").c_str();
        item.append_attribute("last") = obj.at("last").c_str(); // obj.last).c_str();
    }
}
