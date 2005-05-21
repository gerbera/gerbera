/*  upnp_xml.cc - this file is part of MediaTomb.
                                                                                
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

#include "upnp_xml.h"
#include "server.h"
#include "cds_resource_manager.h"
#include "common.h"
#include "config_manager.h"
#include "metadata_reader.h"

using namespace zmm;
using namespace mxml;

Ref<Element> UpnpXML_CreateResponse(String actionName, String serviceType)
{
    Ref<Element> response(new Element(String("u:") + actionName + "Response"));
    response->addAttribute("xmlns:u", serviceType);

    return response; 
}

Ref<Element> UpnpXML_DIDLRenderObject(Ref<CdsObject> obj, bool renderActions)
{
    Ref<Element> result(new Element(""));
       
    result->addAttribute("id", obj->getID());
    result->addAttribute("parentID", obj->getParentID());
    result->addAttribute("restricted", obj->isRestricted() ? String("1") : String("0"));

    result->appendTextChild("dc:title", obj->getTitle());
    result->appendTextChild("upnp:class", obj->getClass());

    int objectType = obj->getObjectType();
    if (IS_CDS_ITEM(objectType))
    {
        Ref<CdsItem> item = RefCast(obj, CdsItem);

        // optional argument
//        if (item->getDescription() != "")
//            result->appendTextChild("dc:description", item->getDescription());

        Ref<MetadataReader> mr(new MetadataReader());
        String meta_value;

        Ref<Dictionary> meta = obj->getMetadata();
        Ref<Array<DictionaryElement> > elements = meta->getElements();
        int len = elements->size();

        String check;
            
        for (int i = 0; i < len; i++)
        {
            Ref<DictionaryElement> el = elements->get(i);
            check = el->getKey();
            if (check != "dc:title")
                result->appendTextChild(check, el->getValue());
        }

        //printf("ITEM HAS FOLLOWING METADATA: %s\n", item->getMetadata()->encode().c_str());


        CdsResourceManager::getInstance()->addResources(item, result);
       
        result->setName("item");
    }
    else if (IS_CDS_CONTAINER(objectType))
    {
        Ref<CdsContainer> cont = RefCast(obj, CdsContainer);
    
        result->setName("container");
        int childCount = cont->getChildCount();
        if (childCount >= 0)
            result->addAttribute("childCount", String::from(childCount));

    }

    if (renderActions && IS_CDS_ACTIVE_ITEM(objectType))
    {
        Ref<CdsActiveItem> aitem = RefCast(obj, CdsActiveItem);
        result->appendTextChild("action", aitem->getAction());
        result->appendTextChild("state", aitem->getState());
        result->appendTextChild("location", aitem->getLocation());
        result->appendTextChild("mime-type", aitem->getMimeType());
    }
   
//    printf("renderen DIDL: %s\n", result->print().c_str());

    return result;
}

void UpnpXML_DIDLUpdateObject(Ref<CdsObject> obj, String text)
{
    Ref<Parser> parser(new Parser());
    Ref<Element> root = parser->parseString(text);
    int objectType = obj->getObjectType();
    
    if (IS_CDS_ACTIVE_ITEM(objectType))
    {
        Ref<CdsActiveItem> aitem = RefCast(obj, CdsActiveItem);

        String title = root->getChildText("dc:title");
        if (title != nil && title != "")
            aitem->setTitle(title);

        /// \todo description should be taken from the dictionary      
/*        String description = root->getChildText("dc:description");
        if (description == nil)
            description = "";
        aitem->setDescription(description);
*/
        String location = root->getChildText("location");
        if (location != nil && location != "")
            aitem->setLocation(location);
    
        String mimeType = root->getChildText("mime-type");
        if (mimeType != nil && mimeType != "")
            aitem->setMimeType(mimeType);

        String action = root->getChildText("action");
        if (action != nil && action != "")
            aitem->setAction(action);

        String state = root->getChildText("state");
        if (state == nil)
            state = "";
        aitem->setState(state);
    }
        
}

Ref<Element> UpnpXML_CreateEventPropertySet()
{
    Ref<Element> propset(new Element("e:propertyset"));
    propset->addAttribute("xmlns:e", "urn:schemas-upnp-org:event-1-0");

    Ref<Element> property(new Element("e:property"));

    propset->appendChild(property);
    return propset;
}

Ref<Element> UpnpXML_RenderDeviceDescription()
{

    printf("UpnpXML_RenderDeviceDescription(): start\n");
    Ref<ConfigManager> config = ConfigManager::getInstance();

    Ref<Element> root(new Element("root")); 
    root->addAttribute("xmlns", DESC_DEVICE_NAMESPACE);
     
    Ref<Element> specVersion(new Element("specVersion"));
    specVersion->appendTextChild("major", DESC_SPEC_VERSION_MAJOR);
    specVersion->appendTextChild("minor", DESC_SPEC_VERSION_MINOR);

    root->appendChild(specVersion);

//    root->appendTextChild("URLBase", "");

    Ref<Element> device(new Element("device"));
    device->appendTextChild("deviceType", DESC_DEVICE_TYPE);
    device->appendTextChild("presentationURL", "/");
    device->appendTextChild("friendlyName", config->getOption("/server/name"));
    device->appendTextChild("manufacturer", DESC_MANUFACTURER);
    device->appendTextChild("manufacturerURL", DESC_MANUFACTURER_URL);
    device->appendTextChild("modelDescription", DESC_MODEL_DESCRIPTION);
    device->appendTextChild("modelName", DESC_MODEL_NAME);
    device->appendTextChild("modelNumber", DESC_MODEL_NUMBER);
    device->appendTextChild("serialNumber", DESC_SERIAL_NUMBER);
    device->appendTextChild("UDN", config->getOption("/server/udn"));

    Ref<Element> serviceList(new Element("serviceList"));

    Ref<Element> serviceCM(new Element("service"));
    serviceCM->appendTextChild("serviceType", DESC_CM_SERVICE_TYPE);
    serviceCM->appendTextChild("serviceId", DESC_CM_SERVICE_ID);
    serviceCM->appendTextChild("SCPDURL", DESC_CM_SCPD_URL);
    serviceCM->appendTextChild("controlURL", DESC_CM_CONTROL_URL);
    serviceCM->appendTextChild("eventSubURL", DESC_CM_EVENT_URL);

    serviceList->appendChild(serviceCM);

    Ref<Element> serviceCDS(new Element("service"));
    serviceCDS->appendTextChild("serviceType", DESC_CDS_SERVICE_TYPE);
    serviceCDS->appendTextChild("serviceId", DESC_CDS_SERVICE_ID);
    serviceCDS->appendTextChild("SCPDURL", DESC_CDS_SCPD_URL);
    serviceCDS->appendTextChild("controlURL", DESC_CDS_CONTROL_URL);
    serviceCDS->appendTextChild("eventSubURL", DESC_CDS_EVENT_URL);

    serviceList->appendChild(serviceCDS);
    
    
    device->appendChild(serviceList);

    root->appendChild(device);

    return root;
}
