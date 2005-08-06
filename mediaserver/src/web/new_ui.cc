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
    Ref<Element> option (new Element("option"));
    option->addAttribute("name", option_name);
    option->addAttribute("type", option_type);

    if (default_value != nil)
        option->addAttribute("default", default_value);

    return option;
}

void web::new_ui::add_container()
{
    String tmp;

    Ref<CdsContainer> cont (new CdsContainer());
    cont->setParentID(param("object_id"));
    cont->setTitle(param("title"));

    tmp = param("location");
    if (tmp != nil)
        cont->setLocation(tmp);
    else
        cont->setLocation("");

    tmp = param("class");
    if (string_ok(tmp))
        cont->setClass(tmp);

    Ref<CdsObject> obj = RefCast(cont, CdsObject);
    
    Ref<ContentManager> cm = ContentManager::getInstance();

    cm->addObject(obj);
}

void web::new_ui::add_item()
{
    String tmp;
    Ref<CdsItem> item (new CdsItem());
    
    item->setParentID(param("object_id"));
    item->setTitle(param("title"));
    item->setLocation(param("location"));

    tmp = param("class");
    if (string_ok(tmp))
        item->setClass(tmp);

    tmp = param("description");
    if (string_ok(tmp))
        item->setMetadata(MetadataHandler::getMetaFieldName(M_DESCRIPTION), tmp);

    /// \todo is there a default setting? autoscan? import settings?
    tmp = param("mime-type");
    if (!string_ok(tmp))
    {  
        tmp = MIMETYPE_DEFAULT;
    }

    item->setMimeType(tmp);
    item->addResource();
    item->setResource(0, MetadataHandler::getResAttrName(R_PROTOCOLINFO), renderProtocolInfo(tmp));
   
    Ref<CdsObject> obj = RefCast(item, CdsObject);
    
    Ref<ContentManager> cm = ContentManager::getInstance();

    cm->addObject(obj);

}

void web::new_ui::add_url()
{
    String tmp;
    Ref<CdsItemExternalURL> item (new CdsItemExternalURL());
    
    item->setParentID(param("object_id"));
    item->setTitle(param("title"));
    item->setURL(param("location"));

    tmp = param("class");
    if (string_ok(tmp))
        item->setClass(tmp);

    tmp = param("description");
    if (string_ok(tmp))
        item->setMetadata(MetadataHandler::getMetaFieldName(M_DESCRIPTION), tmp);

    /// \todo is there a default setting? autoscan? import settings?
    tmp = param("mime-type");
    if (!string_ok(tmp))
    {  
        tmp = MIMETYPE_DEFAULT;
    }

    item->setMimeType(tmp);
    item->addResource();
    item->setResource(0, MetadataHandler::getResAttrName(R_PROTOCOLINFO), renderProtocolInfo(tmp));
  
    Ref<CdsObject> obj = RefCast(item, CdsObject);
    
    Ref<ContentManager> cm = ContentManager::getInstance();

    cm->addObject(obj);

}

void web::new_ui::add_internal_url()
{
    String tmp;   
    Ref<CdsItemInternalURL> item (new CdsItemInternalURL());
    
    item->setParentID(param("object_id"));
    item->setTitle(param("title"));
    item->setURL(param("location"));

    tmp = param("class");
    if (string_ok(tmp))
        item->setClass(tmp);

    tmp = param("description");
    if (string_ok(tmp))
        item->setMetadata(MetadataHandler::getMetaFieldName(M_DESCRIPTION), tmp);

    /// \todo is there a default setting? autoscan? import settings?
    tmp = param("mime-type");
    if (!string_ok(tmp))
    {  
        tmp = MIMETYPE_DEFAULT;
    }

    item->setMimeType(tmp);
    item->addResource();
    item->setResource(0, MetadataHandler::getResAttrName(R_PROTOCOLINFO), renderProtocolInfo(tmp));
   
    Ref<CdsObject> obj = RefCast(item, CdsObject);
    
    Ref<ContentManager> cm = ContentManager::getInstance();

    cm->addObject(obj);

}

void web::new_ui::add_active_item()
{
    String tmp;
    Ref<CdsActiveItem> item (new CdsActiveItem());
    
    item->setParentID(param("object_id"));
    item->setTitle(param("title"));
    item->setLocation(param("location"));
    item->setAction(param("action"));

    tmp = param("class");
    if (string_ok(tmp))
        item->setClass(tmp);

    tmp = param("description");
    if (string_ok(tmp))
        item->setMetadata(MetadataHandler::getMetaFieldName(M_DESCRIPTION), tmp);

    tmp = param("mime-type");
    if (!string_ok(tmp))
    {  
        tmp = MIMETYPE_DEFAULT;
    }

    item->setMimeType(tmp);
    item->addResource();
    item->setResource(0, MetadataHandler::getResAttrName(R_PROTOCOLINFO), renderProtocolInfo(tmp));

    /// \todo is there a default setting? autoscan? import settings?

    tmp = param("state");
    if (string_ok(tmp))
        item->setState(tmp);
    
    Ref<CdsObject> obj = RefCast(item, CdsObject);
    
    Ref<ContentManager> cm = ContentManager::getInstance();

    cm->addObject(obj);
}

