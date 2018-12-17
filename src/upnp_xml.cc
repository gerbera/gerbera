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

#include "upnp_xml.h"
#include "cds_resource_manager.h"
#include "common.h"
#include "config_manager.h"
#include "metadata_handler.h"
#include "server.h"

using namespace zmm;
using namespace mxml;

UpnpXMLBuilder::UpnpXMLBuilder(String virtualUrl): virtualUrl(virtualUrl) {};

Ref<Element> UpnpXMLBuilder::createResponse(String actionName, String serviceType)
{
    Ref<Element> response(new Element(_("u:") + actionName + "Response"));
    response->setAttribute(_("xmlns:u"), serviceType);

    return response;
}

Ref<Element> UpnpXMLBuilder::renderObject(Ref<CdsObject> obj, bool renderActions, int stringLimit)
{
    Ref<Element> result(new Element(_("")));

    result->setAttribute(_("id"), String::from(obj->getID()));
    result->setAttribute(_("parentID"), String::from(obj->getParentID()));
    result->setAttribute(_("restricted"), obj->isRestricted() ? _("1") : _("0"));

    String tmp = obj->getTitle();

    if ((stringLimit > 0) && (tmp.length() > stringLimit)) {
        tmp = tmp.substring(0, getValidUTF8CutPosition(tmp, stringLimit - 3));
        tmp = tmp + _("...");
    }

    result->appendTextChild(_("dc:title"), tmp);

    result->appendTextChild(_("upnp:class"), obj->getClass());

    int objectType = obj->getObjectType();
    if (IS_CDS_ITEM(objectType)) {
        Ref<CdsItem> item = RefCast(obj, CdsItem);

        Ref<Dictionary> meta = obj->getMetadata();
        Ref<Array<DictionaryElement>> elements = meta->getElements();
        int len = elements->size();

        String key;
        String upnp_class = obj->getClass();

        for (int i = 0; i < len; i++) {
            Ref<DictionaryElement> el = elements->get(i);
            key = el->getKey();
            if (key == MetadataHandler::getMetaFieldName(M_DESCRIPTION)) {
                tmp = el->getValue();
                if ((stringLimit > 0) && (tmp.length() > stringLimit)) {
                    tmp = tmp.substring(0, getValidUTF8CutPosition(tmp, stringLimit - 3));
                    tmp = tmp + _("...");
                }
                result->appendTextChild(key, tmp);
            } else if (key == MetadataHandler::getMetaFieldName(M_TRACKNUMBER)) {
                if (upnp_class == UPNP_DEFAULT_CLASS_MUSIC_TRACK)
                    result->appendTextChild(key, el->getValue());
            } else if ((key != MetadataHandler::getMetaFieldName(M_TITLE)) || ((key == MetadataHandler::getMetaFieldName(M_TRACKNUMBER)) && (upnp_class == UPNP_DEFAULT_CLASS_MUSIC_TRACK)))
                result->appendTextChild(key, el->getValue());
        }

        CdsResourceManager::addResources(this, item, result);

        if (upnp_class == UPNP_DEFAULT_CLASS_MUSIC_TRACK) {
            Ref<Storage> storage = Storage::getInstance();
            // extract extension-less, lowercase track name to search for corresponding
            // image as cover alternative
            String dctl = item->getTitle().toLower();
            String trackArtBase = String();
            int doti = dctl.rindex('.');
            if (doti >= 0) {
                trackArtBase = dctl.substring(0, doti);
            }
            String aa_id = storage->findFolderImage(item->getParentID(), trackArtBase);
            if (aa_id != nullptr) {
                String url;
                Ref<Dictionary> dict(new Dictionary());
                dict->put(_(URL_OBJECT_ID), aa_id);

                url = virtualUrl + _(_URL_PARAM_SEPARATOR) + CONTENT_MEDIA_HANDLER + _(_URL_PARAM_SEPARATOR) + dict->encodeSimple() + _(_URL_PARAM_SEPARATOR) + _(URL_RESOURCE_ID) + _(_URL_PARAM_SEPARATOR) + "0";
                log_debug("UpnpXMLRenderer::DIDLRenderObject: url: %s\n", url.c_str());
                Ref<Element> aa(new Element(MetadataHandler::getMetaFieldName(M_ALBUMARTURI)));
                aa->setText(url);
                result->appendElementChild(aa);
            }
        }
        result->setName(_("item"));
    } else if (IS_CDS_CONTAINER(objectType)) {
        Ref<CdsContainer> cont = RefCast(obj, CdsContainer);

        result->setName(_("container"));
        int childCount = cont->getChildCount();
        if (childCount >= 0)
            result->setAttribute(_("childCount"), String::from(childCount));

        String upnp_class = obj->getClass();
        log_debug("container is class: %s\n", upnp_class.c_str());
        if (upnp_class == UPNP_DEFAULT_CLASS_MUSIC_ALBUM) {
            Ref<Dictionary> meta = obj->getMetadata();

            String creator = meta->get(MetadataHandler::getMetaFieldName(M_ALBUMARTIST));
            if (!string_ok(creator)) {
                creator = meta->get(MetadataHandler::getMetaFieldName(M_ARTIST));
            }

            if (string_ok(creator)) {
                result->appendElementChild(renderCreator(creator));
            }

            String composer = meta->get(MetadataHandler::getMetaFieldName(M_COMPOSER));
            if (!string_ok(composer)) {
                composer = _("None");
            }

            if (string_ok(composer)) {
                result->appendElementChild(renderComposer(composer));
            }

            String conductor = meta->get(MetadataHandler::getMetaFieldName(M_CONDUCTOR));
            if (!string_ok(conductor)) {
                conductor = _("None");
            }

            if (string_ok(conductor)) {
                result->appendElementChild(renderConductor(conductor));
            }

            String orchestra = meta->get(MetadataHandler::getMetaFieldName(M_ORCHESTRA));
            if (!string_ok(orchestra)) {
                orchestra = _("None");
            }

            if (string_ok(orchestra)) {
                result->appendElementChild(renderOrchestra(orchestra));
            }

            String date = meta->get(MetadataHandler::getMetaFieldName(M_UPNP_DATE));
            if (!string_ok(date)) {
                date = _("None");
            }

            if (string_ok(date)) {
                result->appendElementChild(renderAlbumDate(date));
            }

        }
        if (upnp_class == UPNP_DEFAULT_CLASS_MUSIC_ALBUM || upnp_class == UPNP_DEFAULT_CLASS_CONTAINER) {
            Ref<Storage> storage = Storage::getInstance();
            String aa_id = storage->findFolderImage(cont->getID(), String());

            if (aa_id != nullptr) {
                log_debug("Using folder image as artwork for container\n");

                String url;
                Ref<Dictionary> dict(new Dictionary());
                dict->put(_(URL_OBJECT_ID), aa_id);

                url = virtualUrl + _(_URL_PARAM_SEPARATOR) + CONTENT_MEDIA_HANDLER + _(_URL_PARAM_SEPARATOR) + dict->encodeSimple() + _(_URL_PARAM_SEPARATOR) + _(URL_RESOURCE_ID) + _(_URL_PARAM_SEPARATOR) + "0";

                result->appendElementChild(renderAlbumArtURI(url));

            } else if (upnp_class == UPNP_DEFAULT_CLASS_MUSIC_ALBUM) {
                // try to find the first track and use its artwork
                auto items = storage->getObjects(cont->getID(), true);
                if (items != nullptr) {

                    bool artAdded = false;
                    for (const auto& id : *items) {
                        if (artAdded)
                            break;

                        Ref<CdsObject> obj = storage->loadObject(id);
                        if (obj->getClass() != UPNP_DEFAULT_CLASS_MUSIC_TRACK)
                            continue;

                        Ref<CdsItem> item = RefCast(obj, CdsItem);

                        auto resources = item->getResources();

                        for (int i = 1; i < resources->size(); i++) {
                            auto res = resources->get(i);
                            // only add upnp:AlbumArtURI if we have an AA, skip the resource
                            if ((res->getHandlerType() == CH_ID3) || (res->getHandlerType() == CH_MP4) || (res->getHandlerType() == CH_FLAC) || (res->getHandlerType() == CH_FANART) || (res->getHandlerType() == CH_EXTURL)) {

                                String url = CdsResourceManager::getArtworkUrl(item);
                                result->appendElementChild(renderAlbumArtURI(url));

                                artAdded = true;
                                break;
                            }
                        }
                    }
                }
            }
        }
    }

    if (renderActions && IS_CDS_ACTIVE_ITEM(objectType)) {
        Ref<CdsActiveItem> aitem = RefCast(obj, CdsActiveItem);
        result->appendTextChild(_("action"), aitem->getAction());
        result->appendTextChild(_("state"), aitem->getState());
        result->appendTextChild(_("location"), aitem->getLocation());
        result->appendTextChild(_("mime-type"), aitem->getMimeType());
    }

    // log_debug("Rendered DIDL: \n%s\n", result->print().c_str());

    return result;
}

