/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    upnp_xml.cc - this file is part of MediaTomb.
    
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

/// \file upnp_xml.cc

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
    response->setAttribute(_("xmlns:u"), serviceType);

    return response; 
}

Ref<Element> UpnpXML_DIDLRenderObject(Ref<CdsObject> obj, bool renderActions, int stringLimit)
{
    Ref<Element> result(new Element(_("")));
    
    result->setAttribute(_("id"), String::from(obj->getID()));
    result->setAttribute(_("parentID"), String::from(obj->getParentID()));
    result->setAttribute(_("restricted"), obj->isRestricted() ? _("1") : _("0"));
   
    String tmp = obj->getTitle();

    if ((stringLimit > 0) && (tmp.length() > stringLimit))
    {
        tmp = tmp.substring(0, getValidUTF8CutPosition(tmp, stringLimit-3));
        tmp = tmp + _("...");
    }
   
    result->appendTextChild(_("dc:title"), tmp);
    
    result->appendTextChild(_("upnp:class"), obj->getClass());
    
    int objectType = obj->getObjectType();
    if (IS_CDS_ITEM(objectType))
    {
        Ref<CdsItem> item = RefCast(obj, CdsItem);
        
        Ref<Dictionary> meta = obj->getMetadata();
        Ref<Array<DictionaryElement> > elements = meta->getElements();
        int len = elements->size();
        
        String key;
        String upnp_class = obj->getClass();
        
        for (int i = 0; i < len; i++)
        {
            Ref<DictionaryElement> el = elements->get(i);
            key = el->getKey();
            if (key == MetadataHandler::getMetaFieldName(M_DESCRIPTION))
            {
                tmp = el->getValue();
                if ((stringLimit > 0) && (tmp.length() > stringLimit))
                {
                    tmp = tmp.substring(0, 
                            getValidUTF8CutPosition(tmp, stringLimit-3));
                    tmp = tmp + _("...");
                }
                result->appendTextChild(key, tmp);
            }
            else if (key == MetadataHandler::getMetaFieldName(M_TRACKNUMBER))
            {
                if (upnp_class == UPNP_DEFAULT_CLASS_MUSIC_TRACK)
                    result->appendTextChild(key, el->getValue());
            }
            else if ((key != MetadataHandler::getMetaFieldName(M_TITLE)) || 
                    ((key == MetadataHandler::getMetaFieldName(M_TRACKNUMBER)) && 
                     (upnp_class == UPNP_DEFAULT_CLASS_MUSIC_TRACK)))
                result->appendTextChild(key, el->getValue());
        }
        
        log_debug("ITEM HAS FOLLOWING METADATA: %s\n", item->getMetadata()->encode().c_str());
        
        CdsResourceManager::addResources(item, result);
        
        result->setName(_("item"));
    }
    else if (IS_CDS_CONTAINER(objectType))
    {
        Ref<CdsContainer> cont = RefCast(obj, CdsContainer);
        
        result->setName(_("container"));
        int childCount = cont->getChildCount();
        if (childCount >= 0)
            result->setAttribute(_("childCount"), String::from(childCount));

        String upnp_class = obj->getClass();
        log_debug("container is class: %s\n", upnp_class.c_str());
        if (upnp_class == UPNP_DEFAULT_CLASS_MUSIC_ALBUM) {
            Ref<Dictionary> meta = obj->getMetadata();
            Ref<Array<DictionaryElement> > elements = meta->getElements();
            int len = elements->size();
            String key;

            log_debug("Album as %d metadata\n", len);

            for (int i = 0; i < len; i++)
            {
                Ref<DictionaryElement> el = elements->get(i);
                key = el->getKey();
                log_debug("Container %s\n", key.c_str());
                if (key == MetadataHandler::getMetaFieldName(M_ARTIST)) {
                    result->appendElementChild(UpnpXML_DIDLRenderCreator(el->getValue()));
                }
            }


        }
        
    }
    
    if (renderActions && IS_CDS_ACTIVE_ITEM(objectType))
    {
        Ref<CdsActiveItem> aitem = RefCast(obj, CdsActiveItem);
        result->appendTextChild(_("action"), aitem->getAction());
        result->appendTextChild(_("state"), aitem->getState());
        result->appendTextChild(_("location"), aitem->getLocation());
        result->appendTextChild(_("mime-type"), aitem->getMimeType());
    }
   
//    log_debug("renderen DIDL: \n%s\n", result->print().c_str());

    return result;
}

