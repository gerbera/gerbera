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

#include <utility>

#include "content_manager.h"
#include "database/database.h"
#include "util/tools.h"

RequestHandler::RequestHandler(std::shared_ptr<ContentManager> content)
    : config(content->getConfig())
    , mime(content->getMime())
    , database(content->getDatabase())
    , content(std::move(content))
{
}

void RequestHandler::splitUrl(const char* url, char separator, std::string& path, std::string& parameters)
{
    size_t i1;

    std::string url_s = url;

    if (separator == '/')
        i1 = url_s.rfind(separator);
    else if (separator == '?')
        i1 = url_s.rfind(separator);
    else
        throw_std_runtime_error(std::string("Forbidden separator: ") + separator);

    if (i1 == std::string::npos) {
        path = url_s;
        parameters = "";
    } else {
        parameters = url_s.substr(i1 + 1);
        path = url_s.substr(0, i1);
    }
}

std::map<std::string, std::string> RequestHandler::parseParameters(const char* filename, const char* baseLink)
{
    std::map<std::string, std::string> params;

    std::string parameters = (filename + strlen(baseLink));
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
