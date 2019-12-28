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
#include "common.h"
#include "config_manager.h"
#include "metadata/metadata_handler.h"
#include "server.h"

using namespace zmm;
using namespace mxml;

UpnpXMLBuilder::UpnpXMLBuilder(std::string virtualUrl, std::string presentationURL)
    : virtualURL(virtualUrl)
    , presentationURL(presentationURL)
{
}

Ref<Element> UpnpXMLBuilder::createResponse(std::string actionName, std::string serviceType)
{
    Ref<Element> response(new Element("u:" + actionName + "Response"));
    response->setAttribute("xmlns:u", serviceType);

    return response;
}

Ref<Element> UpnpXMLBuilder::renderObject(Ref<CdsObject> obj, bool renderActions, size_t stringLimit)
{
    Ref<Element> result(new Element(""));

    result->setAttribute("id", std::to_string(obj->getID()));
    result->setAttribute("parentID", std::to_string(obj->getParentID()));
    result->setAttribute("restricted", obj->isRestricted() ? "1" : "0");

    std::string tmp = obj->getTitle();

    if ((stringLimit != std::string::npos) && (tmp.length() > stringLimit)) {
        tmp = tmp.substr(0, getValidUTF8CutPosition(tmp, stringLimit - 3));
        tmp = tmp + "...";
    }

    result->appendTextChild("dc:title", tmp);

    result->appendTextChild("upnp:class", obj->getClass());

    int objectType = obj->getObjectType();
    if (IS_CDS_ITEM(objectType)) {
        Ref<CdsItem> item = RefCast(obj, CdsItem);

        Ref<Dictionary> meta = obj->getMetadata();
        Ref<Array<DictionaryElement>> elements = meta->getElements();
        int len = elements->size();

        std::string key;
        std::string upnp_class = obj->getClass();

        for (int i = 0; i < len; i++) {
            Ref<DictionaryElement> el = elements->get(i);
            key = el->getKey();
            if (key == MetadataHandler::getMetaFieldName(M_DESCRIPTION)) {
                tmp = el->getValue();
                if ((stringLimit > 0) && (tmp.length() > stringLimit)) {
                    tmp = tmp.substr(0, getValidUTF8CutPosition(tmp, stringLimit - 3));
                    tmp = tmp + "...";
                }
                result->appendTextChild(key, tmp);
            } else if (key == MetadataHandler::getMetaFieldName(M_TRACKNUMBER)) {
                if (upnp_class == UPNP_DEFAULT_CLASS_MUSIC_TRACK)
                    result->appendTextChild(key, el->getValue());
            } else if ((key != MetadataHandler::getMetaFieldName(M_TITLE)) || ((key == MetadataHandler::getMetaFieldName(M_TRACKNUMBER)) && (upnp_class == UPNP_DEFAULT_CLASS_MUSIC_TRACK)))
                result->appendTextChild(key, el->getValue());
        }

        addResources(item, result);

        if (upnp_class == UPNP_DEFAULT_CLASS_MUSIC_TRACK) {
            Ref<Storage> storage = Storage::getInstance();
            // extract extension-less, lowercase track name to search for corresponding
            // image as cover alternative
            std::string dctl = tolower_string(item->getTitle());
            std::string trackArtBase = std::string();
            size_t doti = dctl.rfind('.');
            if (doti != std::string::npos) {
                trackArtBase = dctl.substr(0, doti);
            }
            std::string aa_id = storage->findFolderImage(item->getParentID(), trackArtBase);
            if (!aa_id.empty()) {
                std::string url;
                Ref<Dictionary> dict(new Dictionary());
                dict->put(URL_OBJECT_ID, aa_id);

                url = virtualURL + _URL_PARAM_SEPARATOR + CONTENT_MEDIA_HANDLER + _URL_PARAM_SEPARATOR + dict->encodeSimple() + _URL_PARAM_SEPARATOR + URL_RESOURCE_ID + _URL_PARAM_SEPARATOR + "0";
                log_debug("UpnpXMLRenderer::DIDLRenderObject: url: %s\n", url.c_str());
                Ref<Element> aa(new Element(MetadataHandler::getMetaFieldName(M_ALBUMARTURI)));
                aa->setText(url);
                result->appendElementChild(aa);
            }
        }
        result->setName("item");
    } else if (IS_CDS_CONTAINER(objectType)) {
        Ref<CdsContainer> cont = RefCast(obj, CdsContainer);

        result->setName("container");
        int childCount = cont->getChildCount();
        if (childCount >= 0)
            result->setAttribute("childCount", std::to_string(childCount));

        std::string upnp_class = obj->getClass();
        log_debug("container is class: %s\n", upnp_class.c_str());
        if (upnp_class == UPNP_DEFAULT_CLASS_MUSIC_ALBUM) {
            Ref<Dictionary> meta = obj->getMetadata();

            std::string creator = meta->get(MetadataHandler::getMetaFieldName(M_ALBUMARTIST));
            if (!string_ok(creator)) {
                creator = meta->get(MetadataHandler::getMetaFieldName(M_ARTIST));
            }

            if (string_ok(creator)) {
                result->appendElementChild(renderCreator(creator));
            }

            std::string composer = meta->get(MetadataHandler::getMetaFieldName(M_COMPOSER));
            if (!string_ok(composer)) {
                composer = "None";
            }

            if (string_ok(composer)) {
                result->appendElementChild(renderComposer(composer));
            }

            std::string conductor = meta->get(MetadataHandler::getMetaFieldName(M_CONDUCTOR));
            if (!string_ok(conductor)) {
                conductor = "None";
            }

            if (string_ok(conductor)) {
                result->appendElementChild(renderConductor(conductor));
            }

            std::string orchestra = meta->get(MetadataHandler::getMetaFieldName(M_ORCHESTRA));
            if (!string_ok(orchestra)) {
                orchestra = "None";
            }

            if (string_ok(orchestra)) {
                result->appendElementChild(renderOrchestra(orchestra));
            }

            std::string date = meta->get(MetadataHandler::getMetaFieldName(M_UPNP_DATE));
            if (!string_ok(date)) {
                date = "None";
            }

            if (string_ok(date)) {
                result->appendElementChild(renderAlbumDate(date));
            }

        }
        if (upnp_class == UPNP_DEFAULT_CLASS_MUSIC_ALBUM || upnp_class == UPNP_DEFAULT_CLASS_CONTAINER) {
            Ref<Storage> storage = Storage::getInstance();
            std::string aa_id = storage->findFolderImage(cont->getID(), std::string());

            if (!aa_id.empty()) {
                log_debug("Using folder image as artwork for container\n");

                std::string url;
                Ref<Dictionary> dict(new Dictionary());
                dict->put(URL_OBJECT_ID, aa_id);

                url = virtualURL + _URL_PARAM_SEPARATOR + CONTENT_MEDIA_HANDLER + _URL_PARAM_SEPARATOR + dict->encodeSimple() + _URL_PARAM_SEPARATOR + URL_RESOURCE_ID + _URL_PARAM_SEPARATOR + "0";

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


                                std::string url = getArtworkUrl(item);
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
        result->appendTextChild("action", aitem->getAction());
        result->appendTextChild("state", aitem->getState());
        result->appendTextChild("location", aitem->getLocation());
        result->appendTextChild("mime-type", aitem->getMimeType());
    }

    // log_debug("Rendered DIDL: \n%s\n", result->print().c_str());

    return result;
}

void UpnpXMLBuilder::updateObject(Ref<CdsObject> obj, std::string text)
{
    Ref<Parser> parser(new Parser());
    Ref<Element> root = parser->parseString(text)->getRoot();
    int objectType = obj->getObjectType();

    if (IS_CDS_ACTIVE_ITEM(objectType)) {
        Ref<CdsActiveItem> aitem = RefCast(obj, CdsActiveItem);

        std::string title = root->getChildText("dc:title");
        if (!title.empty())
            aitem->setTitle(title);

        /// \todo description should be taken from the dictionary
        std::string description = root->getChildText("dc:description");
        aitem->setMetadata(MetadataHandler::getMetaFieldName(M_DESCRIPTION),
            description);

        std::string location = root->getChildText("location");
        if (!location.empty())
            aitem->setLocation(location);

        std::string mimeType = root->getChildText("mime-type");
        if (!mimeType.empty())
            aitem->setMimeType(mimeType);

        std::string action = root->getChildText("action");
        if (!action.empty())
            aitem->setAction(action);

        std::string state = root->getChildText("state");
        aitem->setState(state);
    }
}

Ref<Element> UpnpXMLBuilder::createEventPropertySet()
{
    Ref<Element> propset(new Element("e:propertyset"));
    propset->setAttribute("xmlns:e", "urn:schemas-upnp-org:event-1-0");

    Ref<Element> property(new Element("e:property"));

    propset->appendElementChild(property);
    return propset;
}

Ref<Element> UpnpXMLBuilder::renderDeviceDescription()
{
    Ref<ConfigManager> config = ConfigManager::getInstance();

    Ref<Element> root(new Element("root"));
    root->setAttribute("xmlns", DESC_DEVICE_NAMESPACE);

    Ref<Element> specVersion(new Element("specVersion"));
    specVersion->appendTextChild("major", DESC_SPEC_VERSION_MAJOR);
    specVersion->appendTextChild("minor", DESC_SPEC_VERSION_MINOR);

    root->appendElementChild(specVersion);

    Ref<Element> device(new Element("device"));

    if (config->getBoolOption(CFG_SERVER_EXTEND_PROTOCOLINFO)) {
        //we do not do DLNA yet but this is needed for bravia tv (v5500)
        Ref<Element> DLNADOC(new Element("dlna:X_DLNADOC"));
        DLNADOC->setText("DMS-1.50");
        //      DLNADOC->setText("M-DMS-1.50");
        DLNADOC->setAttribute("xmlns:dlna", "urn:schemas-dlna-org:device-1-0");
        device->appendElementChild(DLNADOC);
    }

    device->appendTextChild("deviceType", DESC_DEVICE_TYPE);
    if (!string_ok(presentationURL))
        device->appendTextChild("presentationURL", "/");
    else
        device->appendTextChild("presentationURL", presentationURL);

    device->appendTextChild("friendlyName", config->getOption(CFG_SERVER_NAME));
    device->appendTextChild("manufacturer", config->getOption(CFG_SERVER_MANUFACTURER));
    device->appendTextChild("manufacturerURL", config->getOption(CFG_SERVER_MANUFACTURER_URL));
    device->appendTextChild("modelDescription", config->getOption(CFG_SERVER_MODEL_DESCRIPTION));
    device->appendTextChild("modelName", config->getOption(CFG_SERVER_MODEL_NAME));
    device->appendTextChild("modelNumber", config->getOption(CFG_SERVER_MODEL_NUMBER));
    device->appendTextChild("modelURL", config->getOption(CFG_SERVER_MODEL_URL));
    device->appendTextChild("serialNumber", config->getOption(CFG_SERVER_SERIAL_NUMBER));
    device->appendTextChild("UDN", config->getOption(CFG_SERVER_UDN));

    Ref<Element> iconList(new Element("iconList"));

    Ref<Element> icon120_png(new Element("icon"));
    icon120_png->appendTextChild("mimetype", DESC_ICON_PNG_MIMETYPE);
    icon120_png->appendTextChild("width", "120");
    icon120_png->appendTextChild("height", "120");
    icon120_png->appendTextChild("depth", "24");
    icon120_png->appendTextChild("url", DESC_ICON120_PNG);
    iconList->appendElementChild(icon120_png);

    Ref<Element> icon120_bmp(new Element("icon"));
    icon120_bmp->appendTextChild("mimetype", DESC_ICON_BMP_MIMETYPE);
    icon120_bmp->appendTextChild("width", "120");
    icon120_bmp->appendTextChild("height", "120");
    icon120_bmp->appendTextChild("depth", "24");
    icon120_bmp->appendTextChild("url", DESC_ICON120_BMP);
    iconList->appendElementChild(icon120_bmp);

    Ref<Element> icon120_jpg(new Element("icon"));
    icon120_jpg->appendTextChild("mimetype", DESC_ICON_JPG_MIMETYPE);
    icon120_jpg->appendTextChild("width", "120");
    icon120_jpg->appendTextChild("height", "120");
    icon120_jpg->appendTextChild("depth", "24");
    icon120_jpg->appendTextChild("url", DESC_ICON120_JPG);
    iconList->appendElementChild(icon120_jpg);

    Ref<Element> icon48_png(new Element("icon"));
    icon48_png->appendTextChild("mimetype", DESC_ICON_PNG_MIMETYPE);
    icon48_png->appendTextChild("width", "48");
    icon48_png->appendTextChild("height", "48");
    icon48_png->appendTextChild("depth", "24");
    icon48_png->appendTextChild("url", DESC_ICON48_PNG);
    iconList->appendElementChild(icon48_png);

    Ref<Element> icon48_bmp(new Element("icon"));
    icon48_bmp->appendTextChild("mimetype", DESC_ICON_BMP_MIMETYPE);
    icon48_bmp->appendTextChild("width", "48");
    icon48_bmp->appendTextChild("height", "48");
    icon48_bmp->appendTextChild("depth", "24");
    icon48_bmp->appendTextChild("url", DESC_ICON48_BMP);
    iconList->appendElementChild(icon48_bmp);

    Ref<Element> icon48_jpg(new Element("icon"));
    icon48_jpg->appendTextChild("mimetype", DESC_ICON_JPG_MIMETYPE);
    icon48_jpg->appendTextChild("width", "48");
    icon48_jpg->appendTextChild("height", "48");
    icon48_jpg->appendTextChild("depth", "24");
    icon48_jpg->appendTextChild("url", DESC_ICON48_JPG);
    iconList->appendElementChild(icon48_jpg);

    Ref<Element> icon32_png(new Element("icon"));
    icon32_png->appendTextChild("mimetype", DESC_ICON_PNG_MIMETYPE);
    icon32_png->appendTextChild("width", "32");
    icon32_png->appendTextChild("height", "32");
    icon32_png->appendTextChild("depth", "8");
    icon32_png->appendTextChild("url", DESC_ICON32_PNG);
    iconList->appendElementChild(icon32_png);

    Ref<Element> icon32_bmp(new Element("icon"));
    icon32_bmp->appendTextChild("mimetype", DESC_ICON_BMP_MIMETYPE);
    icon32_bmp->appendTextChild("width", "32");
    icon32_bmp->appendTextChild("height", "32");
    icon32_bmp->appendTextChild("depth", "8");
    icon32_bmp->appendTextChild("url", DESC_ICON32_BMP);
    iconList->appendElementChild(icon32_bmp);

    Ref<Element> icon32_jpg(new Element("icon"));
    icon32_jpg->appendTextChild("mimetype", DESC_ICON_JPG_MIMETYPE);
    icon32_jpg->appendTextChild("width", "32");
    icon32_jpg->appendTextChild("height", "32");
    icon32_jpg->appendTextChild("depth", "8");
    icon32_jpg->appendTextChild("url", DESC_ICON32_JPG);
    iconList->appendElementChild(icon32_jpg);

    device->appendElementChild(iconList);

    Ref<Element> serviceList(new Element("serviceList"));

    Ref<Element> serviceCM(new Element("service"));
    serviceCM->appendTextChild("serviceType", DESC_CM_SERVICE_TYPE);
    serviceCM->appendTextChild("serviceId", DESC_CM_SERVICE_ID);
    serviceCM->appendTextChild("SCPDURL", DESC_CM_SCPD_URL);
    serviceCM->appendTextChild("controlURL", DESC_CM_CONTROL_URL);
    serviceCM->appendTextChild("eventSubURL", DESC_CM_EVENT_URL);

    serviceList->appendElementChild(serviceCM);

    Ref<Element> serviceCDS(new Element("service"));
    serviceCDS->appendTextChild("serviceType", DESC_CDS_SERVICE_TYPE);
    serviceCDS->appendTextChild("serviceId", DESC_CDS_SERVICE_ID);
    serviceCDS->appendTextChild("SCPDURL", DESC_CDS_SCPD_URL);
    serviceCDS->appendTextChild("controlURL", DESC_CDS_CONTROL_URL);
    serviceCDS->appendTextChild("eventSubURL", DESC_CDS_EVENT_URL);

    serviceList->appendElementChild(serviceCDS);

    // media receiver registrar service for the Xbox 360
    Ref<Element> serviceMRREG(new Element("service"));
    serviceMRREG->appendTextChild("serviceType", DESC_MRREG_SERVICE_TYPE);
    serviceMRREG->appendTextChild("serviceId", DESC_MRREG_SERVICE_ID);
    serviceMRREG->appendTextChild("SCPDURL", DESC_MRREG_SCPD_URL);
    serviceMRREG->appendTextChild("controlURL", DESC_MRREG_CONTROL_URL);
    serviceMRREG->appendTextChild("eventSubURL", DESC_MRREG_EVENT_URL);

    serviceList->appendElementChild(serviceMRREG);

    device->appendElementChild(serviceList);

    root->appendElementChild(device);

    return root;
}

Ref<Element> UpnpXMLBuilder::renderResource(std::string URL, Ref<Dictionary> attributes)
{
    Ref<Element> res(new Element("res"));

    res->setText(URL);

    Ref<Array<DictionaryElement>> elements = attributes->getElements();
    int len = elements->size();

    std::string attribute;

    for (int i = 0; i < len; i++) {
        Ref<DictionaryElement> el = elements->get(i);
        attribute = el->getKey();
        res->setAttribute(attribute, el->getValue());
    }

    return res;
}

Ref<Element> UpnpXMLBuilder::renderCaptionInfo(std::string URL)
{
    Ref<Element> cap(new Element("sec:CaptionInfoEx"));

    // Samsung DLNA clients don't follow this URL and
    // obtain subtitle location from video HTTP headers.
    // We don't need to know here what the subtitle type
    // is and even if there is a subtitle.
    // This tag seems to be only a hint for Samsung devices,
    // though it's necessary.

    size_t endp = URL.rfind('.');
    cap->setText(URL.substr(0, endp) + ".srt");
    cap->setAttribute("sec:type", "srt");

    return cap;
}

Ref<Element> UpnpXMLBuilder::renderCreator(std::string creator)
{
    Ref<Element> out(new Element("dc:creator"));

    out->setText(creator);

    return out;
}

Ref<Element> UpnpXMLBuilder::renderAlbumArtURI(std::string uri)
{
    Ref<Element> out(new Element("upnp:albumArtURI"));
    out->setText(uri);
    return out;
}

Ref<Element> UpnpXMLBuilder::renderComposer(std::string composer)
{
    Ref<Element> out(new Element("upnp:composer"));
    out->setText(composer);
    return out;
}

Ref<Element> UpnpXMLBuilder::renderConductor(std::string Conductor)
{
    Ref<Element> out(new Element("upnp:Conductor"));
    out->setText(Conductor);
    return out;
}

Ref<Element> UpnpXMLBuilder::renderOrchestra(std::string orchestra)
{
    Ref<Element> out(new Element("upnp:orchestra"));
    out->setText(orchestra);
    return out;
}

Ref<Element> UpnpXMLBuilder::renderAlbumDate(std::string date) {
    Ref<Element> out(new Element("upnp:date"));
    out->setText(date);
    return out;
}

Ref<UpnpXMLBuilder::PathBase> UpnpXMLBuilder::getPathBase(Ref<CdsItem> item, bool forceLocal)
{
    Ref<Element> res;

    Ref<PathBase> pathBase(new PathBase);
    /// \todo resource options must be read from configuration files

    Ref<Dictionary> dict(new Dictionary());
    dict->put(URL_OBJECT_ID, std::to_string(item->getID()));

    pathBase->addResID = false;
    /// \todo move this down into the "for" loop and create different urls
    /// for each resource once the io handlers are ready
    int objectType = item->getObjectType();
    if (IS_CDS_ITEM_INTERNAL_URL(objectType)) {
        pathBase->pathBase = std::string(_URL_PARAM_SEPARATOR) + CONTENT_SERVE_HANDLER + _URL_PARAM_SEPARATOR + item->getLocation();
        return pathBase;
    }

    if (IS_CDS_ITEM_EXTERNAL_URL(objectType)) {
        if (!item->getFlag(OBJECT_FLAG_PROXY_URL) && (!forceLocal)) {
            pathBase->pathBase = item->getLocation();
            return pathBase;
        }

        if ((item->getFlag(OBJECT_FLAG_ONLINE_SERVICE) && item->getFlag(OBJECT_FLAG_PROXY_URL)) || forceLocal) {
            pathBase->pathBase = std::string(_URL_PARAM_SEPARATOR) + CONTENT_ONLINE_HANDLER + _URL_PARAM_SEPARATOR + dict->encodeSimple() + _URL_PARAM_SEPARATOR + URL_RESOURCE_ID + _URL_PARAM_SEPARATOR;
            pathBase->addResID = true;
            return pathBase;
        }
    }

    pathBase->pathBase = std::string(_URL_PARAM_SEPARATOR) + CONTENT_MEDIA_HANDLER + _URL_PARAM_SEPARATOR + dict->encodeSimple() + _URL_PARAM_SEPARATOR + URL_RESOURCE_ID + _URL_PARAM_SEPARATOR;
    pathBase->addResID = true;
    return pathBase;
}

std::string UpnpXMLBuilder::getFirstResourcePath(Ref<CdsItem> item)
{
    std::string result;
    Ref<PathBase> urlBase = getPathBase(item);
    int objectType = item->getObjectType();

    if (IS_CDS_ITEM_EXTERNAL_URL(objectType) && !urlBase->addResID) { // a remote resource
      result = urlBase->pathBase;
    } else if (urlBase->addResID){                                    // a proxy, remote, resource
      result = SERVER_VIRTUAL_DIR + urlBase->pathBase + std::to_string(0);
    } else {                                                          // a local resource
      result = SERVER_VIRTUAL_DIR + urlBase->pathBase;
    }
    return result;
}

std::string UpnpXMLBuilder::getArtworkUrl(zmm::Ref<CdsItem> item) {
    // FIXME: This is temporary until we do artwork properly.
    log_debug("Building Art url for %d\n", item->getID());

    Ref<PathBase> urlBase = getPathBase(item);
    if (urlBase->addResID) {
        return virtualURL + urlBase->pathBase + std::to_string(1) + "/rct/aa";
    }
    return virtualURL + urlBase->pathBase;
}

std::string UpnpXMLBuilder::renderExtension(std::string contentType, std::string location)
{
    std::string ext = std::string(_URL_PARAM_SEPARATOR) + URL_FILE_EXTENSION + _URL_PARAM_SEPARATOR + "file";

    if (string_ok(contentType) && (contentType != CONTENT_TYPE_PLAYLIST)) {
        ext = ext + "." + contentType;
        return ext;
    }

    if (string_ok(location)) {
        size_t dot = location.rfind('.');
        if (dot != std::string::npos) {
            std::string extension = location.substr(dot);
            // make sure that the extension does not contain the separator character
            if (string_ok(extension) && (extension.find(URL_PARAM_SEPARATOR) == std::string::npos) && (extension.find(URL_PARAM_SEPARATOR) == std::string::npos)) {
                ext = ext + extension;
                return ext;
            }
        }
    }
    return nullptr;
}

void UpnpXMLBuilder::addResources(Ref<CdsItem> item, Ref<Element> element)
{
    Ref<PathBase> urlBase = getPathBase(item);
    Ref<ConfigManager> config = ConfigManager::getInstance();
    bool skipURL = ((IS_CDS_ITEM_INTERNAL_URL(item->getObjectType()) || IS_CDS_ITEM_EXTERNAL_URL(item->getObjectType())) && (!item->getFlag(OBJECT_FLAG_PROXY_URL)));

    bool isExtThumbnail = false; // this sucks
    Ref<Dictionary> mappings = config->getDictionaryOption(CFG_IMPORT_MAPPINGS_MIMETYPE_TO_CONTENTTYPE_LIST);

#if defined(HAVE_FFMPEG) && defined(HAVE_FFMPEGTHUMBNAILER)
    if (config->getBoolOption(CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_ENABLED) && (item->getMimeType().startsWith("video") || item->getFlag(OBJECT_FLAG_OGG_THEORA))) {
        std::string videoresolution = item->getResource(0)->getAttribute(MetadataHandler::getResAttrName(R_RESOLUTION));
        int x;
        int y;

        if (string_ok(videoresolution) && check_resolution(videoresolution, &x, &y)) {
            std::string thumb_mimetype = mappings->get(CONTENT_TYPE_JPG);
            if (!string_ok(thumb_mimetype))
                thumb_mimetype = "image/jpeg";

            Ref<CdsResource> ffres(new CdsResource(CH_FFTH));
            ffres->addParameter(RESOURCE_HANDLER, std::string::from(CH_FFTH));
            ffres->addAttribute(MetadataHandler::getResAttrName(R_PROTOCOLINFO),
                renderProtocolInfo(thumb_mimetype));
            ffres->addOption(RESOURCE_CONTENT_TYPE, THUMBNAIL);

            y = config->getIntOption(CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_THUMBSIZE) * y / x;
            x = config->getIntOption(CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_THUMBSIZE);
            std::string resolution = std::string::from(x) + "x" + std::string::from(y);
            ffres->addAttribute(MetadataHandler::getResAttrName(R_RESOLUTION),
                resolution);
            item->addResource(ffres);
            log_debug("Adding resource for video thumbnail\n");
        }
    }
#endif // FFMPEGTHUMBNAILER

    // this will be used to count only the "real" resources, omitting the
    // transcoded ones
    int realCount = 0;
    bool hide_original_resource = false;
    int original_resource = 0;

    Ref<PathBase> urlBase_tr;

    // once proxying is a feature that can be turned off or on in
    // config manager we should use that setting
    //
    // TODO: allow transcoding for URLs

    // now get the profile
    Ref<TranscodingProfileList> tlist = config->getTranscodingProfileListOption(CFG_TRANSCODING_PROFILE_LIST);
    Ref<ObjectDictionary<TranscodingProfile>> tp_mt = tlist->get(item->getMimeType());
    if (tp_mt != nullptr) {
        Ref<Array<ObjectDictionaryElement<TranscodingProfile>>> profiles = tp_mt->getElements();
        for (int p = 0; p < profiles->size(); p++) {
            Ref<TranscodingProfile> tp = profiles->get(p)->getValue();

            if (tp == nullptr)
                throw _Exception("Invalid profile encountered!");

            std::string ct = mappings->get(item->getMimeType());
            if (ct == CONTENT_TYPE_OGG) {
                if (((item->getFlag(OBJECT_FLAG_OGG_THEORA)) && (!tp->isTheora())) || (!item->getFlag(OBJECT_FLAG_OGG_THEORA) && (tp->isTheora()))) {
                    continue;
                }
            }
            // check user fourcc settings
            else if (ct == CONTENT_TYPE_AVI) {
                avi_fourcc_listmode_t fcc_mode = tp->getAVIFourCCListMode();

                std::vector<std::string> fcc_list = tp->getAVIFourCCList();
                // mode is either process or ignore, so we will have to take a
                // look at the settings
                if (fcc_mode != FCC_None) {
                    std::string current_fcc = item->getResource(0)->getOption(RESOURCE_OPTION_FOURCC);
                    // we can not do much if the item has no fourcc info,
                    // so we will transcode it anyway
                    if (!string_ok(current_fcc)) {
                        // the process mode specifies that we will transcode
                        // ONLY if the fourcc matches the list; since an invalid
                        // fourcc can not match anything we will skip the item
                        if (fcc_mode == FCC_Process)
                            continue;
                    }
                    // we have the current and hopefully valid fcc string
                    // let's have a look if it matches the list
                    else {
                        bool fcc_match = false;
                        for (size_t f = 0; f < fcc_list.size(); f++) {
                            if (current_fcc == fcc_list[f])
                                fcc_match = true;
                        }

                        if (!fcc_match && (fcc_mode == FCC_Process))
                            continue;

                        if (fcc_match && (fcc_mode == FCC_Ignore))
                            continue;
                    }
                }
            }

            Ref<CdsResource> t_res(new CdsResource(CH_TRANSCODE));
            t_res->addParameter(URL_PARAM_TRANSCODE_PROFILE_NAME, tp->getName());
            // after transcoding resource was added we can not rely on
            // index 0, so we will make sure the ogg option is there
            t_res->addOption(CONTENT_TYPE_OGG,
                item->getResource(0)->getOption(CONTENT_TYPE_OGG));
            t_res->addParameter(URL_PARAM_TRANSCODE, URL_VALUE_TRANSCODE);

            std::string targetMimeType = tp->getTargetMimeType();

            if (!tp->isThumbnail()) {
                // duration should be the same for transcoded media, so we can
                // take the value from the original resource
                std::string duration = item->getResource(0)->getAttribute(MetadataHandler::getResAttrName(R_DURATION));
                if (string_ok(duration))
                    t_res->addAttribute(MetadataHandler::getResAttrName(R_DURATION),
                        duration);

                int freq = tp->getSampleFreq();
                if (freq == SOURCE) {
                    std::string frequency = item->getResource(0)->getAttribute(MetadataHandler::getResAttrName(R_SAMPLEFREQUENCY));
                    if (string_ok(frequency)) {
                        t_res->addAttribute(MetadataHandler::getResAttrName(R_SAMPLEFREQUENCY), frequency);
                        targetMimeType = targetMimeType + ";rate=" + frequency;
                    }
                } else if (freq != OFF) {
                    t_res->addAttribute(MetadataHandler::getResAttrName(R_SAMPLEFREQUENCY), std::to_string(freq));
                    targetMimeType = targetMimeType + ";rate=" + std::to_string(freq);
                }

                int chan = tp->getNumChannels();
                if (chan == SOURCE) {
                    std::string nchannels = item->getResource(0)->getAttribute(MetadataHandler::getResAttrName(R_NRAUDIOCHANNELS));
                    if (string_ok(nchannels)) {
                        t_res->addAttribute(MetadataHandler::getResAttrName(R_NRAUDIOCHANNELS), nchannels);
                        targetMimeType = targetMimeType + ";channels=" + nchannels;
                    }
                } else if (chan != OFF) {
                    t_res->addAttribute(MetadataHandler::getResAttrName(R_NRAUDIOCHANNELS), std::to_string(chan));
                    targetMimeType = targetMimeType + ";channels=" + std::to_string(chan);
                }
            }

            t_res->addAttribute(MetadataHandler::getResAttrName(R_PROTOCOLINFO),
                renderProtocolInfo(targetMimeType));

            if (tp->isThumbnail())
                t_res->addOption(RESOURCE_CONTENT_TYPE, EXIF_THUMBNAIL);

            t_res->mergeAttributes(tp->getAttributes());

            if (tp->hideOriginalResource())
                hide_original_resource = true;

            if (tp->firstResource()) {
                item->insertResource(0, t_res);
                original_resource++;
            } else
                item->addResource(t_res);
        }

        if (skipURL)
            urlBase_tr = getPathBase(item, true);
    }

    int resCount = item->getResourceCount();
    for (int i = 0; i < resCount; i++) {

        /// \todo what if the resource has a different mimetype than the item??
        /*        std::string mimeType = item->getMimeType();
                  if (!string_ok(mimeType)) mimeType = DEFAULT_MIMETYPE; */

        Ref<CdsResource> res = item->getResource(i);
        Ref<Dictionary> res_attrs = res->getAttributes();
        Ref<Dictionary> res_params = res->getParameters();
        std::string protocolInfo = res_attrs->get(MetadataHandler::getResAttrName(R_PROTOCOLINFO));
        std::string mimeType = getMTFromProtocolInfo(protocolInfo);

        size_t pos = mimeType.find(";");
        if (pos != std::string::npos) {
            mimeType = mimeType.substr(0, pos);
        }

        assert(string_ok(mimeType));
        std::string contentType = mappings->get(mimeType);
        std::string url;

        /// \todo who will sync mimetype that is part of the protocol info and
        /// that is lying in the resources with the information that is in the
        /// resource tags?

        // ok, here is the tricky part:
        // we add transcoded resources dynamically, that means that when
        // the object is loaded from storage those resources are not there;
        // this again means, that we have to add the res_id parameter
        // accounting for those dynamic resources: i.e. the parameter should
        // still only count the "real" resources, because that's what the
        // file request handler will be getting.
        // for transcoded resources the res_id can be safely ignored,
        // because a transcoded resource is identified by the profile name
        // flag if we are dealing with the transcoded resource
        bool transcoded = (res_params->get(URL_PARAM_TRANSCODE) == URL_VALUE_TRANSCODE);
        if (!transcoded) {
            if (urlBase->addResID) {
                url = urlBase->pathBase + std::to_string(realCount);
            } else
                url = urlBase->pathBase;

            realCount++;
        } else {
            if (!skipURL)
                url = urlBase->pathBase + URL_VALUE_TRANSCODE_NO_RES_ID;
            else {
                assert(urlBase_tr != nullptr);
                url = urlBase_tr->pathBase + URL_VALUE_TRANSCODE_NO_RES_ID;
            }
        }
        if ((res_params != nullptr) && (res_params->size() > 0)) {
            url = url + _URL_PARAM_SEPARATOR;
            url = url + res_params->encodeSimple();
        }

        // ok this really sucks, I guess another rewrite of the resource manager
        // is necessary
        if ((i > 0) && (res->getHandlerType() == CH_EXTURL) && ((res->getOption(RESOURCE_CONTENT_TYPE) == THUMBNAIL) || (res->getOption(RESOURCE_CONTENT_TYPE) == ID3_ALBUM_ART))) {
            url = res->getOption(RESOURCE_OPTION_URL);
            if (!string_ok(url))
                throw _Exception("missing thumbnail URL!");

            isExtThumbnail = true;
        }

        /// FIXME: currently resource is misused for album art

        // only add upnp:AlbumArtURI if we have an AA, skip the resource
        if (i > 0) {
            int handlerType = res->getHandlerType();

            if (handlerType == CH_ID3 || (handlerType == CH_MP4) || handlerType == CH_FLAC || handlerType == CH_FANART || handlerType == CH_EXTURL) {

                std::string rct;
                if (res->getHandlerType() == CH_EXTURL)
                    rct = res->getOption(RESOURCE_CONTENT_TYPE);
                else
                    rct = res->getParameter(RESOURCE_CONTENT_TYPE);

                if (rct == ID3_ALBUM_ART) {
                    Ref<Element> aa(new Element(MetadataHandler::getMetaFieldName(M_ALBUMARTURI)));
                    aa->setText(virtualURL + url);
                    if (config->getBoolOption(CFG_SERVER_EXTEND_PROTOCOLINFO)) {
                        /// \todo clean this up, make sure to check the mimetype and
                        /// provide the profile correctly
                        aa->setAttribute("xmlns:dlna", "urn:schemas-dlna-org:metadata-1-0");
                        aa->setAttribute("dlna:profileID", "JPEG_TN");
                    }
                    element->appendElementChild(aa);
                    continue;
                }
            }
        }

        if (!isExtThumbnail) {
            // when transcoding is enabled the first (zero) resource can be the
            // transcoded stream, that means that we can only go with the
            // content type here and that we will not limit ourselves to the
            // first resource
            if (!skipURL) {
                if (transcoded)
                    url = url + renderExtension(contentType, nullptr);
                else
                    url = url + renderExtension(contentType, item->getLocation());
            }
        }
        if (config->getBoolOption(CFG_SERVER_EXTEND_PROTOCOLINFO)) {
            std::string extend;
            if (contentType == CONTENT_TYPE_JPG) {
                std::string resolution = res_attrs->get(MetadataHandler::getResAttrName(R_RESOLUTION));
                int x;
                int y;
                if (string_ok(resolution) && check_resolution(resolution, &x, &y)) {

                    if ((i > 0) && (((item->getResource(i)->getHandlerType() == CH_LIBEXIF) && (item->getResource(i)->getParameter(RESOURCE_CONTENT_TYPE) == EXIF_THUMBNAIL)) || (item->getResource(i)->getOption(RESOURCE_CONTENT_TYPE) == EXIF_THUMBNAIL) || (item->getResource(i)->getOption(RESOURCE_CONTENT_TYPE) == THUMBNAIL)) && (x <= 160) && (y <= 160))
                        extend = std::string(D_PROFILE) + "=" + D_JPEG_TN + ";";
                    else if ((x <= 640) && (y <= 420))
                        extend = std::string(D_PROFILE) + "=" + D_JPEG_SM + ";";
                    else if ((x <= 1024) && (y <= 768))
                        extend = std::string(D_PROFILE) + "=" + D_JPEG_MED + ";";
                    else if ((x <= 4096) && (y <= 4096))
                        extend = std::string(D_PROFILE) + "=" + D_JPEG_LRG + ";";
                }
            } else {
                /* handle audio/video content */
                extend = getDLNAprofileString(contentType);
                if (string_ok(extend))
                    extend = extend + ";";
            }

            // we do not support seeking at all, so 00
            // and the media is converted, so set CI to 1
            if (!isExtThumbnail && transcoded) {
                extend = extend + D_OP + "=" + D_OP_SEEK_DISABLED + ";" + D_CONVERSION_INDICATOR + "=" D_CONVERSION;

                if (startswith(mimeType, "audio") || startswith(mimeType, "video"))
                    extend = extend + ";" D_FLAGS "=" D_TR_FLAGS_AV;
            } else {
                if (config->getBoolOption(CFG_SERVER_EXTEND_PROTOCOLINFO_DLNA_SEEK))
                    extend = extend + D_OP + "=" + D_OP_SEEK_ENABLED + ";";
                else
                    extend = extend + D_OP + "=" + D_OP_SEEK_DISABLED + ";";
                extend = extend + D_CONVERSION_INDICATOR + "=" + D_NO_CONVERSION;
            }

            protocolInfo = protocolInfo.substr(0, protocolInfo.rfind(':') + 1) + extend;
            res_attrs->put(MetadataHandler::getResAttrName(R_PROTOCOLINFO),
                protocolInfo);

            if (config->getBoolOption(CFG_SERVER_EXTEND_PROTOCOLINFO_SM_HACK)) {
                if (startswith(mimeType, "video")) {
                    element->appendElementChild(renderCaptionInfo(url));
                }
            }

            log_debug("extended protocolInfo: %s\n", protocolInfo.c_str());
        }
        // URL is path until now
        int objectType = item->getObjectType();
        if(!IS_CDS_ITEM_EXTERNAL_URL(objectType)) {
            url = virtualURL + url;
        }

        if (!hide_original_resource || transcoded || (hide_original_resource && (original_resource != i)))
            element->appendElementChild(renderResource(url, res_attrs));
    }
}
