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
#define LOG_FAC log_facility_t::web

#include "pages.h" // API

#include "cds/cds_item.h"
#include "content/autoscan.h"
#include "database/database.h"
#include "server.h"
#include "upnp_xml.h"
#include "util/xml_to_json.h"

Web::Items::Items(const std::shared_ptr<ContentManager>& content, std::shared_ptr<UpnpXMLBuilder> xmlBuilder)
    : WebRequestHandler(content)
    , xmlBuilder(std::move(xmlBuilder))
{
}

/// \brief orocess request for item list in ui
void Web::Items::process()
{
    checkRequest();

    int parentID = intParam("parent_id");
    int start = intParam("start");
    int count = intParam("count");
    if (start < 0)
        throw_std_runtime_error("illegal start parameter");
    if (count < 0)
        throw_std_runtime_error("illegal count parameter");

    // set result options
    auto root = xmlDoc->document_element();
    auto items = root.append_child("items");
    xml2Json->setArrayName(items, "item");
    xml2Json->setFieldType("title", "string");
    xml2Json->setFieldType("part", "string");
    xml2Json->setFieldType("track", "string");
    items.append_attribute("parent_id") = parentID;

    auto container = database->loadObject(DEFAULT_CLIENT_GROUP, parentID);
    auto param = BrowseParam(container, BROWSE_DIRECT_CHILDREN | BROWSE_ITEMS);
    param.setRange(start, count);

    if (container->isSubClass(UPNP_CLASS_MUSIC_ALBUM) || container->isSubClass(UPNP_CLASS_PLAYLIST_CONTAINER))
        param.setFlag(BROWSE_TRACK_SORT);

    // get contents of request
    auto arr = database->browse(param);
    items.append_attribute("virtual") = container->isVirtual();
    items.append_attribute("start") = start;
    // items.append_attribute("returned") = arr->size();
    items.append_attribute("total_matches") = param.getTotalMatches();

    bool protectContainer = false;
    bool protectItems = false;
    std::string autoscanMode = "none";

    auto parentDir = database->getAutoscanDirectory(parentID);
    int autoscanType = 0;
    if (parentDir) {
        autoscanType = parentDir->persistent() ? 2 : 1;
        autoscanMode = AUTOSCAN_TIMED;
    }

#ifdef HAVE_INOTIFY
    if (config->getBoolOption(CFG_IMPORT_AUTOSCAN_USE_INOTIFY)) {
        // check for inotify mode
        int startpointId = INVALID_OBJECT_ID;
        if (autoscanType == 0) {
            auto pathIDs = database->getPathIDs(parentID);
            for (int pathId : pathIDs) {
                auto pathDir = database->getAutoscanDirectory(pathId);
                if (pathDir && pathDir->getRecursive()) {
                    startpointId = pathId;
                    break;
                }
            }
        } else {
            startpointId = parentID;
        }

        if (startpointId != INVALID_OBJECT_ID) {
            auto startPtDir = database->getAutoscanDirectory(startpointId);
            if (startPtDir && startPtDir->getScanMode() == AutoscanScanMode::INotify) {
                protectItems = true;
                if (autoscanType == 0 || startPtDir->persistent())
                    protectContainer = true;

                autoscanMode = AUTOSCAN_INOTIFY;
            }
        }
    }
#endif
    items.append_attribute("autoscan_mode") = autoscanMode.c_str();
    items.append_attribute("autoscan_type") = mapAutoscanType(autoscanType).data();
    items.append_attribute("protect_container") = protectContainer;
    items.append_attribute("protect_items") = protectItems;

    // ouput objects of container
    int cnt = start + 1;
    auto cls = container->getClass();
    auto trackFmt = (param.getTotalMatches() >= 100) ? "{:03}" : "{:02}";
    for (auto&& arrayObj : arr) {
        auto item = items.append_child("item");
        item.append_attribute("id") = arrayObj->getID();
        item.append_child("title").append_child(pugi::node_pcdata).set_value(arrayObj->getTitle().c_str());

        auto objItem = std::static_pointer_cast<CdsItem>(arrayObj);
        if (objItem->getPartNumber() > 0 && startswith(cls, UPNP_CLASS_MUSIC_ALBUM))
            item.append_child("part").append_child(pugi::node_pcdata).set_value(fmt::format("{:02}", objItem->getPartNumber()).c_str());
        if (objItem->getTrackNumber() > 0 && !startswith(cls, UPNP_CLASS_CONTAINER))
            item.append_child("track").append_child(pugi::node_pcdata).set_value(fmt::format(trackFmt, objItem->getTrackNumber()).c_str());
        else
            item.append_child("track").append_child(pugi::node_pcdata).set_value(fmt::format(trackFmt, cnt).c_str());
        item.append_child("mtype").append_child(pugi::node_pcdata).set_value(objItem->getMimeType().c_str());
        item.append_child("upnp_class").append_child(pugi::node_pcdata).set_value(objItem->getClass().c_str());
        std::string res = xmlBuilder->getFirstResourcePath(objItem);
        item.append_child("res").append_child(pugi::node_pcdata).set_value(res.c_str());

        auto url = xmlBuilder->renderItemImageURL(objItem);
        if (url) {
            item.append_child("image").append_child(pugi::node_pcdata).set_value(url.value().c_str());
        }
        cnt++;
    }
}
