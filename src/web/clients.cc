/*GRB*

    Gerbera - https://gerbera.io/
    
    upnp_clients.cc - this file is part of Gerbera.
    
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

/// \file clients.cc

#include "pages.h" // API

#include "config/client_config.h"
#include "content/content_manager.h"
#include "context.h"
#include "database/database.h"
#include "upnp_xml.h"
#include "util/upnp_clients.h"

#include <fmt/chrono.h>

web::clients::clients(std::shared_ptr<ContentManager> content)
    : WebRequestHandler(std::move(content))
{
}

static std::string steady_clock_to_string(std::chrono::steady_clock::time_point t)
{
    auto systime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()
        + std::chrono::duration_cast<std::chrono::system_clock::duration>(t - std::chrono::steady_clock::now()));
    return fmt::format("{:%a %b %d %H:%M:%S %Y}", fmt::localtime(systime));
}

void web::clients::process()
{
    check_request();
    auto root = xmlDoc->document_element();

    auto clients = root.append_child("clients");
    xml2JsonHints->setArrayName(clients, "client");

    auto arr = content->getContext()->getClients()->getClientList();
    for (auto&& obj : *arr) {
        auto item = clients.append_child("client");
        auto ip = sockAddrGetNameInfo(reinterpret_cast<const struct sockaddr*>(&obj.addr));
        item.append_attribute("ip") = ip.c_str();
        auto hostName = getHostName(reinterpret_cast<const struct sockaddr*>(&obj.addr));
        item.append_attribute("host") = hostName.c_str();
        item.append_attribute("time") = steady_clock_to_string(obj.age).c_str();
        item.append_attribute("last") = steady_clock_to_string(obj.last).c_str();
        item.append_attribute("userAgent") = obj.userAgent.c_str();
        item.append_attribute("name") = obj.pInfo->name.c_str();
        item.append_attribute("match") = obj.pInfo->match.c_str();
        item.append_attribute("flags") = ClientConfig::mapFlags(obj.pInfo->flags).c_str();
        item.append_attribute("matchType") = ClientConfig::mapMatchType(obj.pInfo->matchType).c_str();
        item.append_attribute("clientType") = ClientConfig::mapClientType(obj.pInfo->type).c_str();
    }
}
