/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    containers.cc - this file is part of MediaTomb.
    
    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>
    
    Copyright (C) 2006-2007 Gena Batyan <bgeradz@mediatomb.cc>,
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

#ifdef HAVE_CONFIG_H
    #include "autoconfig.h"
#endif

#include "common.h"
#include "pages.h"
#include "storage.h"
#include "cds_objects.h"

using namespace zmm;
using namespace mxml;

web::containers::containers() : WebRequestHandler()
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

        
    Ref<Element> containers (new Element(_("containers")));
    containers->addAttribute(_("ofId"), String::from(parentID));
    containers->addAttribute(_("type"), _("d"));

    if (string_ok(param(_("select_it"))))
        containers->addAttribute(_("select_it"), param(_("select_it")));
    root->appendElementChild(containers);
    
    Ref<BrowseParam> param(new BrowseParam(parentID, BROWSE_DIRECT_CHILDREN | BROWSE_CONTAINERS));
    Ref<Array<CdsObject> > arr;
    try
    {
        arr = storage->browse(param);
    }
    catch (Exception e)
    {
        containers->addAttribute(_("success"), _("0"));
        return;
    }
    containers->addAttribute(_("success"), _("1"));
    
    for (int i = 0; i < arr->size(); i++)
    {
        Ref<CdsObject> obj = arr->get(i);
        //if (IS_CDS_CONTAINER(obj->getObjectType()))
        //{
        Ref<CdsContainer> cont = RefCast(obj, CdsContainer);
        Ref<Element> ce(new Element(_("container")));
        ce->addAttribute(_("id"), String::from(cont->getID()));
        int childCount = cont->getChildCount();
        if (childCount)
            ce->addAttribute(_("childCount"), String::from(childCount));
        int autoscanType = cont->getAutoscanType();
        ce->addAttribute(_("autoscanType"), String::from(autoscanType));
        
        int autoscanMode = 0;
        if (autoscanType > 0)
        {
            autoscanMode = 1;
#ifdef HAVE_INOTIFY
            if (ConfigManager::getInstance()->getBoolOption(CFG_IMPORT_AUTOSCAN_USE_INOTIFY))
            {
                Ref<AutoscanDirectory> adir = storage->getAutoscanDirectory(cont->getID());
                if ((adir != nil) && (adir->getScanMode() == InotifyScanMode))
                    autoscanMode = 2;
            }
#endif
        }
        ce->addAttribute(_("autoscanMode"), String::from(autoscanMode));
        ce->setText(cont->getTitle());
        containers->appendElementChild(ce);
        //}
    }
}
