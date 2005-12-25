/*  task.cc - this file is part of MediaTomb.
                                                                                
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

#include "common.h"
#include "pages.h"

#include "storage.h"
#include "cds_objects.h"
#include "content_manager.h"
#include "tools.h"

using namespace zmm;
using namespace mxml;

web::task::task() : WebRequestHandler()
{
    pagename = _("task");
    plainXML = true;
}

void web::task::process()
{
    check_request();

    Ref<ContentManager> cm = ContentManager::getInstance();
    
    String name = param(_("name"));
    if (name == nil)
        throw Exception(_("web::task: no task name given"));

    if (name == "add")
    {
        String path = param(_("path"));
        if (path == nil)
            return;
        cm->addFile(hex_decode_string(path), true, true);
    }
    else if (name == "remove")
    {
        String objID = param(_("object_id"));
        if (objID == nil)
            return;
        int objectID = objID.toInt();
        cm->removeObject(objectID, true);
    }
}

