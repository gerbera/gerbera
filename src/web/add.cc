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
#include "content_manager.h"
#include "filesystem.h"
#include "pages.h"
#include "server.h"
#include "tools.h"
#include <cstdio>

using namespace zmm;
using namespace mxml;

web::add::add()
    : WebRequestHandler()
{
}

void web::add::process()
{
    log_debug("add: start\n");

    check_request();

    String path;
    String objID = param(_("object_id"));
    if (!string_ok(objID) || objID == "0")
        path = _(FS_ROOT_DIRECTORY);
    else
        path = hex_decode_string(objID);
    if (path == nullptr)
        throw _Exception(_("web::add::process(): illegal path"));

    Ref<ContentManager> cm = ContentManager::getInstance();
    cm->addFile(path, true);
    log_debug("add: returning\n");
}
