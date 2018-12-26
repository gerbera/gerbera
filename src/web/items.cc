/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    items.cc - this file is part of MediaTomb.
    
    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>
    
    Copyright (C) 2006-2010 Gena Batyan <bgeradz@mediatomb.cc>,
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

#include "cds_objects.h"
#include "common.h"
#include "pages.h"
#include "storage.h"
#include "upnp_xml.h"

using namespace zmm;
using namespace mxml;

web::items::items()
    : WebRequestHandler()
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
    Ref<Element> items(new Element(_("items")));
    items->setArrayName(_("item"));
    items->setAttribute(_("parent_id"), String::from(parentID), mxml_int_type);
    root->appendElementChild(items);
    Ref<CdsObject> obj;
    obj = storage->loadObject(parentID);
    Ref<BrowseParam> param(new BrowseParam(parentID, BROWSE_DIRECT_CHILDREN | BROWSE_ITEMS));
    param->setRange(start, count);

    if ((obj->getClass() == UPNP_DEFAULT_CLASS_MUSIC_ALBUM) || (obj->getClass() == UPNP_DEFAULT_CLASS_PLAYLIST_CONTAINER))
        param->setFlag(BROWSE_TRACK_SORT);

    Ref<Array<CdsObject>> arr;
    arr = storage->browse(param);

    String location = obj->getVirtualPath();
    if (string_ok(location))
        items->setAttribute(_("location"), location);
    items->setAttribute(_("virtual"), (obj->isVirtual() ? _("1") : _("0")), mxml_bool_type);

    items->setAttribute(_("start"), String::from(start), mxml_int_type);
    //items->setAttribute(_("returned"), String::from(arr->size()));
    items->setAttribute(_("total_matches"), String::from(param->getTotalMatches()), mxml_int_type);

    int protectContainer = 0;
    int protectItems = 0;
    String autoscanMode = _("none");

    int autoscanType = storage->getAutoscanDirectoryType(parentID);
    if (autoscanType > 0)
        autoscanMode = _("timed");

#ifdef HAVE_INOTIFY
    if (ConfigManager::getInstance()->getBoolOption(CFG_IMPORT_AUTOSCAN_USE_INOTIFY)) {
        int startpoint_id = INVALID_OBJECT_ID;
        if (autoscanType == 0) {
            startpoint_id = storage->isAutoscanChild(parentID);
        } else {
            startpoint_id = parentID;
        }

        if (startpoint_id != INVALID_OBJECT_ID) {
            Ref<AutoscanDirectory> adir = storage->getAutoscanDirectory(startpoint_id);
            if ((adir != nullptr) && (adir->getScanMode() == ScanMode::INotify)) {
                protectItems = 1;
                if (autoscanType == 0 || adir->persistent())
                    protectContainer = 1;

                autoscanMode = _("inotify");
            }
        }
    }
#endif
    items->setAttribute(_("autoscan_mode"), autoscanMode);
    items->setAttribute(_("autoscan_type"), mapAutoscanType(autoscanType));
    items->setAttribute(_("protect_container"), String::from(protectContainer), mxml_bool_type);
    items->setAttribute(_("protect_items"), String::from(protectItems), mxml_bool_type);

    for (int i = 0; i < arr->size(); i++) {
        Ref<CdsObject> obj = arr->get(i);
        //if (IS_CDS_ITEM(obj->getObjectType()))
        //{
        Ref<Element> item(new Element(_("item")));
        item->setAttribute(_("id"), String::from(obj->getID()), mxml_int_type);
        item->appendTextChild(_("title"), obj->getTitle());
        /// \todo clean this up, should have more generic options for online
        /// services
        // FIXME
        item->appendTextChild(_("res"), UpnpXMLBuilder::getFirstResourcePath(RefCast(obj, CdsItem)));

        //item->appendTextChild(_("virtual"), obj->isVirtual() ? _("1") : _("0"), mxml_bool_type);
        items->appendElementChild(item);
        //}
    }
}
