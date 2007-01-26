/*MT*
    
    MediaTomb - http://www.mediatomb.org/
    
    items.cc - this file is part of MediaTomb.
    
    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.org>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.org>
    
    Copyright (C) 2006-2007 Gena Batyan <bgeradz@mediatomb.org>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.org>,
                            Leonhard Wimmer <leo@mediatomb.org>
    
    MediaTomb is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.
    
    MediaTomb is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    version 2 along with MediaTomb; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
    
    $Id$
*/

/// \file items.cc

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
    
    int parentID = intParam(_("parent_id"));
    int start = intParam(_("start"));
    int count = intParam(_("count"));
    if (start < 0)
        throw _Exception(_("illegal start parameter"));
    if (count < 0)
        throw _Exception(_("illegal count parameter"));
    
    Ref<Storage> storage = Storage::getInstance();
    Ref<Element> items (new Element(_("items")));
    items->addAttribute(_("ofId"), String::from(parentID));
    root->appendChild(items);
    Ref<CdsObject> obj;
    try
    {
        obj = storage->loadObject(parentID);
    }
    catch (Exception e)
    {
        items->addAttribute(_("success"), _("0"));
        return;
    }
    Ref<BrowseParam> param(new BrowseParam(parentID, BROWSE_DIRECT_CHILDREN | BROWSE_ITEMS));
    param->setRange(start, count);
    
    Ref<Array<CdsObject> > arr;
    try
    {
        arr = storage->browse(param);
    }
    catch (Exception e)
    {
        items->addAttribute(_("success"), _("0"));
        return;
    }
    items->addAttribute(_("success"), _("1"));
    
    String location = obj->getVirtualPath(); 
    if (string_ok(location))
        items->addAttribute(_("location"), location);
    items->addAttribute(_("virtual"), (obj->isVirtual() ? _("1") : _("0")));
    items->addAttribute(_("autoscan"), String::from(storage->getAutoscanDirectoryType(parentID)));
    items->addAttribute(_("start"), String::from(start));
    //items->addAttribute(_("returned"), String::from(arr->size()));
    items->addAttribute(_("totalMatches"), String::from(param->getTotalMatches()));
                            
    for (int i = 0; i < arr->size(); i++)
    {
        Ref<CdsObject> obj = arr->get(i);
        //if (IS_CDS_ITEM(obj->getObjectType()))
        //{
        Ref<Element> item (new Element(_("item")));
        item->addAttribute(_("id"), String::from(obj->getID()));
        item->appendTextChild(_("title"), obj->getTitle());
        item->appendTextChild(_("res"), CdsResourceManager::getFirstResource(RefCast(obj, CdsItem)));
        item->appendTextChild(_("virtual"), obj->isVirtual() ? _("1") : _("0"));
        items->appendChild(item);
        //}
    }
    
    root->appendChild(items);
}

