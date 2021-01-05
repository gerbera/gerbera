/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    remove.cc - this file is part of MediaTomb.
    
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

/// \file remove.cc

#include "pages.h" // API

#include <cstdio>
#include <utility>

#include "content_manager.h"
#include "database/database.h"
#include "server.h"
#include "util/tools.h"

web::remove::remove(std::shared_ptr<ContentManager> content)
    : WebRequestHandler(std::move(content))
{
}

void web::remove::process()
{
    log_debug("remove: start");

    check_request();

    int objectID = intParam("object_id");
    bool all = intParam("all");

    content->removeObject(nullptr, objectID, true, all);

    log_debug("remove: returning");
}
