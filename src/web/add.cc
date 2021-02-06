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

#include <filesystem> // for path
#include <memory> // for allocator, shared_ptr, __shared...
#include <string> // for operator==, basic_string, string
#include <utility> // for move

#include "common.h" // for FS_ROOT_DIRECTORY
#include "config/directory_tweak.h" // for AutoScanSetting
#include "content/content_manager.h" // for ContentManager
#include "exceptions.h" // for throw_std_runtime_error
#include "pages.h" // for add
#include "util/logger.h" // for log_debug
#include "util/tools.h" // for hexDecodeString
#include "web/web_request_handler.h" // for WebRequestHandler

web::add::add(std::shared_ptr<ContentManager> content)
    : WebRequestHandler(std::move(content))
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
        path = hexDecodeString(objID);
    if (path.empty())
        throw_std_runtime_error("illegal path");

    AutoScanSetting asSetting;
    asSetting.recursive = true;
    asSetting.mergeOptions(config, path);

    content->addFile(path, asSetting);
    log_debug("add: returning");
}