void UpnpXML_DIDLUpdateObject(Ref<CdsObject> obj, String text)
{
    Ref<Parser> parser(new Parser());
    Ref<Element> root = parser->parseString(text)->getRoot();
    int objectType = obj->getObjectType();
    
    if (IS_CDS_ACTIVE_ITEM(objectType))
    {
        Ref<CdsActiveItem> aitem = RefCast(obj, CdsActiveItem);

        String title = root->getChildText(_("dc:title"));
        if (title != nil && title != "")
            aitem->setTitle(title);

        /// \todo description should be taken from the dictionary      
        String description = root->getChildText(_("dc:description"));
        if (description == nil)
            description = _("");
        aitem->setMetadata(MetadataHandler::getMetaFieldName(M_DESCRIPTION),
                    description);

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
    propset->setAttribute(_("xmlns:e"), _("urn:schemas-upnp-org:event-1-0"));

    Ref<Element> property(new Element(_("e:property")));

    propset->appendElementChild(property);
    return propset;
}

Ref<Element> UpnpXML_RenderDeviceDescription(String presentationURL)
{

    log_debug("start\n");
    Ref<ConfigManager> config = ConfigManager::getInstance();

    Ref<Element> root(new Element(_("root"))); 
    root->setAttribute(_("xmlns"), _(DESC_DEVICE_NAMESPACE));
     
    Ref<Element> specVersion(new Element(_("specVersion")));
    specVersion->appendTextChild(_("major"), _(DESC_SPEC_VERSION_MAJOR));
    specVersion->appendTextChild(_("minor"), _(DESC_SPEC_VERSION_MINOR));

    root->appendElementChild(specVersion);

    Ref<Element> device(new Element(_("device")));
    
#ifdef EXTEND_PROTOCOLINFO 
    if (config->getBoolOption(CFG_SERVER_EXTEND_PROTOCOLINFO))
    {
        //we do not do DLNA yet but this is needed for bravia tv (v5500)
        Ref<Element> DLNADOC(new Element(_("dlna:X_DLNADOC")));
        DLNADOC->setText(_("DMS-1.50"));
//      DLNADOC->setText(_("M-DMS-1.50"));
        DLNADOC->setAttribute(_("xmlns:dlna"), 
                                        _("urn:schemas-dlna-org:device-1-0"));
        device->appendElementChild(DLNADOC);
    }
#endif

    device->appendTextChild(_("deviceType"), _(DESC_DEVICE_TYPE));
    if (!string_ok(presentationURL))
        device->appendTextChild(_("presentationURL"), _("/"));
    else
        device->appendTextChild(_("presentationURL"), presentationURL);

    device->appendTextChild(_("friendlyName"), 
                              config->getOption(CFG_SERVER_NAME));

    device->appendTextChild(_("manufacturer"), _(DESC_MANUFACTURER));
    device->appendTextChild(_("manufacturerURL"), 
                               config->getOption(CFG_SERVER_MANUFACTURER_URL));
    device->appendTextChild(_("modelDescription"), 
                              config->getOption(CFG_SERVER_MODEL_DESCRIPTION));
    device->appendTextChild(_("modelName"), 
                              config->getOption(CFG_SERVER_MODEL_NAME));
    device->appendTextChild(_("modelNumber"), 
                              config->getOption(CFG_SERVER_MODEL_NUMBER));
    device->appendTextChild(_("serialNumber"), 
                              config->getOption(CFG_SERVER_SERIAL_NUMBER));
    device->appendTextChild(_("UDN"), config->getOption(CFG_SERVER_UDN));

    Ref<Element> iconList(new Element(_("iconList")));

    Ref<Element> icon120_png(new Element(_("icon")));
    icon120_png->appendTextChild(_("mimetype"), _(DESC_ICON_PNG_MIMETYPE));
    icon120_png->appendTextChild(_("width"), _("120"));
    icon120_png->appendTextChild(_("height"), _("120"));
    icon120_png->appendTextChild(_("depth"), _("24"));
    icon120_png->appendTextChild(_("url"), _(DESC_ICON120_PNG));
    iconList->appendElementChild(icon120_png);

    Ref<Element> icon120_bmp(new Element(_("icon")));
    icon120_bmp->appendTextChild(_("mimetype"), _(DESC_ICON_BMP_MIMETYPE));
    icon120_bmp->appendTextChild(_("width"), _("120"));
    icon120_bmp->appendTextChild(_("height"), _("120"));
    icon120_bmp->appendTextChild(_("depth"), _("24"));
    icon120_bmp->appendTextChild(_("url"), _(DESC_ICON120_BMP));
    iconList->appendElementChild(icon120_bmp);

    Ref<Element> icon120_jpg(new Element(_("icon")));
    icon120_jpg->appendTextChild(_("mimetype"), _(DESC_ICON_JPG_MIMETYPE));
    icon120_jpg->appendTextChild(_("width"), _("120"));
    icon120_jpg->appendTextChild(_("height"), _("120"));
    icon120_jpg->appendTextChild(_("depth"), _("24"));
    icon120_jpg->appendTextChild(_("url"), _(DESC_ICON120_JPG));
    iconList->appendElementChild(icon120_jpg);

    Ref<Element> icon48_png(new Element(_("icon")));
    icon48_png->appendTextChild(_("mimetype"), _(DESC_ICON_PNG_MIMETYPE));
    icon48_png->appendTextChild(_("width"), _("48"));
    icon48_png->appendTextChild(_("height"), _("48"));
    icon48_png->appendTextChild(_("depth"), _("24"));
    icon48_png->appendTextChild(_("url"), _(DESC_ICON48_PNG));
    iconList->appendElementChild(icon48_png);

    Ref<Element> icon48_bmp(new Element(_("icon")));
    icon48_bmp->appendTextChild(_("mimetype"), _(DESC_ICON_BMP_MIMETYPE));
    icon48_bmp->appendTextChild(_("width"), _("48"));
    icon48_bmp->appendTextChild(_("height"), _("48"));
    icon48_bmp->appendTextChild(_("depth"), _("24"));
    icon48_bmp->appendTextChild(_("url"), _(DESC_ICON48_BMP));
    iconList->appendElementChild(icon48_bmp);

    Ref<Element> icon48_jpg(new Element(_("icon")));
    icon48_jpg->appendTextChild(_("mimetype"), _(DESC_ICON_JPG_MIMETYPE));
    icon48_jpg->appendTextChild(_("width"), _("48"));
    icon48_jpg->appendTextChild(_("height"), _("48"));
    icon48_jpg->appendTextChild(_("depth"), _("24"));
    icon48_jpg->appendTextChild(_("url"), _(DESC_ICON48_JPG));
    iconList->appendElementChild(icon48_jpg);

    Ref<Element> icon32_png(new Element(_("icon")));
    icon32_png->appendTextChild(_("mimetype"), _(DESC_ICON_PNG_MIMETYPE));
    icon32_png->appendTextChild(_("width"), _("32"));
    icon32_png->appendTextChild(_("height"), _("32"));
    icon32_png->appendTextChild(_("depth"), _("8"));
    icon32_png->appendTextChild(_("url"), _(DESC_ICON32_PNG));
    iconList->appendElementChild(icon32_png);

    Ref<Element> icon32_bmp(new Element(_("icon")));
    icon32_bmp->appendTextChild(_("mimetype"), _(DESC_ICON_BMP_MIMETYPE));
    icon32_bmp->appendTextChild(_("width"), _("32"));
    icon32_bmp->appendTextChild(_("height"), _("32"));
    icon32_bmp->appendTextChild(_("depth"), _("8"));
    icon32_bmp->appendTextChild(_("url"), _(DESC_ICON32_BMP));
    iconList->appendElementChild(icon32_bmp);

    Ref<Element> icon32_jpg(new Element(_("icon")));
    icon32_jpg->appendTextChild(_("mimetype"), _(DESC_ICON_JPG_MIMETYPE));
    icon32_jpg->appendTextChild(_("width"), _("32"));
    icon32_jpg->appendTextChild(_("height"), _("32"));
    icon32_jpg->appendTextChild(_("depth"), _("8"));
    icon32_jpg->appendTextChild(_("url"), _(DESC_ICON32_JPG));
    iconList->appendElementChild(icon32_jpg);

    device->appendElementChild(iconList);

    Ref<Element> serviceList(new Element(_("serviceList")));

    Ref<Element> serviceCM(new Element(_("service")));
    serviceCM->appendTextChild(_("serviceType"), _(DESC_CM_SERVICE_TYPE));
    serviceCM->appendTextChild(_("serviceId"), _(DESC_CM_SERVICE_ID));
    serviceCM->appendTextChild(_("SCPDURL"), _(DESC_CM_SCPD_URL));
    serviceCM->appendTextChild(_("controlURL"), _(DESC_CM_CONTROL_URL));
    serviceCM->appendTextChild(_("eventSubURL"), _(DESC_CM_EVENT_URL));

    serviceList->appendElementChild(serviceCM);

    Ref<Element> serviceCDS(new Element(_("service")));
    serviceCDS->appendTextChild(_("serviceType"), _(DESC_CDS_SERVICE_TYPE));
    serviceCDS->appendTextChild(_("serviceId"), _(DESC_CDS_SERVICE_ID));
    serviceCDS->appendTextChild(_("SCPDURL"), _(DESC_CDS_SCPD_URL));
    serviceCDS->appendTextChild(_("controlURL"), _(DESC_CDS_CONTROL_URL));
    serviceCDS->appendTextChild(_("eventSubURL"), _(DESC_CDS_EVENT_URL));

    serviceList->appendElementChild(serviceCDS);

#if defined(ENABLE_MRREG)
    // media receiver registrar service for the Xbox 360
    Ref<Element> serviceMRREG(new Element(_("service")));
    serviceMRREG->appendTextChild(_("serviceType"), _(DESC_MRREG_SERVICE_TYPE));
    serviceMRREG->appendTextChild(_("serviceId"), _(DESC_MRREG_SERVICE_ID));
    serviceMRREG->appendTextChild(_("SCPDURL"), _(DESC_MRREG_SCPD_URL));
    serviceMRREG->appendTextChild(_("controlURL"), _(DESC_MRREG_CONTROL_URL));
    serviceMRREG->appendTextChild(_("eventSubURL"), _(DESC_MRREG_EVENT_URL));

    serviceList->appendElementChild(serviceMRREG);
#endif    
    
    device->appendElementChild(serviceList);

    root->appendElementChild(device);

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
        res->setAttribute(attribute, el->getValue());
    }

    return res;
}

Ref<Element> UpnpXML_DIDLRenderCaptionInfo(String URL) {
    Ref<Element> cap(new Element(_("sec:CaptionInfoEx")));

    // Samsung DLNA clients don't follow this URL and
    // obtain subtitle location from video HTTP headers.
    // We don't need to know here what the subtitle type
    // is and even if there is a subtitle.
    // This tag seems to be only a hint for Samsung devices,
    // though it's necessary.

    int endp = URL.rindex('.');
    cap->setText(URL.substring(0, endp) + ".srt");
    cap->setAttribute(_("sec:type"), _("srt"));

    return cap;
}

Ref<Element> UpnpXML_DIDLRenderCreator(String creator) {
    Ref<Element> out(new Element(_("dc:creator")));

    out->setText(creator);

    return out;
}
