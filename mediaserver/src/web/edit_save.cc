/*  edit_save.cc - this file is part of MediaTomb.
                                                                                
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
#include "content_manager.h"
#include "cds_objects.h"
#include "dictionary.h"
#include "pages.h"
#include "session_manager.h"
#include "update_manager.h"
#include "tools.h"

using namespace zmm;
using namespace mxml;

web::edit_save::edit_save() : WebRequestHandler()
{}

void web::edit_save::process()
{
    Ref<Session> session;
    Ref<Storage> storage; // storage instance, will be chosen depending on the driver
    Ref<ContentManager> content;
        
    session_data_t sd;

    check_request();

    String objID = param(_("object_id"));
    int objectID;
    String driver = param(_("driver"));
    String sid = param(_("sid"));

    if (!string_ok(objID))
        throw Exception(_("invalid object id"));
    else
        objectID = objID.toInt();
    
    storage = Storage::getInstance();
    sd = PRIMARY;

    ContentManager::getInstance()->updateObject(objectID, params);
    
    session = SessionManager::getInstance()->getSession(sid);
    
    Ref<Dictionary> sub(new Dictionary());
    
    objID = session->getFrom(sd, _("object_id"));
    if (objID == nil)
        objID = _("0");

    String requested_count = session->getFrom(sd, _("requested_count"));
    if ((requested_count == nil) || (requested_count == ""))
    {
        requested_count = _("0");
    }
    if (requested_count.toInt() < 0)
        requested_count = _("0");

    sub->put(_("object_id"), objID);
    sub->put(_("requested_count"), requested_count);
    sub->put(_("sid"), sid);
    sub->put(_("driver"), driver);
    *out << subrequest(_("browse"), sub);
}

