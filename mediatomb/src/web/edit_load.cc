/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    edit_load.cc - this file is part of MediaTomb.
    
    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>
    
    Copyright (C) 2006-2007 Gena Batyan <bgeradz@mediatomb.cc>,
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

/// \file edit_load.cc

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
    
    Ref<Element> title (new Element(_("title")));
    title->setText(obj->getTitle());
    title->addAttribute(_("editable"), obj->isVirtual() ? _("1") : _("0"));
    item->appendChild(title);
    
    Ref<Element> classEl (new Element(_("class")));
    classEl->setText(obj->getClass());
    classEl->addAttribute(_("editable"), _("1"));
    item->appendChild(classEl);
    
    int objectType = obj->getObjectType();
    item->appendTextChild(_("objType"), String::from(objectType));
    
    if (IS_CDS_ITEM(objectType))
    {
        Ref<CdsItem> objItem = RefCast(obj, CdsItem);
        
        Ref<Element> description (new Element(_("description")));
        description->setText(objItem->getMetadata(_("dc:description")));
        description->addAttribute(_("editable"), _("1"));
        item->appendChild(description);
        
        Ref<Element> location (new Element(_("location")));
        location->setText(objItem->getLocation());
        if (IS_CDS_PURE_ITEM(objectType) || ! objItem->isVirtual())
            location->addAttribute(_("editable"),_("0"));
        else
            location->addAttribute(_("editable"),_("1"));
        item->appendChild(location);
        
        Ref<Element> mimeType (new Element(_("mime-type")));
        mimeType->setText(objItem->getMimeType());
        mimeType->addAttribute(_("editable"), _("1"));
        item->appendChild(mimeType);
        
        if (IS_CDS_ITEM_EXTERNAL_URL(objectType))
        {
            Ref<Element> protocol (new Element(_("protocol")));
            protocol->setText(getProtocol(objItem->getResource(0)->getAttribute(_("protocolInfo"))));
            protocol->addAttribute(_("editable"), _("1"));
            item->appendChild(protocol);
        }
        else if (IS_CDS_ACTIVE_ITEM(objectType))
        {
            Ref<CdsActiveItem> objActiveItem = RefCast(objItem, CdsActiveItem);
            
            Ref<Element> action (new Element(_("action")));
            action->setText(objActiveItem->getAction());
            action->addAttribute(_("editable"), _("1"));
            item->appendChild(action);
            
            Ref<Element> state (new Element(_("state")));
            state->setText(objActiveItem->getState());
            state->addAttribute(_("editable"), _("1"));
            item->appendChild(state);
        }
    }
    
    root->appendChild(item);
    //log_debug("serving XML: \n%s\n",  root->print().c_str());
}