void UpnpXMLBuilder::updateObject(Ref<CdsObject> obj, String text)
{
    Ref<Parser> parser(new Parser());
    Ref<Element> root = parser->parseString(text)->getRoot();
    int objectType = obj->getObjectType();

    if (IS_CDS_ACTIVE_ITEM(objectType)) {
        Ref<CdsActiveItem> aitem = RefCast(obj, CdsActiveItem);

        String title = root->getChildText(_("dc:title"));
        if (title != nullptr && title != "")
            aitem->setTitle(title);

        /// \todo description should be taken from the dictionary
        String description = root->getChildText(_("dc:description"));
        if (description == nullptr)
            description = _("");
        aitem->setMetadata(MetadataHandler::getMetaFieldName(M_DESCRIPTION),
            description);

        String location = root->getChildText(_("location"));
        if (location != nullptr && location != "")
            aitem->setLocation(location);

        String mimeType = root->getChildText(_("mime-type"));
        if (mimeType != nullptr && mimeType != "")
            aitem->setMimeType(mimeType);

        String action = root->getChildText(_("action"));
        if (action != nullptr && action != "")
            aitem->setAction(action);

        String state = root->getChildText(_("state"));
        if (state == nullptr)
            state = _("");
        aitem->setState(state);
    }
}

Ref<Element> UpnpXMLBuilder::createEventPropertySet()
{
    Ref<Element> propset(new Element(_("e:propertyset")));
    propset->setAttribute(_("xmlns:e"), _("urn:schemas-upnp-org:event-1-0"));

    Ref<Element> property(new Element(_("e:property")));

    propset->appendElementChild(property);
    return propset;
}