void web::new_ui::add_object()
{
    String obj_type = param("type");
    String location = param("location");
    
    if (!string_ok(param("title")))
        throw Exception(String("empty title"));

    switch (obj_type.toInt())
    {
        case OBJECT_TYPE_CONTAINER:
            if ((location != nil) && (location != ""))
            {
                if (!check_path(location, true))
                    throw Exception(String("path not found"));
            }

            this->add_container();
            break;

        case OBJECT_TYPE_ITEM_INTERNAL_URL:
            if (!string_ok(location))
                throw Exception(String("No URL given"));
            this->add_internal_url();
            break;

        case OBJECT_TYPE_ITEM_EXTERNAL_URL:
            if (!string_ok(location))
                throw Exception(String("No URL given"));
            this->add_url();
            break;

        case OBJECT_TYPE_ACTIVE_ITEM:
            if (!string_ok(location))
                throw Exception(String("no location given"));

            if (!check_path(location, false))
                throw Exception(String("path not found"));
            this->add_active_item();
            break;

        case OBJECT_TYPE_ITEM:
            if (!string_ok(location))
                throw Exception(String("no location given"));

            if (!check_path(location, false))
                throw Exception(String("file not found"));
            this->add_item();
            break;

        default:
            throw Exception(String("unknown object type"));
            break;
    }
}

// general idea:
// there will be a special "type" parameter
// when ommited, we will serve a default interface
// when given, we will serve a special inteface to match the object type
void web::new_ui::process()
{
    log_info(("edit: start\n"));

    Ref<Session> session;
    Ref<Storage> storage; // storage instance, will be chosen depending on the driver

    String TYPE_CONTAINER    = String::from(OBJECT_TYPE_CONTAINER);
    String TYPE_ITEM         = String::from(OBJECT_TYPE_ITEM);
    String TYPE_ACTIVE_ITEM  = String::from(OBJECT_TYPE_ACTIVE_ITEM);
    String TYPE_ITEM_EXTERNAL_URL = String::from(OBJECT_TYPE_ITEM_EXTERNAL_URL);
    String TYPE_ITEM_INTERNAL_URL = String::from(OBJECT_TYPE_ITEM_INTERNAL_URL);

    check_request();

    String object_id = param("object_id");
    String driver = param("driver");
    String sid = param("sid");
    String object_type = param("type");

    if (!string_ok(object_id))
        throw Exception(String("invalid object id"));

    if (driver == "1")
    {
        storage = Storage::getInstance();
    }
    else 
    {
        throw Exception(String("adding objects to secondary driver not supported"));
    }

    Ref<Element> root (new Element("root"));
    root->addAttribute("xmlns:dc", "http://purl.org/dc/elements/1.1/");
    root->addAttribute("xmlns:upnp", "urn:schemas-upnp-org:metadata-1-0/upnp/");

    String reload = param("reload");

    if (reload == "0")
    {
        this->add_object();

        Ref<Dictionary> sub(new Dictionary());
        sub->put("object_id", object_id);     
        sub->put("driver", driver);
        sub->put("sid", sid);
        *out << subrequest("browse", sub);

    }
    else
    {
        //    Ref <Element> last_browse  (new Element("last_browse"));
        root->appendTextChild("driver", driver);
        root->appendTextChild("sid", sid);
        root->appendTextChild("object_id", object_id);

        //    root->appendChild(last_browse);

        Ref <Element> select (new Element("select"));
        root->appendChild(select);

        Ref <Element> inputs (new Element("inputs"));
        root->appendChild(inputs);

        select->appendChild(addOption("Container", TYPE_CONTAINER));
        select->appendChild(addOption("Item", TYPE_ITEM));
        select->appendChild(addOption("Active Item", TYPE_ACTIVE_ITEM));
        select->appendChild(addOption("External Link (URL)", TYPE_ITEM_EXTERNAL_URL));
        select->appendChild(addOption("Internal Link (Local URL)", TYPE_ITEM_INTERNAL_URL));

        inputs->appendChild(addOption("Title: ", "title"));

        if ((object_type == TYPE_ITEM) || (object_type == TYPE_ITEM_EXTERNAL_URL) || (object_type == TYPE_ITEM_INTERNAL_URL))
        {
            select->addAttribute("default", object_type);
            if (object_type == TYPE_ITEM)
            {
                inputs->appendChild(addOption("Location: ", "location"));
            }
            else
            {
                inputs->appendChild(addOption("URL: ", "location"));
            }
            inputs->appendChild(addOption("Class: ", "class", "object.item"));
            inputs->appendChild(addOption("Description: ", "description"));
            inputs->appendChild(addOption("Mimetype: ", "mime-type"));
        }
        else if (object_type == TYPE_ACTIVE_ITEM)
        {
            select->addAttribute("default", TYPE_ACTIVE_ITEM);
            inputs->appendChild(addOption("Location: ", "location"));
            inputs->appendChild(addOption("Class: ", "class", "object.item.activeItem"));
            inputs->appendChild(addOption("Description: ", "description"));
            inputs->appendChild(addOption("Mimetype: ", "mime-type"));
            inputs->appendChild(addOption("Action Script: ", "action"));
            inputs->appendChild(addOption("State: ", "state"));

        }
        else
        {
            select->addAttribute("default", TYPE_CONTAINER);
            inputs->appendChild(addOption("Class: ", "class", "object.container"));
        }

        *out << renderXMLHeader("/new.xsl");
        *out << root->print();
    }
    log_info(("edit: returning\n"));
}

