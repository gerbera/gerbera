/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    add_object.cc - this file is part of MediaTomb.
    
    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>
    
    Copyright (C) 2006-2010 Gena Batyan <bgeradz@mediatomb.cc>,
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

/// \file add_object.cc

#include "cds_objects.h"
#include "common.h"
#include "content_manager.h"
#include "metadata_handler.h"
#include "pages.h"
#include "server.h"
#include "tools.h"
#include <cstdio>

using namespace zmm;
using namespace mxml;

web::addObject::addObject()
    : WebRequestHandler()
{
}

/*static Ref<Element> addOption(String option_name, String option_type, String default_value = nullptr)
{
    Ref<Element> option (new Element(_("option")));
    option->addAttribute(_("name"), option_name);
    option->addAttribute(_("type"), option_type);
    
    if (default_value != nullptr)
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

    return RefCast(item, CdsObject);
}

Ref<CdsObject> web::addObject::addActiveItem(int parentID)
{
    String tmp;
    Ref<CdsActiveItem> item(new CdsActiveItem());

    item->setAction(param(_("action")));

    /// \todo is there a default setting? autoscan? import settings?
    tmp = param(_("state"));
    if (string_ok(tmp))
        item->setState(tmp);

    item->setParentID(parentID);
    item->setLocation(param(_("location")));

    tmp = param(_("mime-type"));
    if (!string_ok(tmp))
        tmp = _(MIMETYPE_DEFAULT);
    item->setMimeType(tmp);

    MetadataHandler::setMetadata(RefCast(item, CdsItem));

    item->setTitle(param(_("title")));
    item->setClass(param(_("class")));

    tmp = param(_("description"));
    if (string_ok(tmp))
        item->setMetadata(MetadataHandler::getMetaFieldName(M_DESCRIPTION), tmp);

    /// \todo is there a default setting? autoscan? import settings?

    //    Ref<CdsResource> resource(new CdsResource(CH_DEFAULT));

    //    Ref<CdsResource> resource = item->getResource(0); // added by m-handler
    //    resource->addAttribute(MetadataHandler::getResAttrName(R_PROTOCOLINFO),
    //                           renderProtocolInfo(tmp));
    //    item->addResource(resource);

    return RefCast(item, CdsObject);
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

    if (addProtocol) {
        String protocol = param(_("protocol"));
        if (string_ok(protocol))
            protocolInfo = renderProtocolInfo(tmp, protocol);
        else
            protocolInfo = renderProtocolInfo(tmp);
    } else
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

    String obj_type = param(_("obj_type"));
    String location = param(_("location"));

    if (!string_ok(param(_("title"))))
        throw _Exception(_("empty title"));

    if (!string_ok(param(_("class"))))
        throw _Exception(_("empty class"));

    int parentID = intParam(_("parent_id"), 0);

    Ref<CdsObject> obj = nullptr;

    Ref<Element> updateContainerEl;

    bool allow_fifo = false;

    if (obj_type == STRING_OBJECT_TYPE_CONTAINER) {
        this->addContainer(parentID);
        //updateContainerEl = Ref<Element>(new Element(_("updateContainer")));
        //updateContainerEl->setText(parID);
        //updateContainerEl->addAttribute(_("add"), _("1"));
        //root->appendChild(updateContainerEl);
    } else if (obj_type == STRING_OBJECT_TYPE_ITEM) {
        if (!string_ok(location))
            throw _Exception(_("no location given"));
        if (!check_path(location, false))
            throw _Exception(_("file not found"));
        obj = this->addItem(parentID, Ref<CdsItem>(new CdsItem()));
        allow_fifo = true;
    } else if (obj_type == STRING_OBJECT_TYPE_ACTIVE_ITEM) {
        if (!string_ok(param(_("action"))))
            throw _Exception(_("no action given"));
        if (!string_ok(location))
            throw _Exception(_("no location given"));
        if (!check_path(location, false))
            throw _Exception(_("path not found"));
        obj = this->addActiveItem(parentID);
        allow_fifo = true;
    } else if (obj_type == STRING_OBJECT_TYPE_EXTERNAL_URL) {
        if (!string_ok(location))
            throw _Exception(_("No URL given"));
        obj = this->addUrl(parentID, Ref<CdsItemExternalURL>(new CdsItemExternalURL()), true);
    } else if (obj_type == STRING_OBJECT_TYPE_INTERNAL_URL) {
        if (!string_ok(location))
            throw _Exception(_("No URL given"));
        obj = this->addUrl(parentID, Ref<CdsItemExternalURL>(new CdsItemInternalURL()), false);
    } else {
        throw _Exception(_("unknown object type: ") + obj_type.c_str());
    }

    if (obj != nullptr) {
        obj->setVirtual(true);
        if (obj_type == STRING_OBJECT_TYPE_ITEM) {
            ContentManager::getInstance()->addVirtualItem(obj, allow_fifo);
        } else {
            ContentManager::getInstance()->addObject(obj);
        }
    }
}
