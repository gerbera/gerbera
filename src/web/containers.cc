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

    int parentID = intParam(_("parent_id"), INVALID_OBJECT_ID);
    if (parentID == INVALID_OBJECT_ID)
        throw _Exception(_("web::containers: no parent_id given"));

    Ref<Storage> storage = Storage::getInstance();

    Ref<Element> containers(new Element(_("containers")));
    containers->setArrayName(_("container"));
    containers->setAttribute(_("parent_id"), String::from(parentID), mxml_int_type);
    containers->setAttribute(_("type"), _("database"));

    if (string_ok(param(_("select_it"))))
        containers->setAttribute(_("select_it"), param(_("select_it")));
    root->appendElementChild(containers);

    Ref<BrowseParam> param(new BrowseParam(parentID, BROWSE_DIRECT_CHILDREN | BROWSE_CONTAINERS));
    Ref<Array<CdsObject>> arr;
    arr = storage->browse(param);

    for (int i = 0; i < arr->size(); i++) {
        Ref<CdsObject> obj = arr->get(i);
        //if (IS_CDS_CONTAINER(obj->getObjectType()))
        //{
        Ref<CdsContainer> cont = RefCast(obj, CdsContainer);
        Ref<Element> ce(new Element(_("container")));
        ce->setAttribute(_("id"), String::from(cont->getID()), mxml_int_type);
        int childCount = cont->getChildCount();
        ce->setAttribute(_("child_count"), String::from(childCount), mxml_int_type);
        int autoscanType = cont->getAutoscanType();
        ce->setAttribute(_("autoscan_type"), mapAutoscanType(autoscanType));

        String autoscanMode = _("none");
        if (autoscanType > 0) {
            autoscanMode = _("timed");
#ifdef HAVE_INOTIFY
            if (ConfigManager::getInstance()->getBoolOption(CFG_IMPORT_AUTOSCAN_USE_INOTIFY)) {
                Ref<AutoscanDirectory> adir = storage->getAutoscanDirectory(cont->getID());
                if ((adir != nullptr) && (adir->getScanMode() == ScanMode::INotify))
                    autoscanMode = _("inotify");
            }
#endif
        }
        ce->setAttribute(_("autoscan_mode"), autoscanMode);
        ce->setTextKey(_("title"));
        ce->setText(cont->getTitle());
        containers->appendElementChild(ce);
        //}
    }
}
