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

/// \file web/containers.cc
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
#include "util/xml_to_json.h"

const std::string Web::Containers::PAGE = "containers";

void Web::Containers::processPageAction(pugi::xml_node& element)
{
    int parentID = intParam("parent_id", INVALID_OBJECT_ID);
    std::string action = param("action");
    if (parentID == INVALID_OBJECT_ID)
        throw_std_runtime_error("no parent_id given");

    auto containers = element.append_child("containers");
    xml2Json->setArrayName(containers, "container");
    xml2Json->setFieldType("title", FieldType::STRING);
    containers.append_attribute("parent_id") = parentID;
    containers.append_attribute("type") = action == "browse" ? "database" : "search";
    if (!param("select_it").empty())
        containers.append_attribute("select_it") = param("select_it").c_str();

    log_debug("{} {}", action, parentID);
    auto browseParam = BrowseParam(database->loadObject(getGroup(), parentID), BROWSE_DIRECT_CHILDREN | BROWSE_CONTAINERS);
    auto arr = database->browse(browseParam);
    for (auto&& obj : arr) {
        auto cont = std::static_pointer_cast<CdsContainer>(obj);
        auto ce = containers.append_child("container");
        ce.append_attribute("id") = cont->getID();
        ce.append_attribute("ref_id") = cont->getRefID();
        int childCount = cont->getChildCount();
        ce.append_attribute("child_count") = childCount;

        auto url = xmlBuilder->renderContainerImageURL(cont);
        if (url) {
            ce.append_attribute("image") = url.value().c_str();
        }

        auto autoscanType = cont->getAutoscanType();
        std::string autoscanMode = (autoscanType != AutoscanType::None) ? AUTOSCAN_TIMED : "none";
        auto adir = content->getAutoscanDirectory(cont->getLocation());
        if (adir) {
            autoscanType = autoscanType == AutoscanType::None ? AutoscanType::Config : autoscanType;
            autoscanMode = AUTOSCAN_TIMED;
        }
#ifdef HAVE_INOTIFY
        if (config->getBoolOption(ConfigVal::IMPORT_AUTOSCAN_USE_INOTIFY)) {
            if (adir && (adir->getScanMode() == AutoscanScanMode::INotify)) {
                autoscanMode = AUTOSCAN_INOTIFY;
                autoscanType = autoscanType == AutoscanType::None ? AutoscanType::Config : autoscanType;
            }
        }
#endif
        ce.append_attribute("autoscan_type") = mapAutoscanType(autoscanType).data();
        ce.append_attribute("autoscan_mode") = autoscanMode.c_str();
        ce.append_attribute("persistent") = cont->getFlags() & OBJECT_FLAG_PERSISTENT_CONTAINER ? "true" : "false";
        ce.append_attribute("title") = cont->getTitle().c_str();
        ce.append_attribute("location") = cont->getLocation().c_str();
        ce.append_attribute("upnp_shortcut") = cont->getUpnpShortcut().c_str();
        ce.append_attribute("upnp_class") = cont->getClass().c_str();
    }
}
