/*MT*

    MediaTomb - http://www.mediatomb.cc/

    request_handler.cc - this file is part of MediaTomb.

    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>

    Copyright (C) 2006-2010 Gena Batyan <bgeradz@mediatomb.cc>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>,
                            Leonhard Wimmer <leo@mediatomb.cc>

    Copyright (C) 2016-2024 Gerbera Contributors

    MediaTomb is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.

    MediaTomb is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    version 2 along with MediaTomb; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.

    $Id$
*/

/// \file request_handler.cc
#define GRB_LOG_FAC GrbLogFacility::requests

#include "request_handler.h" // API

#include "content/content_manager.h"
#include "database/database.h"
#include "upnp/quirks.h"
#include "util/grb_net.h"
#include "util/tools.h"

#include <fmt/core.h>

RequestHandler::RequestHandler(std::shared_ptr<ContentManager> content, std::shared_ptr<UpnpXMLBuilder> xmlBuilder)
    : content(std::move(content))
    , context(this->content->getContext())
    , config(this->context->getConfig())
    , mime(this->context->getMime())
    , database(this->context->getDatabase())
    , xmlBuilder(std::move(xmlBuilder))
{
}

std::shared_ptr<CdsObject> RequestHandler::loadObject(const std::map<std::string, std::string>& params) const
{
    auto it = params.find("object_id");
    if (it == params.end()) {
        throw_std_runtime_error("loadObject: object_id not found");
    }
    int objectID = stoiString(it->second);
    it = params.find(CLIENT_GROUP_TAG);
    std::string group = DEFAULT_CLIENT_GROUP;
    if (it != params.end()) {
        group = it->second;
    }

    return database->loadObject(group, objectID);
}

std::shared_ptr<Quirks> RequestHandler::getQuirks(const UpnpFileInfo* info) const
{
    auto ctrlPtIPAddr = std::make_shared<GrbNet>(UpnpFileInfo_get_CtrlPtIPAddr(info));
    // HINT: most clients do not report exactly the same User-Agent for UPnP services and file request.
    std::string userAgent = UpnpFileInfo_get_Os_cstr(info);
    return std::make_shared<Quirks>(xmlBuilder, context->getClients(), ctrlPtIPAddr, std::move(userAgent));
}