Ref<Element> UpnpXMLBuilder::renderDeviceDescription(String presentationURL)
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
    if (config->getBoolOption(CFG_SERVER_EXTEND_PROTOCOLINFO)) {
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

    // media receiver registrar service for the Xbox 360
    Ref<Element> serviceMRREG(new Element(_("service")));
    serviceMRREG->appendTextChild(_("serviceType"), _(DESC_MRREG_SERVICE_TYPE));
    serviceMRREG->appendTextChild(_("serviceId"), _(DESC_MRREG_SERVICE_ID));
    serviceMRREG->appendTextChild(_("SCPDURL"), _(DESC_MRREG_SCPD_URL));
    serviceMRREG->appendTextChild(_("controlURL"), _(DESC_MRREG_CONTROL_URL));
    serviceMRREG->appendTextChild(_("eventSubURL"), _(DESC_MRREG_EVENT_URL));

    serviceList->appendElementChild(serviceMRREG);

    device->appendElementChild(serviceList);

    root->appendElementChild(device);

    return root;
}

Ref<Element> UpnpXMLBuilder::renderResource(String URL, Ref<Dictionary> attributes)
{
    Ref<Element> res(new Element(_("res")));

    res->setText(URL);

    Ref<Array<DictionaryElement>> elements = attributes->getElements();
    int len = elements->size();

    String attribute;

    for (int i = 0; i < len; i++) {
        Ref<DictionaryElement> el = elements->get(i);
        attribute = el->getKey();
        res->setAttribute(attribute, el->getValue());
    }

    return res;
}

Ref<Element> UpnpXMLBuilder::renderCaptionInfo(String URL)
{
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

Ref<Element> UpnpXMLBuilder::renderCreator(String creator)
{
    Ref<Element> out(new Element(_("dc:creator")));

    out->setText(creator);

    return out;
}

Ref<Element> UpnpXMLBuilder::renderAlbumArtURI(String uri)
{
    Ref<Element> out(new Element(_("upnp:albumArtURI")));
    out->setText(uri);
    return out;
}

Ref<Element> UpnpXMLBuilder::renderComposer(String composer)
{
    Ref<Element> out(new Element(_("upnp:composer")));
    out->setText(composer);
    return out;
}

Ref<Element> UpnpXMLBuilder::renderConductor(String Conductor)
{
    Ref<Element> out(new Element(_("upnp:Conductor")));
    out->setText(Conductor);
    return out;
}

Ref<Element> UpnpXMLBuilder::renderOrchestra(String orchestra)
{
    Ref<Element> out(new Element(_("upnp:orchestra")));
    out->setText(orchestra);
    return out;
}

Ref<Element> UpnpXMLBuilder::renderAlbumDate(String date) {
    Ref<Element> out(new Element(_("upnp:date")));
    out->setText(date);
    return out;
}
