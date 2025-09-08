/*MT*

    MediaTomb - http://www.mediatomb.cc/

    web/containers.cc - this file is part of MediaTomb.

    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>

    Copyright (C) 2006-2010 Gena Batyan <bgeradz@mediatomb.cc>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>,
                            Leonhard Wimmer <leo@mediatomb.cc>

    Copyright (C) 2016-2025 Gerbera Contributors

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

/// @file web/containers.cc
#define GRB_LOG_FAC GrbLogFacility::web

#include "pages.h" // API

#include "cds/cds_container.h"
#include "config/config_val.h"
#include "config/result/autoscan.h"
#include "content/content.h"
#include "database/database.h"
#include "database/db_param.h"
#include "exceptions.h"
#include "upnp/xml_builder.h"

const std::string_view Web::Containers::PAGE = "containers";

bool Web::Containers::processPageAction(Json::Value& element, const std::string& action)
{
    int parentID = intParam("parent_id", INVALID_OBJECT_ID);
    if (parentID == INVALID_OBJECT_ID)
        throw_std_runtime_error("no parent_id given");

    Json::Value containers;
    Json::Value containerArray(Json::arrayValue);
    containers["parent_id"] = parentID;
    containers["type"] = action == "browse" ? "database" : "search";
    if (!param("select_it").empty())
        containers["select_it"] = param("select_it").c_str();

    log_debug("{} {}", action, parentID);
    auto flags = BROWSE_DIRECT_CHILDREN | BROWSE_CONTAINERS;
    if (config->getBoolOption(ConfigVal::SERVER_HIDE_PC_DIRECTORY_WEB))
        flags |= BROWSE_HIDE_FS_ROOT;
    auto browseParam = BrowseParam(database->loadObject(getGroup(), parentID), flags);
    auto arr = database->browse(browseParam);
    for (auto&& obj : arr) {
        auto cont = std::static_pointer_cast<CdsContainer>(obj);
        Json::Value ce;
        ce["id"] = cont->getID();
        ce["ref_id"] = cont->getRefID();
        ce["child_count"] = cont->getChildCount();

        auto url = xmlBuilder->renderContainerImageURL(cont);
        if (url) {
            ce["image"] = url.value();
        }

        auto autoscanType = cont->getAutoscanType();
        std::string autoscanMode = (autoscanType != AutoscanType::None) ? AUTOSCAN_TIMED : "none";
        auto adir = content->getAutoscanDirectory(cont->getLocation());
        if (adir) {
            autoscanType = autoscanType == AutoscanType::None ? AutoscanType::Config : autoscanType;
            autoscanMode = (adir->getScanMode() == AutoscanScanMode::Timed) ? AUTOSCAN_TIMED : AUTOSCAN_MANUAL;
        }
#ifdef HAVE_INOTIFY
        if (config->getBoolOption(ConfigVal::IMPORT_AUTOSCAN_USE_INOTIFY)) {
            if (adir && (adir->getScanMode() == AutoscanScanMode::INotify)) {
                autoscanMode = AUTOSCAN_INOTIFY;
                autoscanType = autoscanType == AutoscanType::None ? AutoscanType::Config : autoscanType;
            }
        }
#endif
        ce["autoscan_type"] = mapAutoscanType(autoscanType).data();
        ce["autoscan_mode"] = autoscanMode;
        ce["persistent"] = cont->getFlags() & OBJECT_FLAG_PERSISTENT_CONTAINER ? true : false;
        ce["title"] = cont->getTitle();
        ce["location"] = cont->getLocation().string();
        ce["upnp_shortcut"] = cont->getUpnpShortcut();
        ce["upnp_class"] = cont->getClass();
        containerArray.append(ce);
    }
    containers["container"] = containerArray;
    element["containers"] = containers;

    return true;
}
