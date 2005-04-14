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

#define DEFAULT_MIMETYPE "*";

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

    String prot("http-get:*:");
    /// \todo resource options must be read from configuration files

    Ref<Dictionary> dict(new Dictionary());
    dict->put(URL_OBJECT_ID, item->getID());

    String urlBase = server->getVirtualURL() + DIR_SEPARATOR + CONTENT_MEDIA_HANDLER + URL_REQUEST_SEPARATOR + dict->encode();

    res = Ref<Element> (new Element("res"));
    String mimeType = item->getMimeType();
    if (!string_ok(mimeType)) mimeType = DEFAULT_MIMETYPE;
    res->addAttribute("protocolInfo", prot + mimeType + ":*");

    int objectType = item->getObjectType();
    if (IS_CDS_ITEM_EXTERNAL_URL(objectType))
    {
        res->setText(item->getLocation());
    }
    else
    { 
        res->setText(urlBase);
    }
    element->appendChild(res);
    
/*    // main resource
    if (item->getMimeType() == "image/jpeg")
    {
        res = Ref<Element> (new Element("res"));
        res->addAttribute("protocolInfo", prot + item->getMimeType() + ":*");
//        res->addAttribute("resolution", "720x576");
        res->setText(urlBase);
        element->appendChild(res);
    }
    else
    {  
        res = Ref<Element> (new Element("res"));
        res->addAttribute("protocolInfo", prot + item->getMimeType() + ":*");
        res->setText(urlBase);
        element->appendChild(res);
    }

    // aux resources
    if (item->getMimeType() == "image/jpeg")
    {
        res = Ref<Element> (new Element("res"));
        res->addAttribute("protocolInfo", prot + item->getMimeType() + ":*");
        res->addAttribute("resolution", "138x104");
        res->setText(urlBase + '?' + "138x104");
        element->appendChild(res);
    }
*/

}

