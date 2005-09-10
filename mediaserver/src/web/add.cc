/*  add.cc - this file is part of MediaTomb.
                                                                                
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

#include "server.h"
#include <stdio.h>
#include "common.h"
#include "storage.h"
#include "cds_objects.h"
#include "dictionary.h"
#include "pages.h"
#include "content_manager.h"
#include "session_manager.h"

using namespace zmm;
using namespace mxml;

web::add::add() : WebRequestHandler()
{
    pagename = _("add");
}

void web::add::process()
{
    Ref<Session>   session;
    Ref<Storage>   storage;
    session_data_t sd;

    check_request();
    
    String objID = param(_("object_id"));
    int objectID;
    String path = param(_("path"));
    String driver = param(_("driver"));
    String sid = param(_("sid"));

    storage = Storage::getInstance();
    sd = PRIMARY;

    // there must at least a path or an objectID given
    if (((objID == nil) || (objID == "")) && ((path == nil) || (path == "")))
        throw Exception(_("web::add::process(): missing parameters: either path or object id must be specified"));


    // if path is there, we take it, else browse...
    if (objID != nil && objID != "")
        objectID = objID.toInt();
    else
        objectID = 0;

    if ((path == nil) || (path == ""))
    {
        Ref<CdsObject> current = storage->loadObject(objectID);
        path = current->getLocation();
    }
    
    Ref<ContentManager> cm = ContentManager::getInstance();
    cm->addFile(path, true);

    session = SessionManager::getInstance()->getSession(sid);

    objID = session->getFrom(sd, _("object_id"));
    
    Ref<Dictionary> sub(new Dictionary());
    sub->put(_("object_id"), objID);
    sub->put(_("driver"), driver);
    sub->put(_("sid"), sid); 
    *out << subrequest(_("browse"), sub);
}

