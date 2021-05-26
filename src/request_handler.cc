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

#include <numeric>
#include <vector>

#include "content/content_manager.h"
#include "database/database.h"
#include "util/tools.h"

RequestHandler::RequestHandler(std::shared_ptr<ContentManager> content)
    : content(std::move(content))
    , context(this->content->getContext())
    , config(this->context->getConfig())
    , mime(this->context->getMime())
    , database(this->context->getDatabase())
    , server(this->context->getServer())
{
}

void RequestHandler::splitUrl(const char* url, char separator, std::string& path, std::string& parameters)
{
    const auto url_s = std::string { url };
    const auto i1 = size_t { [=]() {
        if (separator == '/')
            return url_s.rfind(separator);
        if (separator == '?')
            return url_s.find(separator);
        throw_std_runtime_error("Forbidden separator: {}", separator);
    }() };

    if (i1 == std::string::npos) {
        path = url_s;
        parameters.clear();
    } else {
        parameters = url_s.substr(i1 + 1);
        path = url_s.substr(0, i1);
    }
}

std::string RequestHandler::joinUrl(const std::vector<std::string>& components, bool addToEnd, const std::string& separator)
{
    return !components.empty()
        ? (std::accumulate(std::next(components.begin()), components.end(), separator + components[0], [=](auto&& a, auto&& b) { return a + separator + b; }) + (addToEnd ? separator : ""))
        : "/";
}

std::map<std::string, std::string> RequestHandler::parseParameters(const char* filename, const char* baseLink)
{
    std::map<std::string, std::string> params;

    const auto parameters = std::string(filename + strlen(baseLink));
    dictDecodeSimple(parameters, &params);
    log_debug("filename: {} -> parameters: {}", filename, parameters.c_str());

    return params;
}

std::shared_ptr<CdsObject> RequestHandler::getObjectById(std::map<std::string, std::string> params)
{
    auto it = params.find("object_id");
    if (it == params.end()) {
        //log_error("object_id not found in url");
        throw_std_runtime_error("getInfo: object_id not found");
    }

    int objectID = std::stoi(it->second);
    //log_debug("load objectID: {}", objectID);
    return database->loadObject(objectID);
}
