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

    int parentID = intParam("parent_id");
    int start = intParam("start");
    int count = intParam("count");
    if (start < 0)
        throw _Exception("illegal start parameter");
    if (count < 0)
        throw _Exception("illegal count parameter");

    Ref<Storage> storage = Storage::getInstance();
    Ref<Element> items(new Element("items"));
    items->setArrayName("item");
    items->setAttribute("parent_id", std::to_string(parentID), mxml_int_type);
    root->appendElementChild(items);
    Ref<CdsObject> obj;
    obj = storage->loadObject(parentID);
    Ref<BrowseParam> param(new BrowseParam(parentID, BROWSE_DIRECT_CHILDREN | BROWSE_ITEMS));
    param->setRange(start, count);

    if ((obj->getClass() == UPNP_DEFAULT_CLASS_MUSIC_ALBUM) || (obj->getClass() == UPNP_DEFAULT_CLASS_PLAYLIST_CONTAINER))
        param->setFlag(BROWSE_TRACK_SORT);

    Ref<Array<CdsObject>> arr;
    arr = storage->browse(param);

    std::string location = obj->getVirtualPath();
    if (string_ok(location))
        items->setAttribute("location", location);
    items->setAttribute("virtual", (obj->isVirtual() ? "1" : "0"), mxml_bool_type);

    items->setAttribute("start", std::to_string(start), mxml_int_type);
    //items->setAttribute("returned", std::to_string(arr->size()));
    items->setAttribute("total_matches", std::to_string(param->getTotalMatches()), mxml_int_type);

    int protectContainer = 0;
    int protectItems = 0;
    std::string autoscanMode = "none";

    int autoscanType = storage->getAutoscanDirectoryType(parentID);
    if (autoscanType > 0)
        autoscanMode = "timed";

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

                autoscanMode = "inotify";
            }
        }
    }
#endif
    items->setAttribute("autoscan_mode", autoscanMode);
    items->setAttribute("autoscan_type", mapAutoscanType(autoscanType));
    items->setAttribute("protect_container", std::to_string(protectContainer), mxml_bool_type);
    items->setAttribute("protect_items", std::to_string(protectItems), mxml_bool_type);

    for (int i = 0; i < arr->size(); i++) {
        Ref<CdsObject> obj = arr->get(i);
        //if (IS_CDS_ITEM(obj->getObjectType()))
        //{
        Ref<Element> item(new Element("item"));
        item->setAttribute("id", std::to_string(obj->getID()), mxml_int_type);
        item->appendTextChild("title", obj->getTitle());
        /// \todo clean this up, should have more generic options for online
        /// services
        // FIXME
        item->appendTextChild("res", UpnpXMLBuilder::getFirstResourcePath(RefCast(obj, CdsItem)));

        //item->appendTextChild("virtual", obj->isVirtual() ? "1" : "0", mxml_bool_type);
        items->appendElementChild(item);
        //}
    }
}
