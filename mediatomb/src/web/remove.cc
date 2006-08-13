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

using namespace zmm;
using namespace mxml;

web::remove::remove() : WebRequestHandler()
{}

void web::remove::process()
{
    log_debug("remove: start\n");

    check_request();
    
    String objID = param(_("object_id"));

    if (string_ok(objID))
        ContentManager::getInstance()->removeObject(objID.toInt());

    /*
    int objectID;
    objID = session->getFrom(sd, _("object_id"));
    if (objID == nil)
    {
        objectID = 0;
    }
    else
    {
        objectID = objID.toInt();
        try
        {
            Storage::getInstance()->loadObject(objectID);
        }
        catch (Exception e)
        {
            objectID = 0;
        }
    }

    requested_count = session->getFrom(sd, _("requested_count"));
    if ((requested_count == nil) || (requested_count == ""))
    {
        requested_count = _("0");
    }

    if (requested_count.toInt() < 0)
        requested_count = _("0");


    Ref<Dictionary> sub(new Dictionary());
    sub->put(_("object_id"), String::from(objectID));
    sub->put(_("requested_count"), requested_count);
    sub->put(_("sid"), sid);
    sub->put(_("driver"), driver);
    *out << subrequest(_("browse"), sub);
    */
    log_debug("remove: returning\n");
}

