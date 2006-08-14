/*  items.cc - this file is part of MediaTomb.
                                                                                
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
#include "common.h"
#include "storage.h"
#include "cds_objects.h"
#include "cds_resource_manager.h"

using namespace zmm;
using namespace mxml;

web::items::items() : WebRequestHandler()
{
}

void web::items::process()
{
    check_request();
    
    String parID = param(_("parent_id"));
    int parentID;
    if (parID == nil)
        parentID = 0;
    else
        parentID = parID.toInt();
    
    Ref<Storage> storage = Storage::getInstance();
    Ref<SelectParam> param(new SelectParam(FILTER_PARENT_ID_ITEMS, parentID));
    
    Ref<Array<CdsObject> > arr = storage->selectObjects(param);
    
    Ref<Element> items (new Element(_("items")));
                            
    for (int i = 0; i < arr->size(); i++)
    {
        Ref<CdsObject> obj = arr->get(i);
        if (IS_CDS_ITEM(obj->getObjectType()))
        {
            Ref<Element> item (new Element(_("item")));
            item->addAttribute(_("id"), String::from(obj->getID()));
            item->appendTextChild(_("title"), obj->getTitle());
            item->appendTextChild(_("res"), CdsResourceManager::getInstance()->getFirstResource(RefCast(obj, CdsItem)));
            items->appendChild(item);
        }
    }
    
    root->appendChild(items);
}

