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
#include "storage.h"

using namespace zmm;
using namespace mxml;

web::containers::containers()
    : WebRequestHandler()
{
}

void web::containers::process()
{
    log_debug(("containers.cc: containers::process()\n"));
    check_request();

    int parentID = intParam("parent_id", INVALID_OBJECT_ID);
    if (parentID == INVALID_OBJECT_ID)
        throw _Exception("web::containers: no parent_id given");

    Ref<Storage> storage = Storage::getInstance();

    Ref<Element> containers(new Element("containers"));
    containers->setArrayName("container");
    containers->setAttribute("parent_id", std::to_string(parentID), mxml_int_type);
    containers->setAttribute("type", "database");

    if (string_ok(param("select_it")))
        containers->setAttribute("select_it", param("select_it"));
    root->appendElementChild(containers);

    Ref<BrowseParam> param(new BrowseParam(parentID, BROWSE_DIRECT_CHILDREN | BROWSE_CONTAINERS));
    Ref<Array<CdsObject>> arr;
    arr = storage->browse(param);

    for (int i = 0; i < arr->size(); i++) {
        Ref<CdsObject> obj = arr->get(i);
        //if (IS_CDS_CONTAINER(obj->getObjectType()))
        //{
        Ref<CdsContainer> cont = RefCast(obj, CdsContainer);
        Ref<Element> ce(new Element("container"));
        ce->setAttribute("id", std::to_string(cont->getID()), mxml_int_type);
        int childCount = cont->getChildCount();
        ce->setAttribute("child_count", std::to_string(childCount), mxml_int_type);
        int autoscanType = cont->getAutoscanType();
        ce->setAttribute("autoscan_type", mapAutoscanType(autoscanType));

        std::string autoscanMode = "none";
        if (autoscanType > 0) {
            autoscanMode = "timed";
#ifdef HAVE_INOTIFY
            if (ConfigManager::getInstance()->getBoolOption(CFG_IMPORT_AUTOSCAN_USE_INOTIFY)) {
                Ref<AutoscanDirectory> adir = storage->getAutoscanDirectory(cont->getID());
                if ((adir != nullptr) && (adir->getScanMode() == ScanMode::INotify))
                    autoscanMode = "inotify";
            }
#endif
        }
        ce->setAttribute("autoscan_mode", autoscanMode);
        ce->setTextKey("title");
        ce->setText(cont->getTitle());
        containers->appendElementChild(ce);
        //}
    }
}
