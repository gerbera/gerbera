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
    , server(this->context->getServer())
{
}

std::tuple<std::string_view, std::string_view> RequestHandler::splitUrl(std::string_view url, char separator)
{
    std::size_t i1 = 0;
    switch (separator) {
    case '/':
        i1 = url.rfind('/');
        break;
    case '?':
        i1 = url.find('?');
        break;
    default:
        throw_std_runtime_error("Forbidden separator: {}", separator);
    }

    if (i1 == std::string::npos) {
        return { url, std::string_view() };
    } else {
        return { url.substr(0, i1), url.substr(i1 + 1) };
    }
}

std::string RequestHandler::joinUrl(const std::vector<std::string>& components, bool addToEnd, std::string_view separator)
{
    if (components.empty()) {
        return std::string(separator);
    } else {
        return fmt::format("{}{}{}", separator, fmt::join(components, separator), (addToEnd ? separator : ""));
    }
}

std::map<std::string, std::string> RequestHandler::parseParameters(std::string_view filename, std::string_view baseLink)
{
    const auto parameters = filename.substr(baseLink.size());
    //log_debug("filename: {} -> parameters: {}", filename, parameters);
    return dictDecodeSimple(parameters);
}

std::shared_ptr<CdsObject> RequestHandler::getObjectById(const std::map<std::string, std::string>& params) const
{
    auto it = params.find("object_id");
    if (it == params.end()) {
        //log_error("object_id not found in url");
        throw_std_runtime_error("getObjectById: object_id not found");
    }

    int objectID = std::stoi(it->second);
    //log_debug("load objectID: {}", objectID);
    return database->loadObject(objectID);
}
