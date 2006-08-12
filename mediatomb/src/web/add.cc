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

#ifdef HAVE_CONFIG_H
    #include "autoconfig.h"
#endif

#include "server.h"
#include <stdio.h>
#include "common.h"
//#include "storage.h"
//#include "cds_objects.h"
//#include "dictionary.h"
#include "pages.h"
#include "tools.h"
#include "content_manager.h"
//#include "session_manager.h"

using namespace zmm;
using namespace mxml;

web::add::add() : WebRequestHandler()
{
}

void web::add::process()
{
    Ref<Storage>   storage;

    check_request();
    
    String path;
    String objID = param(_("object_id"));
    if (objID == nil || objID == "" || objID == "0")
        throw Exception(_("web::add::process(): missing parameters: object id must be specified"));
    else
        path = hex_decode_string(objID);
    
    if (path == nil) throw Exception(_("web::add::process(): illegal path"));

    Ref<ContentManager> cm = ContentManager::getInstance();
    cm->addFile(path, true);
    
    /*
    session = SessionManager::getInstance()->getSession(sid);

    objID = session->getFrom(sd, _("object_id"));
    
    Ref<Dictionary> sub(new Dictionary());
    sub->put(_("object_id"), objID);
    sub->put(_("driver"), driver);
    sub->put(_("sid"), sid); 
    *out << subrequest(_("browse"), sub);
    */
}

