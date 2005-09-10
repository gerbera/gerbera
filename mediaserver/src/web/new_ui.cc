/*  new_ui.cc - this file is part of MediaTomb.
                                                                                
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
#include "tools.h"
#include "metadata_handler.h"

using namespace zmm;
using namespace mxml;

web::new_ui::new_ui() : WebRequestHandler()
{}

static Ref<Element> addOption(String option_name, String option_type, String default_value = nil)
{
    Ref<Element> option (new Element(_("option")));
    option->addAttribute(_("name"), option_name);
    option->addAttribute(_("type"), option_type);

    if (default_value != nil)
        option->addAttribute(_("default"), default_value);

    return option;
}

void web::new_ui::addContainer()
{
    String tmp;

    Ref<CdsContainer> cont (new CdsContainer());

    String objID = param(_("object_id"));
    int objectID;
    if (objID == nil)
        objectID = 0;
    else
        objectID = objID.toInt();
    cont->setParentID(objectID);

    cont->setTitle(param(_("title")));

    tmp = param(_("location"));
    if (tmp != nil)
        cont->setLocation(tmp);
    else
        cont->setLocation(_(""));

    tmp = param(_("class"));
    if (string_ok(tmp))
        cont->setClass(tmp);

    Ref<CdsObject> obj = RefCast(cont, CdsObject);
    
    Ref<ContentManager> cm = ContentManager::getInstance();

    cm->addObject(obj);
}

void web::new_ui::addItem()
{
    String tmp;
    Ref<CdsItem> item (new CdsItem());

    String objID = param(_("object_id"));
    int objectID;
    if (objID == nil)
        objectID = 0;
    else
        objectID = objID.toInt();
    item->setParentID(objectID);

    item->setTitle(param(_("title")));
    item->setLocation(param(_("location")));

    tmp = param(_("class"));
    if (string_ok(tmp))
        item->setClass(tmp);

    tmp = param(_("description"));
    if (string_ok(tmp))
        item->setMetadata(MetadataHandler::getMetaFieldName(M_DESCRIPTION), tmp);

    /// \todo is there a default setting? autoscan? import settings?
    tmp = param(_("mime-type"));
    if (!string_ok(tmp))
    {  
        tmp = _(MIMETYPE_DEFAULT);
    }

    item->setMimeType(tmp);

    Ref<CdsResource> resource(new CdsResource(CH_DEFAULT));
    resource->addAttribute(MetadataHandler::getResAttrName(R_PROTOCOLINFO),
                           renderProtocolInfo(tmp));
    item->addResource(resource);
   
    Ref<CdsObject> obj = RefCast(item, CdsObject);
    
    Ref<ContentManager> cm = ContentManager::getInstance();

    cm->addObject(obj);

}

void web::new_ui::addUrl()
{
    String tmp;
    String protocol;

    Ref<CdsItemExternalURL> item (new CdsItemExternalURL());

    String objID = param(_("object_id"));
    int objectID;
    if (objID == nil)
        objectID = 0;
    else
        objectID = objID.toInt();
    item->setParentID(objectID);

    item->setTitle(param(_("title")));
    item->setURL(param(_("location")));

    tmp = param(_("class"));
    if (string_ok(tmp))
        item->setClass(tmp);

    tmp = param(_("description"));
    if (string_ok(tmp))
        item->setMetadata(MetadataHandler::getMetaFieldName(M_DESCRIPTION), tmp);

    /// \todo is there a default setting? autoscan? import settings?
    tmp = param(_("mime-type"));
    if (!string_ok(tmp))
    {  
        tmp = _(MIMETYPE_DEFAULT);
    }

    protocol = param(_("protocol"));
    if (!string_ok(protocol))
        protocol = _(PROTOCOL);
    
    item->setMimeType(tmp);

    Ref<CdsResource> resource(new CdsResource(CH_DEFAULT));
    resource->addAttribute(MetadataHandler::getResAttrName(R_PROTOCOLINFO),
                           renderProtocolInfo(tmp, protocol));
    item->addResource(resource);

    Ref<CdsObject> obj = RefCast(item, CdsObject);
    
    Ref<ContentManager> cm = ContentManager::getInstance();

    cm->addObject(obj);

}

void web::new_ui::addInternalUrl()
{
    String tmp; 
    String protocol;
    Ref<CdsItemInternalURL> item (new CdsItemInternalURL());

    String objID = param(_("object_id"));
    int objectID;
    if (objID == nil)
        objectID = 0;
    else
        objectID = objID.toInt();
    item->setParentID(objectID);
    
    item->setTitle(param(_("title")));
    item->setURL(param(_("location")));

    tmp = param(_("class"));
    if (string_ok(tmp))
        item->setClass(tmp);

    tmp = param(_("description"));
    if (string_ok(tmp))
        item->setMetadata(MetadataHandler::getMetaFieldName(M_DESCRIPTION), tmp);

    /// \todo is there a default setting? autoscan? import settings?
    tmp = param(_("mime-type"));
    if (!string_ok(tmp))
    {  
        tmp = _(MIMETYPE_DEFAULT);
    }

    protocol = param(_("protocol"));
    if (!string_ok(protocol))
        protocol = _(PROTOCOL);

    item->setMimeType(tmp);

    Ref<CdsResource> resource(new CdsResource(CH_DEFAULT));
    resource->addAttribute(MetadataHandler::getResAttrName(R_PROTOCOLINFO),
                           renderProtocolInfo(tmp, protocol));
    item->addResource(resource);

    Ref<CdsObject> obj = RefCast(item, CdsObject);
    
    Ref<ContentManager> cm = ContentManager::getInstance();

    cm->addObject(obj);

}

void web::new_ui::addActiveItem()
{
    String tmp;
    Ref<CdsActiveItem> item (new CdsActiveItem());

    String objID = param(_("object_id"));
    int objectID;
    if (objID == nil)
        objectID = 0;
    else
        objectID = objID.toInt();
    item->setParentID(objectID);
    
    item->setTitle(param(_("title")));
    item->setLocation(param(_("locatin")));
    item->setAction(param(_("action")));

    tmp = param(_("class"));
    if (string_ok(tmp))
        item->setClass(tmp);

    tmp = param(_("description"));
    if (string_ok(tmp))
        item->setMetadata(MetadataHandler::getMetaFieldName(M_DESCRIPTION), tmp);

    tmp = param(_("mime-type"));
    if (!string_ok(tmp))
    {  
        tmp = _(MIMETYPE_DEFAULT);
    }

    item->setMimeType(tmp);

    Ref<CdsResource> resource(new CdsResource(CH_DEFAULT));
    resource->addAttribute(MetadataHandler::getResAttrName(R_PROTOCOLINFO),
                           renderProtocolInfo(tmp));
    item->addResource(resource);

    /// \todo is there a default setting? autoscan? import settings?

    tmp = param(_("state"));
    if (string_ok(tmp))
        item->setState(tmp);
    
    Ref<CdsObject> obj = RefCast(item, CdsObject);
    
    Ref<ContentManager> cm = ContentManager::getInstance();

    cm->addObject(obj);
}

void web::new_ui::addObject()
{
    String obj_type = param(_("type"));
    String location = param(_("location"));
    
    if (!string_ok(param(_("title"))))
        throw Exception(_("empty title"));

    switch (obj_type.toInt())
    {
        case OBJECT_TYPE_CONTAINER:
            if ((location != nil) && (location != ""))
            {
                if (!check_path(location, true))
                    throw Exception(_("path not found"));
            }

            this->addContainer();
            break;

        case OBJECT_TYPE_ITEM_INTERNAL_URL:
            if (!string_ok(location))
                throw Exception(_("No URL given"));
            this->addInternalUrl();
            break;

        case OBJECT_TYPE_ITEM_EXTERNAL_URL:
            if (!string_ok(location))
                throw Exception(_("No URL given"));
            this->addUrl();
            break;

        case OBJECT_TYPE_ACTIVE_ITEM:
            if (!string_ok(location))
                throw Exception(_("no location given"));

            if (!check_path(location, false))
                throw Exception(_("path not found"));
            this->addActiveItem();
            break;

        case OBJECT_TYPE_ITEM:
            if (!string_ok(location))
                throw Exception(_("no location given"));

            if (!check_path(location, false))
                throw Exception(_("file not found"));
            this->addItem();
            break;

        default:
            throw Exception(_("unknown object type"));
            break;
    }
}

// general idea:
// there will be a special "type" parameter
// when ommited, we will serve a default interface
// when given, we will serve a special inteface to match the object type
void web::new_ui::process()
{
    check_request();

    log_info(("edit: start\n"));

    Ref<Session> session;
    Ref<Storage> storage; // storage instance, will be chosen depending on the driver

    String TYPE_CONTAINER    = String::from(OBJECT_TYPE_CONTAINER);
    String TYPE_ITEM         = String::from(OBJECT_TYPE_ITEM);
    String TYPE_ACTIVE_ITEM  = String::from(OBJECT_TYPE_ACTIVE_ITEM);
    String TYPE_ITEM_EXTERNAL_URL = String::from(OBJECT_TYPE_ITEM_EXTERNAL_URL);
    String TYPE_ITEM_INTERNAL_URL = String::from(OBJECT_TYPE_ITEM_INTERNAL_URL);

    String object_id = param(_("object_id"));
    String driver = param(_("driver"));
    String sid = param(_("sid"));
    String object_type = param(_("type"));

    if (!string_ok(object_id))
        throw Exception(_("invalid object id"));

    if (driver == "1")
    {
        storage = Storage::getInstance();
    }
    else 
    {
        throw Exception(_("adding objects to secondary driver not supported"));
    }

    Ref<Element> root (new Element(_("root")));
    root->addAttribute(_("xmlns:dc"), _("http://purl.org/dc/elements/1.1/"));
    root->addAttribute(_("xmlns:upnp"), _("urn:schemas-upnp-org:metadata-1-0/upnp/"));

    String reload = param(_("reload"));

    if (reload == "0")
    {
        this->addObject();

        Ref<Dictionary> sub(new Dictionary());
        sub->put(_("object_id"), object_id);     
        sub->put(_("driver"), driver);
        sub->put(_("sid"), sid);
        *out << subrequest(_("browse"), sub);

    }
    else
    {
        //    Ref <Element> last_browse  (new Element("last_browse"));
        root->appendTextChild(_("driver"), driver);
        root->appendTextChild(_("sid"), sid);
        root->appendTextChild(_("object_id"), object_id);

        //    root->appendChild(last_browse);

        Ref <Element> select (new Element(_("select")));
        root->appendChild(select);

        Ref <Element> inputs (new Element(_("inputs")));
        root->appendChild(inputs);

        select->appendChild(addOption(_("Container"), TYPE_CONTAINER));
        select->appendChild(addOption(_("Item"), TYPE_ITEM));
        select->appendChild(addOption(_("Active Item"), TYPE_ACTIVE_ITEM));
        select->appendChild(addOption(_("External Link (URL)"), TYPE_ITEM_EXTERNAL_URL));
        select->appendChild(addOption(_("Internal Link (Local URL)"), TYPE_ITEM_INTERNAL_URL));

        inputs->appendChild(addOption(_("Title: "), _("title")));

        if ((object_type == TYPE_ITEM) || (object_type == TYPE_ITEM_EXTERNAL_URL) || (object_type == TYPE_ITEM_INTERNAL_URL))
        {
            select->addAttribute(_("default"), object_type);
            if (object_type == TYPE_ITEM)
            {
                inputs->appendChild(addOption(_("Location: "), _("location")));
            }
            else
            {
                inputs->appendChild(addOption(_("URL: "), _("location")));
                inputs->appendChild(addOption(_("Protocol: "), _("protocol"), _(PROTOCOL)));
            }
            inputs->appendChild(addOption(_("Class: "), _("class"), _("object.item")));
            inputs->appendChild(addOption(_("Description: "), _("description")));
            inputs->appendChild(addOption(_("Mimetype: "), _("mime-type")));
        }
        else if (object_type == TYPE_ACTIVE_ITEM)
        {
            select->addAttribute(_("default"), TYPE_ACTIVE_ITEM);
            inputs->appendChild(addOption(_("Location: "), _("location")));
            inputs->appendChild(addOption(_("Class: "), _("class"), _("object.item.activeItem")));
            inputs->appendChild(addOption(_("Description: "), _("description")));
            inputs->appendChild(addOption(_("Mimetype: "), _("mime-type")));
            inputs->appendChild(addOption(_("Action Script: "), _("action")));
            inputs->appendChild(addOption(_("State: "), _("state")));

        }
        else
        {
            select->addAttribute(_("default"), TYPE_CONTAINER);
            inputs->appendChild(addOption(_("Class: "), _("class"), _("object.container")));
        }

        *out << renderXMLHeader(_("/new.xsl"));
        *out << root->print();
    }
    log_info(("edit: returning\n"));
}

