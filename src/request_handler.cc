/*MT*

    MediaTomb - http://www.mediatomb.cc/

    request_handler.cc - this file is part of MediaTomb.

    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>

    Copyright (C) 2006-2010 Gena Batyan <bgeradz@mediatomb.cc>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>,
                            Leonhard Wimmer <leo@mediatomb.cc>

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
#define LOG_FAC log_facility_t::requests

#include "request_handler.h" // API

#include "content/content_manager.h"
#include "database/database.h"
#include "util/tools.h"

#include <fmt/core.h>

RequestHandler::RequestHandler(std::shared_ptr<ContentManager> content)
    : content(std::move(content))
    , context(this->content->getContext())
    , config(this->context->getConfig())
    , mime(this->context->getMime())
    , database(this->context->getDatabase())
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
