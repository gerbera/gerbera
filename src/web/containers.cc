/*MT*

    MediaTomb - http://www.mediatomb.cc/

    containers.cc - this file is part of MediaTomb.

    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>

    Copyright (C) 2006-2010 Gena Batyan <bgeradz@mediatomb.cc>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>,
                            Leonhard Wimmer <leo@mediatomb.cc>

    Copyright (C) 2016-2024 Gerbera Contributors

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

/// \file containers.cc
#define GRB_LOG_FAC GrbLogFacility::web

#include "pages.h" // API

#include "cds/cds_container.h"
#include "config/config_val.h"
#include "config/result/autoscan.h"
#include "database/database.h"
#include "exceptions.h"
#include "server.h"
#include "upnp/xml_builder.h"
#include "util/xml_to_json.h"

Web::Containers::Containers(const std::shared_ptr<Content>& content, std::shared_ptr<UpnpXMLBuilder> xmlBuilder)
    : WebRequestHandler(content)
    , xmlBuilder(std::move(xmlBuilder))
{
}

void Web::Containers::process()
{
    log_debug("start process()");
    checkRequest();

    int parentID = intParam("parent_id", INVALID_OBJECT_ID);
    if (parentID == INVALID_OBJECT_ID)
        throw_std_runtime_error("no parent_id given");

    auto root = xmlDoc->document_element();

    auto containers = root.append_child("containers");
    xml2Json->setArrayName(containers, "container");
    xml2Json->setFieldType("title", "string");
    containers.append_attribute("parent_id") = parentID;
    containers.append_attribute("type") = "database";
    if (!param("select_it").empty())
        containers.append_attribute("select_it") = param("select_it").c_str();

    auto browseParam = BrowseParam(database->loadObject(DEFAULT_CLIENT_GROUP, parentID), BROWSE_DIRECT_CHILDREN | BROWSE_CONTAINERS);
    auto arr = database->browse(browseParam);
    for (auto&& obj : arr) {
        auto cont = std::static_pointer_cast<CdsContainer>(obj);
        auto ce = containers.append_child("container");
        ce.append_attribute("id") = cont->getID();
        int childCount = cont->getChildCount();
        ce.append_attribute("child_count") = childCount;
        int autoscanType = cont->getAutoscanType();
        ce.append_attribute("autoscan_type") = mapAutoscanType(autoscanType).data();

        auto url = xmlBuilder->renderContainerImageURL(cont);
        if (url) {
            ce.append_attribute("image") = url.value().c_str();
        }

        std::string autoscanMode = "none";
        if (autoscanType > 0) {
            autoscanMode = AUTOSCAN_TIMED;
#ifdef HAVE_INOTIFY
            if (config->getBoolOption(ConfigVal::IMPORT_AUTOSCAN_USE_INOTIFY)) {
                auto adir = database->getAutoscanDirectory(cont->getID());
                if (adir && (adir->getScanMode() == AutoscanScanMode::INotify))
                    autoscanMode = AUTOSCAN_INOTIFY;
            }
#endif
        }
        ce.append_attribute("autoscan_mode") = autoscanMode.c_str();
        ce.append_attribute("title") = cont->getTitle().c_str();
    }
    log_debug("end process()");
}
