/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    add.cc - this file is part of MediaTomb.
    
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

/// \file add.cc

#include "common.h"
#include "config/config_manager.h"
#include "content_manager.h"
#include "pages.h"
#include "server.h"
#include "util/tools.h"
#include <cstdio>
#include <utility>

web::add::add(std::shared_ptr<ConfigManager> config, std::shared_ptr<Storage> storage,
    std::shared_ptr<ContentManager> content, std::shared_ptr<SessionManager> sessionManager)
    : WebRequestHandler(std::move(config), std::move(storage), std::move(content), std::move(sessionManager))
{
}

void web::add::process()
{
    log_debug("add: start");

    check_request();

    fs::path path;
    std::string objID = param("object_id");
    if (objID == "0")
        path = FS_ROOT_DIRECTORY;
    else
        path = hex_decode_string(objID);
    if (path.empty())
        throw std::runtime_error("web::add::process(): illegal path");

    content->addFile(path, true);
    log_debug("add: returning");
}
