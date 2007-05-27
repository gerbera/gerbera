/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    cds_resource_manager.cc - this file is part of MediaTomb.
    
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

/// \file cds_resource_manager.cc

#ifdef HAVE_CONFIG_H
    #include "autoconfig.h"
#endif

#include "cds_resource_manager.h"
#include "dictionary.h"
#include "server.h"
#include "common.h"
#include "tools.h"
#include "metadata_handler.h"

using namespace zmm;
using namespace mxml;

CdsResourceManager::CdsResourceManager() : Object()
{
}

void CdsResourceManager::addResources(Ref<CdsItem> item, Ref<Element> element)
{
    Ref<UrlBase> urlBase = addResources_getUrlBase(item);

#ifdef EXTEND_PROTOCOLINFO
    String prot;
    Ref<ConfigManager> config = ConfigManager::getInstance();
    Ref<Dictionary> mappings = config->getMimeToContentTypeMappings();
    String content_type = mappings->get(item->getMimeType());
#endif

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
#ifdef EXTEND_PROTOCOLINFO
            if (config->getOption(_("/server/protocolInfo/attribute::extend")) == "yes")
            {
                prot = res_attrs->get(MetadataHandler::getResAttrName(R_PROTOCOLINFO));
          
                String extend;
                if (content_type == CONTENT_TYPE_MP3)
                    extend = _(D_PROFILE) + "=" + D_MP3 + ";";

                extend = extend + D_DEFAULT_OPS + ";" + 
                                  D_DEFAULT_CONVERSION_INDICATOR;
                
                prot = prot.substring(0, prot.rindex(':')+1) + extend;
                log_debug("extended protocolInfo: %s\n", prot.c_str());

                res_attrs->put(MetadataHandler::getResAttrName(R_PROTOCOLINFO),
                        prot);
            }
#endif

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
