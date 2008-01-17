/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    items.cc - this file is part of MediaTomb.
    
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
    root->appendElementChild(items);
    Ref<CdsObject> obj;
    try
    {
        obj = storage->loadObject(parentID);
    }
    catch (ObjectNotFoundException e)
    {
        items->addAttribute(_("success"), _("0"));
        return;
    }
    Ref<BrowseParam> param(new BrowseParam(parentID, BROWSE_DIRECT_CHILDREN | BROWSE_ITEMS));
    param->setRange(start, count);

    if ((obj->getClass() == UPNP_DEFAULT_CLASS_MUSIC_ALBUM) ||
        (obj->getClass() == UPNP_DEFAULT_CLASS_PLAYLIST_CONTAINER))
        param->setFlag(BROWSE_TRACK_SORT);
    
    Ref<Array<CdsObject> > arr;
    try
    {
        arr = storage->browse(param);
    }
    catch (ObjectNotFoundException e)
    {
        items->addAttribute(_("success"), _("0"));
        return;
    }
    items->addAttribute(_("success"), _("1"));
    
    String location = obj->getVirtualPath(); 
    if (string_ok(location))
        items->addAttribute(_("location"), location);
    items->addAttribute(_("virtual"), (obj->isVirtual() ? _("1") : _("0")));

    int autoscanType = storage->getAutoscanDirectoryType(parentID);
    items->addAttribute(_("autoscanType"), String::from(autoscanType));
    items->addAttribute(_("start"), String::from(start));
    //items->addAttribute(_("returned"), String::from(arr->size()));
    items->addAttribute(_("totalMatches"), String::from(param->getTotalMatches()));

    int protectContainer = 0;
    int protectItems = 0;
    int autoscanMode = 0;

    if (autoscanType > 0)
        autoscanMode = 1;

#ifdef HAVE_INOTIFY
    int startpoint_id = INVALID_OBJECT_ID;
    if (autoscanType == 0)
    {
        startpoint_id = storage->isAutoscanChild(parentID);
    }
    else
    {
        startpoint_id = parentID;
    }
    
    if (startpoint_id != INVALID_OBJECT_ID)
    {
        Ref<AutoscanDirectory> adir = storage->getAutoscanDirectory(startpoint_id);
        if ((adir != nil) && (adir->getScanMode() == InotifyScanMode))
        {
            protectItems = 1;
            if (autoscanType == 0 || adir->persistent())
                protectContainer = 1;

            autoscanMode = 2;
        }
    }

#endif

    items->addAttribute(_("protectContainer"), String::from(protectContainer));
    items->addAttribute(_("protectItems"), String::from(protectItems));
    items->addAttribute(_("autoscanMode"), String::from(autoscanMode));

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
        items->appendElementChild(item);
        //}
    }
}
