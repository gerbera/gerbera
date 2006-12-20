/*MT*
    
    MediaTomb - http://www.mediatomb.org/
    
    containers.cc - this file is part of MediaTomb.
    
    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.org>,
                       Sergey Bostandzhyan <jin@mediatomb.org>
    Copyright (C) 2006 Gena Batyan <bgeradz@mediatomb.org>,
                       Sergey Bostandzhyan <jin@mediatomb.org>,
                       Leonhard Wimmer <leo@mediatomb.org>
    
    MediaTomb is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.
    
    MediaTomb is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    version 2 along with MediaTomb; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
    
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
    containers->addAttribute(_("select_it"), param(_("select_it")));
    root->appendChild(containers);
    
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
        if (cont->isAutoscanStart())
            ce->addAttribute(_("autoscan"), _("1"));
        ce->setText(cont->getTitle());
        containers->appendChild(ce);
        //}
    }
}

