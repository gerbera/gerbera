/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    containers.cc - this file is part of MediaTomb.
    
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

/// \file containers.cc

#include "cds_objects.h"
#include "common.h"
#include "pages.h"
#include "storage/storage.h"

web::containers::containers(std::shared_ptr<ConfigManager> config, std::shared_ptr<Storage> storage,
    std::shared_ptr<ContentManager> content, std::shared_ptr<SessionManager> sessionManager)
    : WebRequestHandler(config, storage, content, sessionManager)
{
}

void web::containers::process()
{
    log_debug(("containers.cc: containers::process()"));
    check_request();

    int parentID = intParam("parent_id", INVALID_OBJECT_ID);
    if (parentID == INVALID_OBJECT_ID)
        throw std::runtime_error("web::containers: no parent_id given");

    auto root = xmlDoc->document_element();

    auto containers = root.append_child("containers");
    xml2JsonHints->setArrayName(containers, "container");
    containers.append_attribute("parent_id") = parentID;
    containers.append_attribute("type") = "database";
    if (string_ok(param("select_it")))
        containers.append_attribute("select_it") = param("select_it").c_str();

    auto param = std::make_unique<BrowseParam>(parentID, BROWSE_DIRECT_CHILDREN | BROWSE_CONTAINERS);
    auto arr = storage->browse(param);

    for (size_t i = 0; i < arr.size(); i++) {
        auto obj = arr[i];
        //if (IS_CDS_CONTAINER(obj->getObjectType()))
        //{
        auto cont = std::static_pointer_cast<CdsContainer>(obj);
        auto ce = containers.append_child("container");
        ce.append_attribute("id") = cont->getID();
        int childCount = cont->getChildCount();
        ce.append_attribute("child_count") = childCount;
        int autoscanType = cont->getAutoscanType();
        ce.append_attribute("autoscan_type") = mapAutoscanType(autoscanType).c_str();

        std::string autoscanMode = "none";
        if (autoscanType > 0) {
            autoscanMode = "timed";
#ifdef HAVE_INOTIFY
            if (config->getBoolOption(CFG_IMPORT_AUTOSCAN_USE_INOTIFY)) {
                std::shared_ptr<AutoscanDirectory> adir = storage->getAutoscanDirectory(cont->getID());
                if ((adir != nullptr) && (adir->getScanMode() == ScanMode::INotify))
                    autoscanMode = "inotify";
            }
#endif
        }
        ce.append_attribute("autoscan_mode") = autoscanMode.c_str();
        ce.append_attribute("title") = cont->getTitle().c_str();
        //}
    }
}
