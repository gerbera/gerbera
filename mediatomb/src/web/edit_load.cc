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

#include "server.h"
#include "content_manager.h"
// ? #include "storage.h"

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
        throw Exception(_("invalid object id"));
    else
        objectID = objID.toInt();
    
    storage = Storage::getInstance();
    Ref<CdsObject> obj = storage->loadObject(objectID);
    
    Ref<Element> item (new Element(_("item")));
    
    item->appendTextChild(_("title"), obj->getTitle());
    item->appendTextChild(_("class"), obj->getClass());
    //item->appendTextChild(_("description"), obj->;
    
    item->appendTextChild(_("location"), obj->getLocation());
    
    
    
    /*
    Ref<Element> didl_object = UpnpXML_DIDLRenderObject(current, true);
    didl_object->appendTextChild(_("location"), current->getLocation());
    
    
    int objectType = current->getObjectType();
    if (IS_CDS_ITEM_INTERNAL_URL(objectType) || IS_CDS_ITEM_EXTERNAL_URL(objectType))
        didl_object->appendTextChild(_("object_type"), _("url"));
    else if (IS_CDS_ACTIVE_ITEM(objectType))
        didl_object->appendTextChild(_("object_type"), _("act"));
    */
    
    root->appendChild(item);
}

