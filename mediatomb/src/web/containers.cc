/*  containers.cc - this file is part of MediaTomb.
                                                                                
    Copyright (C) 2005 Gena Batyan <bgeradz@deadlock.dhs.org>,
                       Sergey Bostandzhyan <jin@deadlock.dhs.org>
                                                                                
    MediaTomb is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
                                                                                
    MediaTomb is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
                                                                                
    You should have received a copy of the GNU General Public License
    along with MediaTomb; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "common.h"
#include "pages.h"

#include "storage.h"
#include "cds_objects.h"

using namespace zmm;
using namespace mxml;

web::containers::containers() : WebRequestHandler()
{
    pagename = _("containers");
    plainXML = true;
}

void web::containers::process()
{
    check_request();

    String parID = param(_("parent_id"));
    int parentID;
    if (parID == nil)
        throw Exception(_("web::containers: no parent_id given"));
    else
        parentID = parID.toInt();
/*
    root->addAttribute("xmlns:dc", "http://purl.org/dc/elements/1.1/");
    root->addAttribute("xmlns:upnp", "urn:schemas-upnp-org:metadata-1-0/upnp/");
*/
    Ref<Storage> storage = Storage::getInstance();
    Ref<BrowseParam> param(new BrowseParam(parentID, BROWSE_DIRECT_CHILDREN));
    param->containersOnly = true;

    Ref<Array<CdsObject> > arr = storage->browse(param);

    // we keep the browse result in the DIDL-Lite tag in our xml
    Ref<Element> containers (new Element(_("containers")));

    for (int i = 0; i < arr->size(); i++)
    {
        Ref<CdsObject> obj = arr->get(i);
        Ref<CdsContainer> cont = RefCast(obj, CdsContainer);
        // this has to be adjusted: the resourced for browsing are different
        Ref<Element> ce(new Element(_("container")));
        ce->addAttribute(_("id"), String::from(cont->getID()));
        int childCount = cont->getChildCount();
        if (childCount)
            ce->addAttribute(_("childCount"), String::from(childCount));
        ce->setText(cont->getTitle());
        containers->appendChild(ce);
    }
    
    root->appendChild(containers);
}

