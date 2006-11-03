/*MT*
    
    MediaTomb - http://www.mediatomb.org/
    
    cds_resource_manager.cc - this file is part of MediaTomb.
    
    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.org>,
                       Sergey Bostandzhyan <jin@mediatomb.org>
    Copyright (C) 2006 Gena Batyan <bgeradz@mediatomb.org>,
                       Sergey Bostandzhyan <jin@mediatomb.org>,
                       Leonhard Wimmer <leo@mediatomb.org>
    
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
    
    $Id$
*/

/// \file cds_resource_manager.cc

#ifdef HAVE_CONFIG_H
    #include "autoconfig.h"
#endif

#include "cds_resource_manager.h"
#include "dictionary.h"
#include "server.h"
#include "common.h"
#include "tools.h"

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
    Ref<UrlBase> urlBase = addResources_getUrlBase(item);
    
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
        
        //  res_attrs->put("protocolInfo", prot + mimeType + ":*");
        String tmp;
        if (urlBase->addResID)
            tmp = urlBase->urlBase + i;
        else
            tmp = urlBase->urlBase;
    
        if ((!IS_CDS_ITEM_INTERNAL_URL(item->getObjectType())) &&
            (!IS_CDS_ITEM_EXTERNAL_URL(item->getObjectType())))
        {
            // this is especially for the TG100, we need to add the file extension
            String location = item->getLocation();
            int dot = location.rindex('.');
            if (dot > -1)
            {
                String extension = location.substring(dot);
                // make sure that the extension does not contain the separator character
                if (string_ok(extension) && (extension.index(URL_PARAM_SEPARATOR) == -1) 
                        && (extension.index(URL_ARG_SEPARATOR) == -1))
                {
                    tmp = tmp + _(_URL_ARG_SEPARATOR) + _(URL_FILE_EXTENSION) + _("=") + extension;
//                    log_debug("New URL: %s\n", tmp.c_str());
                }
            }
        }
        element->appendChild(UpnpXML_DIDLRenderResource(tmp, res_attrs));
    }
}

Ref<CdsResourceManager::UrlBase> CdsResourceManager::addResources_getUrlBase(Ref<CdsItem> item)
{
    Ref<Element> res;
    
    Ref<UrlBase> urlBase(new UrlBase);
    /// \todo resource options must be read from configuration files
    
    Ref<Dictionary> dict(new Dictionary());
    dict->put(_(URL_OBJECT_ID), String::from(item->getID()));
    
    urlBase->addResID = false;
    /// \todo move this down into the "for" loop and create different urls for each resource once the io handlers are ready
    int objectType = item->getObjectType();
    if (IS_CDS_ITEM_INTERNAL_URL(objectType))
    {
        urlBase->urlBase = Server::getInstance()->getVirtualURL() + _("/") + CONTENT_SERVE_HANDLER + 
                  _("/") + item->getLocation();
    }
    else if (IS_CDS_ITEM_EXTERNAL_URL(objectType))
    {
        urlBase->urlBase = item->getLocation();
    }
    else
    { 
        urlBase->urlBase = Server::getInstance()->getVirtualURL() + _("/") +
            CONTENT_MEDIA_HANDLER + _(_URL_PARAM_SEPARATOR) + dict->encode() + _(_URL_ARG_SEPARATOR) + _(URL_RESOURCE_ID) + _("=");
        urlBase->addResID = true;
    }
    return urlBase;
}

String CdsResourceManager::getFirstResource(Ref<CdsItem> item)
{
    Ref<UrlBase> urlBase = addResources_getUrlBase(item);
    
    if (urlBase->addResID)
        return urlBase->urlBase + 0;
    else
        return urlBase->urlBase;
}
