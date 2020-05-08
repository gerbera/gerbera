/*GRB*

    Gerbera - https://gerbera.io/
    
    upnp_clients.cc - this file is part of Gerbera.
    
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

/// \file clients.cc

#include "pages.h" // API

#include "config/client_config.h"
#include "storage/storage.h"
#include "upnp_xml.h"
#include "util/upnp_clients.h"

#include <utility>

web::clients::clients(std::shared_ptr<Config> config, std::shared_ptr<Storage> storage,
    std::shared_ptr<ContentManager> content, std::shared_ptr<SessionManager> sessionManager)
    : WebRequestHandler(std::move(config), std::move(storage), std::move(content), std::move(sessionManager))
{
}

void web::clients::process()
{
    check_request();
    auto root = xmlDoc->document_element();

    auto clients = root.append_child("clients");
    xml2JsonHints->setArrayName(clients, "client");

    auto arr = Clients::getClientList();

    for (const auto obj : *arr) {
        auto item = clients.append_child("client");
        item.append_attribute("ip") = sockAddrGetNameInfo((const struct sockaddr*)&obj.addr).c_str();

        // item.append_attribute("age") = fmt::format("{}",obj.age);
        item.append_attribute("userAgent") = obj.userAgent.c_str();
        item.append_attribute("name") = obj.pInfo->name.c_str();
        item.append_attribute("match") = obj.pInfo->match.c_str();
        item.append_attribute("flags") = obj.pInfo->flags;
        item.append_attribute("matchType") = ClientConfig::mapMatchType(obj.pInfo->matchType).c_str();
        item.append_attribute("clientType") = ClientConfig::mapClientType(obj.pInfo->type).c_str();
    }
}
