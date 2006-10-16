/*  remove.cc - this file is part of MediaTomb.
                                                                                
    Copyright (C) 2005 Gena Batyan <bgeradz@deadlock.dhs.org>,
                       Sergey Bostandzhyan <jin@deadlock.dhs.org>
                                                                                
    MediaTomb is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
                                                                                
    MediaTomb is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
                                                                                
    You should have received a copy of the GNU General Public License
    along with MediaTomb; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifdef HAVE_CONFIG_H
    #include "autoconfig.h"
#endif

#include "server.h"
#include <stdio.h>
#include "common.h"
#include "content_manager.h"
#include "pages.h"
#include "tools.h"
#include "storage.h"

using namespace zmm;
using namespace mxml;

web::remove::remove() : WebRequestHandler()
{}

void web::remove::process()
{
    log_debug("remove: start\n");
    
    check_request();
    
    String objID = param(_("object_id"));
    int objectID;
    if (!string_ok(objID))
        throw _Exception(_("invalid object id"));
    else
        objectID = objID.toInt();
    
    String allStr = param(_("all"));
    int allInt;
    if (!string_ok(allStr))
        throw _Exception(_("invalid 'all' flag"));
    else
        allInt = allStr.toInt();
    bool all = allInt;
    
    ContentManager::getInstance()->removeObject(objectID, false, all);
    
    log_debug("remove: returning\n");
}

