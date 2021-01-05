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

#include "pages.h" // API

#include <utility>

#include "autoscan.h"
#include "cds_objects.h"
#include "database/database.h"
#include "upnp_xml.h"

web::items::items(std::shared_ptr<ContentManager> content)
    : WebRequestHandler(std::move(content))
{
}

void web::items::process()
{
    check_request();

    int parentID = intParam("parent_id");
    int start = intParam("start");
    int count = intParam("count");
    if (start < 0)
        throw_std_runtime_error("illegal start parameter");
    if (count < 0)
        throw_std_runtime_error("illegal count parameter");

    auto root = xmlDoc->document_element();

    auto items = root.append_child("items");
    xml2JsonHints->setArrayName(items, "item");
    xml2JsonHints->setFieldType("title", "string");
    items.append_attribute("parent_id") = parentID;

    auto obj = database->loadObject(parentID);
    auto param = std::make_unique<BrowseParam>(parentID, BROWSE_DIRECT_CHILDREN | BROWSE_ITEMS);
    param->setRange(start, count);

    if ((obj->getClass() == UPNP_CLASS_MUSIC_ALBUM) || (obj->getClass() == UPNP_CLASS_PLAYLIST_CONTAINER))
        param->setFlag(BROWSE_TRACK_SORT);

    auto arr = database->browse(param);
    items.append_attribute("virtual") = obj->isVirtual();

    items.append_attribute("start") = start;
    //items.append_attribute("returned") = arr->size();
    items.append_attribute("total_matches") = param->getTotalMatches();

    bool protectContainer = false;
    bool protectItems = false;
    std::string autoscanMode = "none";

    auto adir = database->getAutoscanDirectory(parentID);
    int autoscanType = 0;
    if (adir != nullptr) {
        autoscanType = adir->persistent() ? 2 : 1;
        autoscanMode = "timed";
    }

#ifdef HAVE_INOTIFY
    if (config->getBoolOption(CFG_IMPORT_AUTOSCAN_USE_INOTIFY)) {
        int startpoint_id = INVALID_OBJECT_ID;
        if (autoscanType == 0) {
            auto pathIDs = database->getPathIDs(parentID);
            if (pathIDs != nullptr) {
                for (int pathId : *pathIDs) {
                    auto adir = database->getAutoscanDirectory(pathId);
                    if (adir != nullptr && adir->getRecursive()) {
                        startpoint_id = pathId;
                        break;
                    }
                }
            }
        } else {
            startpoint_id = parentID;
        }

        if (startpoint_id != INVALID_OBJECT_ID) {
            std::shared_ptr<AutoscanDirectory> adir = database->getAutoscanDirectory(startpoint_id);
            if (adir != nullptr && adir->getScanMode() == ScanMode::INotify) {
                protectItems = true;
                if (autoscanType == 0 || adir->persistent())
                    protectContainer = true;

                autoscanMode = "inotify";
            }
        }
    }
#endif
    items.append_attribute("autoscan_mode") = autoscanMode.c_str();
    items.append_attribute("autoscan_type") = mapAutoscanType(autoscanType).c_str();
    items.append_attribute("protect_container") = protectContainer;
    items.append_attribute("protect_items") = protectItems;

    for (const auto& obj : arr) {
        //if (obj->isItem())
        //{
        auto item = items.append_child("item");
        item.append_attribute("id") = obj->getID();
        item.append_child("title").append_child(pugi::node_pcdata).set_value(obj->getTitle().c_str());
        /// \todo clean this up, should have more generic options for online
        /// services
        // FIXME
        std::string res = UpnpXMLBuilder::getFirstResourcePath(std::static_pointer_cast<CdsItem>(obj));
        item.append_child("res").append_child(pugi::node_pcdata).set_value(res.c_str());
        //item.append_attribute("virtual") = obj->isVirtual();
        //}
    }
}
