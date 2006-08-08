/*  cds_resource_manager.cc - this file is part of MediaTomb.
                                                                                
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

#include "cds_resource_manager.h"
#include "dictionary.h"
#include "server.h"
#include "common.h"

using namespace zmm;
using namespace mxml;

static Ref<CdsResourceManager> instance;

CdsResourceManager::CdsResourceManager() : Object()
{

}

Ref<CdsResourceManager> CdsResourceManager::getInstance()
{
    if (instance == nil)
    {
        instance = Ref<CdsResourceManager>(new CdsResourceManager());
    }
    return instance;
}

void CdsResourceManager::addResources(Ref<CdsItem> item, Ref<Element> element)
{
    Ref<Element> res;

    String urlBase;
    /// \todo resource options must be read from configuration files

    Ref<Dictionary> dict(new Dictionary());
    dict->put(_(URL_OBJECT_ID), String::from(item->getID()));

    bool addResID = false;
    /// \todo move this down into the "for" loop and create different urls for each resource once the io handlers are ready
    int objectType = item->getObjectType();
    if (IS_CDS_ITEM_INTERNAL_URL(objectType))
    {
        urlBase = Server::getInstance()->getVirtualURL() + "/" + CONTENT_SERVE_HANDLER + 
                  "/" + item->getLocation();
    }
    else if (IS_CDS_ITEM_EXTERNAL_URL(objectType))
    {
        urlBase = item->getLocation();
    }
    else
    { 
        urlBase = Server::getInstance()->getVirtualURL() + "/" +
            CONTENT_MEDIA_HANDLER + "?" + dict->encode();
        addResID = true;
    }

    int resCount = item->getResourceCount();
    for (int i = 0; i < resCount; i++)
    {
        /// \todo what if the resource has a different mimetype than the item??
/*        String mimeType = item->getMimeType();
        if (!string_ok(mimeType)) mimeType = DEFAULT_MIMETYPE; */

        Ref<Dictionary> res_attrs = item->getResource(i)->getAttributes();
        /// \todo who will sync mimetype that is part of the protocl info and
        /// that is lying in the resources with the information that is in the
        /// resource tags?

//        res_attrs->put("protocolInfo", prot + mimeType + ":*");
        String tmp;
        if (addResID)
            tmp = urlBase + "&" + "res_id=" + i;
        else
            tmp = urlBase;
      
        element->appendChild(UpnpXML_DIDLRenderResource(tmp, res_attrs));
    }
}

