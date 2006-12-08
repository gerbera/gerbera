/*MT*
    
    MediaTomb - http://www.mediatomb.org/
    
    add_object.cc - this file is part of MediaTomb.
    
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

/// \file add_object.cc

#ifdef HAVE_CONFIG_H
    #include "autoconfig.h"
#endif

#include "server.h"
#include <stdio.h>
#include "common.h"
#include "content_manager.h"
#include "cds_objects.h"
#include "pages.h"
#include "tools.h"
#include "metadata_handler.h"

using namespace zmm;
using namespace mxml;

web::addObject::addObject() : WebRequestHandler()
{}

/*static Ref<Element> addOption(String option_name, String option_type, String default_value = nil)
{
    Ref<Element> option (new Element(_("option")));
    option->addAttribute(_("name"), option_name);
    option->addAttribute(_("type"), option_type);
    
    if (default_value != nil)
        option->addAttribute(_("default"), default_value);
    
    return option;
}*/

void web::addObject::addContainer(int parentID)
{
    ContentManager::getInstance()->addContainer(parentID, param(_("title")), param(_("class")));
}

Ref<CdsObject> web::addObject::addItem(int parentID, Ref<CdsItem> item)
{
    String tmp;
    
    item->setParentID(parentID);
    
    item->setTitle(param(_("title")));
    item->setLocation(param(_("location")));
    item->setClass(param(_("class")));
    
    tmp = param(_("description"));
    if (string_ok(tmp))
        item->setMetadata(MetadataHandler::getMetaFieldName(M_DESCRIPTION), tmp);
    
    /// \todo is there a default setting? autoscan? import settings?
    tmp = param(_("mime-type"));
    if (!string_ok(tmp))
        tmp = _(MIMETYPE_DEFAULT);
    item->setMimeType(tmp);

    item->setFlag(OBJECT_FLAG_USE_RESOURCE_REF);
/*
    Ref<CdsResource> resource(new CdsResource(CH_DEFAULT));
    resource->addAttribute(MetadataHandler::getResAttrName(R_PROTOCOLINFO),
                           renderProtocolInfo(tmp));
    item->addResource(resource);
*/   
    return RefCast(item, CdsObject);
}

Ref<CdsObject> web::addObject::addActiveItem(int parentID)
{
    String tmp;
    Ref<CdsActiveItem> item (new CdsActiveItem());
    
    item->setAction(param(_("action")));
    
    /// \todo is there a default setting? autoscan? import settings?
    tmp = param(_("state"));
    if (string_ok(tmp))
        item->setState(tmp);
    
    return addItem(parentID, RefCast(item, CdsItem));
}

Ref<CdsObject> web::addObject::addUrl(int parentID, Ref<CdsItemExternalURL> item, bool addProtocol)
{
    String tmp;
    String protocolInfo;
    
    item->setParentID(parentID);
    
    item->setTitle(param(_("title")));
    item->setURL(param(_("location")));
    item->setClass(param(_("class")));
    
    tmp = param(_("description"));
    if (string_ok(tmp))
        item->setMetadata(MetadataHandler::getMetaFieldName(M_DESCRIPTION), tmp);
    
    /// \todo is there a default setting? autoscan? import settings?
    tmp = param(_("mime-type"));
    if (!string_ok(tmp))
        tmp = _(MIMETYPE_DEFAULT);
    item->setMimeType(tmp);
    
    if (addProtocol)
    {
        String protocol = param(_("protocol"));
        if (string_ok(protocol))
            protocolInfo = renderProtocolInfo(tmp, protocol);
        else protocolInfo = renderProtocolInfo(tmp);
    }
    else
        protocolInfo = renderProtocolInfo(tmp);
    
    Ref<CdsResource> resource(new CdsResource(CH_DEFAULT));
    resource->addAttribute(MetadataHandler::getResAttrName(R_PROTOCOLINFO),
                           protocolInfo);
    item->addResource(resource);
    
    return RefCast(item, CdsObject);
}

void web::addObject::process()
{
    check_request();
    
    String obj_type = param(_("objType"));
    String location = param(_("location"));
    
    if (!string_ok(param(_("title"))))
        throw _Exception(_("empty title"));
    
    if (!string_ok(param(_("class"))))
        throw _Exception(_("empty class"));
    
    int parentID = intParam(_("parent_id"), 0);
    
    Ref<CdsObject> obj = nil;
    
    Ref<Element> updateContainerEl;
    
    switch (obj_type.toInt())
    {
        case OBJECT_TYPE_CONTAINER:
            this->addContainer(parentID);
            //updateContainerEl = Ref<Element>(new Element(_("updateContainer")));
            //updateContainerEl->setText(parID);
            //updateContainerEl->addAttribute(_("add"), _("1"));
            //root->appendChild(updateContainerEl);
            break;
            
        case OBJECT_TYPE_ITEM:
            if (!string_ok(location)) throw _Exception(_("no location given"));
            if (!check_path(location, false)) throw _Exception(_("file not found"));
            obj = this->addItem(parentID, Ref<CdsItem> (new CdsItem()));
            break;
            
        case OBJECT_TYPE_ITEM | OBJECT_TYPE_ACTIVE_ITEM:
            if (!string_ok(param(_("action")))) throw _Exception(_("no action given"));
            if (!string_ok(location)) throw _Exception(_("no location given"));
            if (!check_path(location, false))
                throw _Exception(_("path not found"));
            obj = this->addActiveItem(parentID);
            break;
            
        case OBJECT_TYPE_ITEM | OBJECT_TYPE_ITEM_EXTERNAL_URL:
            if (!string_ok(location)) throw _Exception(_("No URL given"));
            obj = this->addUrl(parentID, Ref<CdsItemExternalURL> (new CdsItemExternalURL()), true);
            break;
            
        case OBJECT_TYPE_ITEM | OBJECT_TYPE_ITEM_EXTERNAL_URL | OBJECT_TYPE_ITEM_INTERNAL_URL:
            if (!string_ok(location)) throw _Exception(_("No URL given"));
            obj = this->addUrl(parentID, Ref<CdsItemExternalURL> (new CdsItemInternalURL()), false);
            break;
            
        default:
            throw _Exception(_("unknown object type: ") + obj_type.c_str());
            break;
    }
    if (obj != nil)
    {
        obj->setVirtual(true);
        if (obj_type.toInt() == OBJECT_TYPE_ITEM)
        {
            ContentManager::getInstance()->addVirtualItem(obj);
        }
        else
        {
            ContentManager::getInstance()->addObject(obj);
        }
    }
}

