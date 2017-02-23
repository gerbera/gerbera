/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    edit_save.cc - this file is part of MediaTomb.
    
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

/// \file edit_save.cc

#include "content_manager.h"
#include "dictionary.h"
#include "pages.h"
#include "storage.h"
#include "tools.h"
#include <cstdio>

using namespace zmm;
using namespace mxml;

web::edit_save::edit_save()
    : WebRequestHandler()
{
}

void web::edit_save::process()
{
    check_request();

    String objID = param(_("object_id"));
    int objectID;
    if (!string_ok(objID))
        throw _Exception(_("invalid object id"));
    else
        objectID = objID.toInt();

    ContentManager::getInstance()->updateObject(objectID, params);
}
