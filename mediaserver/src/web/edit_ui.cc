/*  edit_ui.cc - this file is part of MediaTomb.
                                                                                
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

#include "server.h"
#include <stdio.h>
#include "common.h"
#include "content_manager.h"
#include "cds_objects.h"
#include "dictionary.h"
#include "pages.h"
#include "session_manager.h"

using namespace zmm;
using namespace mxml;

web::edit_ui::edit_ui() : WebRequestHandler()
{}

void web::edit_ui::process()
{
    Ref<Session> session;
    Ref<Storage> storage; // storage instance, will be chosen depending on the driver
        
    session_data_t sd;

    check_request();

    String objID = param(_("object_id"));
    int objectID;
    String driver = param(_("driver"));
    String sid = param(_("sid"));

    if (objID == nil)
        throw Exception(_("invalid object id"));
    else
        objectID = objID.toInt();
    
    storage = Storage::getInstance();
    sd = PRIMARY;

    Ref<CdsObject> current = storage->loadObject(objectID);
    Ref<Element> didl_object = UpnpXML_DIDLRenderObject(current, true);
    didl_object->appendTextChild(_("location"), current->getLocation());
    int objectType = current->getObjectType();
    if (IS_CDS_ITEM_INTERNAL_URL(objectType) || IS_CDS_ITEM_EXTERNAL_URL(objectType))
        didl_object->appendTextChild(_("object_type"), _("url"));
    else if (IS_CDS_ACTIVE_ITEM(objectType))
        didl_object->appendTextChild(_("object_type"), _("act"));
   
    Ref<Element> root (new Element(_("root")));
    root->addAttribute(_("xmlns:dc"), _("http://purl.org/dc/elements/1.1/"));
    root->addAttribute(_("xmlns:upnp"), _("urn:schemas-upnp-org:metadata-1-0/upnp/"));

    root->appendChild(didl_object);
    root->appendTextChild(_("driver"), driver);
    root->appendTextChild(_("sid"), sid);
    root->appendTextChild(_("object_id"), String::from(objectID));


    *out << renderXMLHeader(_("/edit.xsl"));
    *out << root->print();
}

