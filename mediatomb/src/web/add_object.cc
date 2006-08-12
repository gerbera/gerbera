/*  add_object.cc - this file is part of MediaTomb.
                                                                                
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

Ref<CdsObject> web::addObject::addContainer(int objectID)
{
    Ref<CdsContainer> cont (new CdsContainer());
    
    cont->setParentID(objectID);
    
    cont->setTitle(param(_("title")));
    
    /*
    tmp = param(_("location"));
    if (tmp != nil)
        cont->setLocation(tmp);
    else
        cont->setLocation(_(""));
    */
    
    String class_str = param(_("class"));
    if (string_ok(class_str))
        cont->setClass(class_str);
    
    return RefCast(cont, CdsObject);
}

Ref<CdsObject> web::addObject::addItem(int objectID, Ref<CdsItem> item)
{
    String tmp;
    
    item->setParentID(objectID);
    
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
    
    Ref<CdsResource> resource(new CdsResource(CH_DEFAULT));
    resource->addAttribute(MetadataHandler::getResAttrName(R_PROTOCOLINFO),
                           renderProtocolInfo(tmp));
    item->addResource(resource);
   
    return RefCast(item, CdsObject);
}

Ref<CdsObject> web::addObject::addActiveItem(int objectID)
{
    String tmp;
    Ref<CdsActiveItem> item (new CdsActiveItem());
    
    item->setAction(param(_("action")));
    
    /// \todo is there a default setting? autoscan? import settings?
    tmp = param(_("state"));
    if (string_ok(tmp))
        item->setState(tmp);
    
    return addItem(objectID, RefCast(item, CdsItem));
}

Ref<CdsObject> web::addObject::addUrl(int objectID, Ref<CdsItemExternalURL> item)
{
    String tmp;
    String protocol;
    
    item->setParentID(objectID);
    
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
    
    protocol = param(_("protocol"));
    if (!string_ok(protocol))
        protocol = _(PROTOCOL);
    
    Ref<CdsResource> resource(new CdsResource(CH_DEFAULT));
    resource->addAttribute(MetadataHandler::getResAttrName(R_PROTOCOLINFO),
                           renderProtocolInfo(tmp, protocol));
    item->addResource(resource);
    
    return RefCast(item, CdsObject);
}

void web::addObject::process()
{
    check_request();
    
    String obj_type = param(_("objType"));
    String location = param(_("location"));
    
    if (!string_ok(param(_("title"))))
        throw Exception(_("empty title"));
    
    if (!string_ok(param(_("class"))))
        throw Exception(_("empty class"));
    
    String objID = param(_("object_id"));
    int objectID;
    if (objID == nil)
        objectID = 0;
    else
        objectID = objID.toInt();
    
    Ref<CdsObject> obj;
    
    switch (obj_type.toInt())
    {
        case OBJECT_TYPE_CONTAINER:
            obj = this->addContainer(objectID);
            break;
            
        case OBJECT_TYPE_ITEM:
            if (!string_ok(location)) throw Exception(_("no location given"));
            if (!check_path(location, false)) throw Exception(_("file not found"));
            obj = this->addItem(objectID, Ref<CdsItem> (new CdsItem()));
            break;
            
        case OBJECT_TYPE_ACTIVE_ITEM:
            if (!string_ok(param(_("action")))) throw Exception(_("no action given"));
            if (!string_ok(location)) throw Exception(_("no location given"));
            if (!check_path(location, false))
                throw Exception(_("path not found"));
            obj = this->addActiveItem(objectID);
            break;
            
        case OBJECT_TYPE_ITEM_EXTERNAL_URL:
            if (!string_ok(location)) throw Exception(_("No URL given"));
            obj = this->addUrl(objectID, Ref<CdsItemExternalURL> (new CdsItemExternalURL()));
            break;
            
        case OBJECT_TYPE_ITEM_INTERNAL_URL:
            if (!string_ok(location)) throw Exception(_("No URL given"));
            obj = this->addUrl(objectID, Ref<CdsItemExternalURL> (new CdsItemInternalURL()));
            break;
            
        default:
            throw Exception(_("unknown object type"));
            break;
    }
    ContentManager::getInstance()->addObject(obj);
}

