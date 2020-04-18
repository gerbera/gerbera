/*GRB*

    Gerbera - https://gerbera.io/
    
    upnp_quirks.cc - this file is part of Gerbera.
    
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

/// \file upnp_quirks.cc

#include "upnp_quirks.h" // API

#include "cds_objects.h"
#include "config/config_manager.h"
#include "server.h"
#include "util/tools.h"
#include "util/upnp_clients.h"
#include "util/upnp_headers.h"

Quirks::Quirks(std::shared_ptr<ConfigManager> config, const struct sockaddr_storage* addr, const std::string& userAgent)
    : config(std::move(config))
{
    Clients::getInfo(addr, userAgent, &pClientInfo);
}

void Quirks::addCaptionInfo(std::shared_ptr<CdsItem> item, std::unique_ptr<Headers>& headers)
{
    if ((pClientInfo->flags & QUIRK_FLAG_SAMSUNG) == 0)
        return;

    if (!startswith(item->getMimeType(), "video"))
        return;

    std::vector<std::string> exts {
        ".srt",
        ".ssa",
        ".smi",
        ".sub"
    };

    // remove .ext
    fs::path path = item->getLocation();
    std::string pathNoExt = path.parent_path() / path.stem();

    std::string validext;
    for (const auto& ext : exts) {
        std::string captionPath = pathNoExt + ext;
        if (access(captionPath.c_str(), R_OK) == 0) {
            validext = ext;
            break;
        }
    }

    if (validext.length() == 0)
        return;

    std::string url = "http://" + Server::getIP() + ":" + Server::getPort() + pathNoExt + validext;
    headers->addHeader("CaptionInfo.sec:", url);
}

void Quirks::appendSpecialNamespace(pugi::xml_node* didlLiteRoot)
{
    if ((pClientInfo->flags & QUIRK_FLAG_SAMSUNG) == 0)
        return;

    didlLiteRoot->append_attribute(XML_SEC_NAMESPACE_ATTR) = XML_SEC_NAMESPACE;
}
