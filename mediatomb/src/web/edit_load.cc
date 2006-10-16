/*  edit_ui.cc - this file is part of MediaTomb.
                                                                                
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

#include "pages.h"

#include <stdio.h>
#include "common.h"
#include "cds_objects.h"
#include "tools.h"
#include "storage.h"

//#include "server.h"
//#include "content_manager.h"


using namespace zmm;
using namespace mxml;

web::edit_load::edit_load() : WebRequestHandler()
{}

void web::edit_load::process()
{
    check_request();
    
    Ref<Storage> storage;
    
    String objID = param(_("object_id"));
    int objectID;
    if (objID == nil)
        throw _Exception(_("invalid object id"));
    else
        objectID = objID.toInt();
    
    storage = Storage::getInstance();
    Ref<CdsObject> obj = storage->loadObject(objectID);
    
    Ref<Element> item (new Element(_("item")));
    
    item->setAttribute(_("object_id"), objID);
    item->appendTextChild(_("title"), obj->getTitle());
    item->appendTextChild(_("class"), obj->getClass());
    
    int objectType = obj->getObjectType();
    item->appendTextChild(_("objType"), String::from(objectType));
    
    if (IS_CDS_ITEM(objectType))
    {
        Ref<CdsItem> objItem = RefCast(obj, CdsItem);
        item->appendTextChild(_("description"), objItem->getMetadata(_("dc:description")));
        item->appendTextChild(_("location"), objItem->getLocation());
        item->appendTextChild(_("mime-type"), objItem->getMimeType());
        
        if (IS_CDS_ITEM_EXTERNAL_URL(objectType))
        {
            item->appendTextChild(_("protocol"), getProtocol(objItem->getResource(0)->getAttribute(_("protocolInfo"))));
        }
        else if (IS_CDS_ACTIVE_ITEM(objectType))
        {
            Ref<CdsActiveItem> objActiveItem = RefCast(objItem, CdsActiveItem);
            item->appendTextChild(_("action"), objActiveItem->getAction());
            item->appendTextChild(_("state"), objActiveItem->getState());
        }
    }
    
    root->appendChild(item);
    //log_debug("serving XML: \n%s\n",  root->print().c_str());
}

