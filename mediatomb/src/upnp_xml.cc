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

#ifdef HAVE_CONFIG_H
    #include "autoconfig.h"
#endif

#include "upnp_xml.h"
#include "server.h"
#include "cds_resource_manager.h"
#include "common.h"
#include "config_manager.h"
#include "metadata_handler.h"

using namespace zmm;
using namespace mxml;

Ref<Element> UpnpXML_CreateResponse(String actionName, String serviceType)
{
    Ref<Element> response(new Element(_("u:") + actionName +
                                      "Response"));
    response->addAttribute(_("xmlns:u"), serviceType);

    return response; 
}

Ref<Element> UpnpXML_DIDLRenderObject(Ref<CdsObject> obj, bool renderActions)
{
    Ref<Element> result(new Element(_("")));
       
    result->addAttribute(_("id"), String::from(obj->getID()));
    result->addAttribute(_("parentID"), String::from(obj->getParentID()));
    result->addAttribute(_("restricted"), obj->isRestricted() ? _("1") : _("0"));

    result->appendTextChild(_("dc:title"), obj->getTitle());
    result->appendTextChild(_("upnp:class"), obj->getClass());

    int objectType = obj->getObjectType();
    if (IS_CDS_ITEM(objectType))
    {
        Ref<CdsItem> item = RefCast(obj, CdsItem);

        Ref<Dictionary> meta = obj->getMetadata();
        Ref<Array<DictionaryElement> > elements = meta->getElements();
        int len = elements->size();

        String key;
            
        for (int i = 0; i < len; i++)
        {
            Ref<DictionaryElement> el = elements->get(i);
            key = el->getKey();
            if (key != "dc:title")
                result->appendTextChild(key, el->getValue());
        }

        //log_info(("ITEM HAS FOLLOWING METADATA: %s\n", item->getMetadata()->encode().c_str()));


        CdsResourceManager::getInstance()->addResources(item, result);
       
        result->setName(_("item"));
    }
    else if (IS_CDS_CONTAINER(objectType))
    {
        Ref<CdsContainer> cont = RefCast(obj, CdsContainer);
    
        result->setName(_("container"));
        int childCount = cont->getChildCount();
        if (childCount >= 0)
            result->addAttribute(_("childCount"), String::from(childCount));

    }

    if (renderActions && IS_CDS_ACTIVE_ITEM(objectType))
    {
        Ref<CdsActiveItem> aitem = RefCast(obj, CdsActiveItem);
        result->appendTextChild(_("action"), aitem->getAction());
        result->appendTextChild(_("state"), aitem->getState());
        result->appendTextChild(_("location"), aitem->getLocation());
        result->appendTextChild(_("mime-type"), aitem->getMimeType());
    }
   
//    log_info(("renderen DIDL: \n%s\n", result->print().c_str()));

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

        String title = root->getChildText(_("dc:title"));
        if (title != nil && title != "")
            aitem->setTitle(title);

        /// \todo description should be taken from the dictionary      
/*        String description = root->getChildText("dc:description");
        if (description == nil)
            description = "";
        aitem->setDescription(description);
*/
        String location = root->getChildText(_("location"));
        if (location != nil && location != "")
            aitem->setLocation(location);
    
        String mimeType = root->getChildText(_("mime-type"));
        if (mimeType != nil && mimeType != "")
            aitem->setMimeType(mimeType);

        String action = root->getChildText(_("action"));
        if (action != nil && action != "")
            aitem->setAction(action);

        String state = root->getChildText(_("state"));
        if (state == nil)
            state = _("");
        aitem->setState(state);
    }
        
}

Ref<Element> UpnpXML_CreateEventPropertySet()
{
    Ref<Element> propset(new Element(_("e:propertyset")));
    propset->addAttribute(_("xmlns:e"), _("urn:schemas-upnp-org:event-1-0"));

    Ref<Element> property(new Element(_("e:property")));

    propset->appendChild(property);
    return propset;
}

Ref<Element> UpnpXML_RenderDeviceDescription()
{

//    log_info(("UpnpXML_RenderDeviceDescription(): start\n"));
    Ref<ConfigManager> config = ConfigManager::getInstance();

    Ref<Element> root(new Element(_("root"))); 
    root->addAttribute(_("xmlns"), _(DESC_DEVICE_NAMESPACE));
     
    Ref<Element> specVersion(new Element(_("specVersion")));
    specVersion->appendTextChild(_("major"), _(DESC_SPEC_VERSION_MAJOR));
    specVersion->appendTextChild(_("minor"), _(DESC_SPEC_VERSION_MINOR));

    root->appendChild(specVersion);

//    root->appendTextChild("URLBase", "");

    Ref<Element> device(new Element(_("device")));
    device->appendTextChild(_("deviceType"), _(DESC_DEVICE_TYPE));
    device->appendTextChild(_("presentationURL"), _("/"));
    device->appendTextChild(_("friendlyName"), config->getOption(_("/server/name")));
    device->appendTextChild(_("manufacturer"), _(DESC_MANUFACTURER));
    device->appendTextChild(_("manufacturerURL"), _(DESC_MANUFACTURER_URL));
    device->appendTextChild(_("modelDescription"), _(DESC_MODEL_DESCRIPTION));
    device->appendTextChild(_("modelName"), _(DESC_MODEL_NAME));
    device->appendTextChild(_("modelNumber"), _(DESC_MODEL_NUMBER));
    device->appendTextChild(_("serialNumber"), _(DESC_SERIAL_NUMBER));
    device->appendTextChild(_("UDN"), config->getOption(_("/server/udn")));

    Ref<Element> serviceList(new Element(_("serviceList")));

    Ref<Element> serviceCM(new Element(_("service")));
    serviceCM->appendTextChild(_("serviceType"), _(DESC_CM_SERVICE_TYPE));
    serviceCM->appendTextChild(_("serviceId"), _(DESC_CM_SERVICE_ID));
    serviceCM->appendTextChild(_("SCPDURL"), _(DESC_CM_SCPD_URL));
    serviceCM->appendTextChild(_("controlURL"), _(DESC_CM_CONTROL_URL));
    serviceCM->appendTextChild(_("eventSubURL"), _(DESC_CM_EVENT_URL));

    serviceList->appendChild(serviceCM);

    Ref<Element> serviceCDS(new Element(_("service")));
    serviceCDS->appendTextChild(_("serviceType"), _(DESC_CDS_SERVICE_TYPE));
    serviceCDS->appendTextChild(_("serviceId"), _(DESC_CDS_SERVICE_ID));
    serviceCDS->appendTextChild(_("SCPDURL"), _(DESC_CDS_SCPD_URL));
    serviceCDS->appendTextChild(_("controlURL"), _(DESC_CDS_CONTROL_URL));
    serviceCDS->appendTextChild(_("eventSubURL"), _(DESC_CDS_EVENT_URL));

    serviceList->appendChild(serviceCDS);
    
    
    device->appendChild(serviceList);

    root->appendChild(device);

    return root;
}

Ref<Element> UpnpXML_DIDLRenderResource(String URL, Ref<Dictionary> attributes)
{
    Ref<Element> res(new Element(_("res")));

    res->setText(URL);

    Ref<Array<DictionaryElement> > elements = attributes->getElements();
    int len = elements->size();

    String attribute;

    for (int i = 0; i < len; i++)
    {
        Ref<DictionaryElement> el = elements->get(i);
        attribute = el->getKey();
        res->addAttribute(attribute, el->getValue());
    }

    return res;
}

