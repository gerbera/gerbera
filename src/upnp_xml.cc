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
#include "config/config_manager.h"
#include "metadata/metadata_handler.h"
#include "server.h"
#include "storage/storage.h"
#include <utility>

UpnpXMLBuilder::UpnpXMLBuilder(std::shared_ptr<ConfigManager> config,
    std::shared_ptr<Storage> storage,
    std::string virtualUrl, std::string presentationURL)
    : config(std::move(config))
    , storage(std::move(storage))
    , virtualURL(std::move(virtualUrl))
    , presentationURL(std::move(presentationURL))
{
}

std::unique_ptr<pugi::xml_document> UpnpXMLBuilder::createResponse(const std::string& actionName, const std::string& serviceType)
{
    auto response = std::make_unique<pugi::xml_document>();
    auto root = response->append_child(("u:" + actionName + "Response").c_str());
    root.append_attribute("xmlns:u") = serviceType.c_str();

    return response;
}

void UpnpXMLBuilder::renderObject(const std::shared_ptr<CdsObject>& obj, bool renderActions, size_t stringLimit, pugi::xml_node* parent)
{
    auto result = parent->append_child("");

    result.append_attribute("id") = obj->getID();
    result.append_attribute("parentID") = obj->getParentID();
    result.append_attribute("restricted") = obj->isRestricted() ? "1" : "0";

    std::string tmp = obj->getTitle();
    if ((stringLimit != std::string::npos) && (tmp.length() > stringLimit)) {
        tmp = tmp.substr(0, getValidUTF8CutPosition(tmp, stringLimit - 3));
        tmp = tmp + "...";
    }
    result.append_child("dc:title").append_child(pugi::node_pcdata).set_value(tmp.c_str());

    result.append_child("upnp:class").append_child(pugi::node_pcdata).set_value(obj->getClass().c_str());

    int objectType = obj->getObjectType();
    if (IS_CDS_ITEM(objectType)) {
        auto item = std::static_pointer_cast<CdsItem>(obj);

        auto meta = obj->getMetadata();

        std::string key;
        std::string upnp_class = obj->getClass();

        for (const auto& it : meta) {
            key = it.first;
            if (key == MetadataHandler::getMetaFieldName(M_DESCRIPTION)) {
                tmp = it.second;
                if ((stringLimit > 0) && (tmp.length() > stringLimit)) {
                    tmp = tmp.substr(0, getValidUTF8CutPosition(tmp, stringLimit - 3));
                    tmp.append("...");
                }
                result.append_child(key.c_str()).append_child(pugi::node_pcdata).set_value(tmp.c_str());
            } else if (key == MetadataHandler::getMetaFieldName(M_TRACKNUMBER)) {
                if (upnp_class == UPNP_DEFAULT_CLASS_MUSIC_TRACK)
                    result.append_child(key.c_str()).append_child(pugi::node_pcdata).set_value(it.second.c_str());
            } else if ((key != MetadataHandler::getMetaFieldName(M_TITLE)) || ((key == MetadataHandler::getMetaFieldName(M_TRACKNUMBER)) && (upnp_class == UPNP_DEFAULT_CLASS_MUSIC_TRACK)))
                result.append_child(key.c_str()).append_child(pugi::node_pcdata).set_value(it.second.c_str());
        }

        addResources(item, &result);

        if (upnp_class == UPNP_DEFAULT_CLASS_MUSIC_TRACK) {
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
                std::map<std::string, std::string> dict;
                dict[URL_OBJECT_ID] = aa_id;

                url = virtualURL + _URL_PARAM_SEPARATOR + CONTENT_MEDIA_HANDLER + _URL_PARAM_SEPARATOR + dict_encode_simple(dict) + _URL_PARAM_SEPARATOR + URL_RESOURCE_ID + _URL_PARAM_SEPARATOR + "0";
                log_debug("UpnpXMLRenderer::DIDLRenderObject: url: {}", url.c_str());
                result.append_child(MetadataHandler::getMetaFieldName(M_ALBUMARTURI).c_str()).append_child(pugi::node_pcdata).set_value(url.c_str());
            }
        }
        result.set_name("item");
    } else if (IS_CDS_CONTAINER(objectType)) {
        auto cont = std::static_pointer_cast<CdsContainer>(obj);

        result.set_name("container");
        int childCount = cont->getChildCount();
        if (childCount >= 0)
            result.append_attribute("childCount") = childCount;

        std::string upnp_class = obj->getClass();
        log_debug("container is class: {}", upnp_class.c_str());
        if (upnp_class == UPNP_DEFAULT_CLASS_MUSIC_ALBUM) {
            auto meta = obj->getMetadata();

            std::string creator = getValueOrDefault(meta, MetadataHandler::getMetaFieldName(M_ALBUMARTIST));
            if (!string_ok(creator)) {
                creator = getValueOrDefault(meta, MetadataHandler::getMetaFieldName(M_ARTIST));
            }

            if (string_ok(creator)) {
                renderCreator(creator, &result);
            }

            std::string composer = getValueOrDefault(meta, MetadataHandler::getMetaFieldName(M_COMPOSER));
            if (!string_ok(composer)) {
                composer = "None";
            }

            if (string_ok(composer)) {
                renderComposer(composer, &result);
            }

            std::string conductor = getValueOrDefault(meta, MetadataHandler::getMetaFieldName(M_CONDUCTOR));
            if (!string_ok(conductor)) {
                conductor = "None";
            }

            if (string_ok(conductor)) {
                renderConductor(conductor, &result);
            }

            std::string orchestra = getValueOrDefault(meta, MetadataHandler::getMetaFieldName(M_ORCHESTRA));
            if (!string_ok(orchestra)) {
                orchestra = "None";
            }

            if (string_ok(orchestra)) {
                renderOrchestra(orchestra, &result);
            }

            std::string date = getValueOrDefault(meta, MetadataHandler::getMetaFieldName(M_UPNP_DATE));
            if (!string_ok(date)) {
                date = "None";
            }

            if (string_ok(date)) {
                renderAlbumDate(date, &result);
            }
        }
        if (upnp_class == UPNP_DEFAULT_CLASS_MUSIC_ALBUM || upnp_class == UPNP_DEFAULT_CLASS_CONTAINER) {
            std::string aa_id = storage->findFolderImage(cont->getID(), std::string());

            if (!aa_id.empty()) {
                log_debug("Using folder image as artwork for container");

                std::string url;
                std::map<std::string, std::string> dict;
                dict[URL_OBJECT_ID] = aa_id;

                url = virtualURL + _URL_PARAM_SEPARATOR + CONTENT_MEDIA_HANDLER + _URL_PARAM_SEPARATOR + dict_encode_simple(dict) + _URL_PARAM_SEPARATOR + URL_RESOURCE_ID + _URL_PARAM_SEPARATOR + "0";
                renderAlbumArtURI(url, &result);

            } else if (upnp_class == UPNP_DEFAULT_CLASS_MUSIC_ALBUM) {
                // try to find the first track and use its artwork
                auto items = storage->getObjects(cont->getID(), true);
                if (items != nullptr) {

                    bool artAdded = false;
                    for (const auto& id : *items) {
                        if (artAdded)
                            break;

                        auto obj = storage->loadObject(id);
                        if (obj->getClass() != UPNP_DEFAULT_CLASS_MUSIC_TRACK)
                            continue;

                        auto item = std::static_pointer_cast<CdsItem>(obj);

                        auto resources = item->getResources();
                        for (size_t i = 1; i < resources.size(); i++) {
                            auto res = resources[i];
                            // only add upnp:AlbumArtURI if we have an AA, skip the resource
                            if ((res->getHandlerType() == CH_ID3) || (res->getHandlerType() == CH_MP4) || (res->getHandlerType() == CH_FLAC) || (res->getHandlerType() == CH_FANART) || (res->getHandlerType() == CH_EXTURL)) {

                                std::string url = getArtworkUrl(item);
                                renderAlbumArtURI(url, &result);

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
        auto aitem = std::static_pointer_cast<CdsActiveItem>(obj);
        result.append_child("action").append_child(pugi::node_pcdata).set_value(aitem->getAction().c_str());
        result.append_child("state").append_child(pugi::node_pcdata).set_value(aitem->getState().c_str());
        result.append_child("location").append_child(pugi::node_pcdata).set_value(aitem->getLocation().c_str());
        result.append_child("mime-type").append_child(pugi::node_pcdata).set_value(aitem->getMimeType().c_str());
    }

    // log_debug("Rendered DIDL: {}", result->print().c_str());
}

void UpnpXMLBuilder::updateObject(const std::shared_ptr<CdsObject>& obj, const std::string& text)
{
    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_string(text.c_str());
    if (result.status != pugi::xml_parse_status::status_ok) {
        log_error("Error parsing object XML: {}", result.description());
        return;
    }

    auto root = doc.document_element();
    int objectType = obj->getObjectType();

    if (IS_CDS_ACTIVE_ITEM(objectType)) {
        auto aitem = std::static_pointer_cast<CdsActiveItem>(obj);

        std::string title = root.child("dc:title").text().as_string();
        if (!title.empty())
            aitem->setTitle(title);

        /// \todo description should be taken from the dictionary
        std::string description = root.child("dc:description").text().as_string();
        aitem->setMetadata(MetadataHandler::getMetaFieldName(M_DESCRIPTION),
            description);

        std::string location = root.child("location").text().as_string();
        if (!location.empty())
            aitem->setLocation(location);

        std::string mimeType = root.child("mime-type").text().as_string();
        if (!mimeType.empty())
            aitem->setMimeType(mimeType);

        std::string action = root.child("action").text().as_string();
        if (!action.empty())
            aitem->setAction(action);

        std::string state = root.child("state").text().as_string();
        aitem->setState(state);
    }
}

std::unique_ptr<pugi::xml_document> UpnpXMLBuilder::createEventPropertySet()
{
    auto doc = std::make_unique<pugi::xml_document>();

    auto propset = doc->append_child("e:propertyset");
    propset.append_attribute("xmlns:e") = "urn:schemas-upnp-org:event-1-0";

    propset.append_child("e:property");

    return doc;
}

std::unique_ptr<pugi::xml_document> UpnpXMLBuilder::renderDeviceDescription()
{
    auto doc = std::make_unique<pugi::xml_document>();

    auto decl = doc->prepend_child(pugi::node_declaration);
    decl.append_attribute("version") = "1.0";
    decl.append_attribute("encoding") = "UTF-8";

    auto root = doc->append_child("root");
    root.append_attribute("xmlns") = DESC_DEVICE_NAMESPACE;

    auto specVersion = root.append_child("specVersion");
    specVersion.append_child("major").append_child(pugi::node_pcdata).set_value(DESC_SPEC_VERSION_MAJOR);
    specVersion.append_child("minor").append_child(pugi::node_pcdata).set_value(DESC_SPEC_VERSION_MINOR);

    auto device = root.append_child("device");

    if (config->getBoolOption(CFG_SERVER_EXTEND_PROTOCOLINFO)) {
        // we do not do DLNA yet but this is needed for bravia tv (v5500)
        auto dlnaDoc = device.append_child("dlna:X_DLNADOC");
        dlnaDoc.append_attribute("xmlns:dlna") = "urn:schemas-dlna-org:device-1-0";
        dlnaDoc.append_child(pugi::node_pcdata).set_value("DMS-1.50");
        // dlnaDoc.append_child(pugi::node_pcdata).set_value("M-DMS-1.50");
    }

    device.append_child("deviceType").append_child(pugi::node_pcdata).set_value(DESC_DEVICE_TYPE);
    if (!string_ok(presentationURL))
        device.append_child("presentationURL").append_child(pugi::node_pcdata).set_value("/");
    else
        device.append_child("presentationURL").append_child(pugi::node_pcdata).set_value(presentationURL.c_str());

    device.append_child("friendlyName").append_child(pugi::node_pcdata).set_value(config->getOption(CFG_SERVER_NAME).c_str());
    device.append_child("manufacturer").append_child(pugi::node_pcdata).set_value(config->getOption(CFG_SERVER_MANUFACTURER).c_str());
    device.append_child("manufacturerURL").append_child(pugi::node_pcdata).set_value(config->getOption(CFG_SERVER_MANUFACTURER_URL).c_str());
    device.append_child("modelDescription").append_child(pugi::node_pcdata).set_value(config->getOption(CFG_SERVER_MODEL_DESCRIPTION).c_str());
    device.append_child("modelName").append_child(pugi::node_pcdata).set_value(config->getOption(CFG_SERVER_MODEL_NAME).c_str());
    device.append_child("modelNumber").append_child(pugi::node_pcdata).set_value(config->getOption(CFG_SERVER_MODEL_NUMBER).c_str());
    device.append_child("modelURL").append_child(pugi::node_pcdata).set_value(config->getOption(CFG_SERVER_MODEL_URL).c_str());
    device.append_child("serialNumber").append_child(pugi::node_pcdata).set_value(config->getOption(CFG_SERVER_SERIAL_NUMBER).c_str());
    device.append_child("UDN").append_child(pugi::node_pcdata).set_value(config->getOption(CFG_SERVER_UDN).c_str());

    // add icons
    {
        auto iconList = device.append_child("iconList");

        struct IconDim {
            const char* dim;
            const char* depth;
        };
        std::vector<IconDim> iconDims;
        iconDims.push_back({ "120", "24" });
        iconDims.push_back({ "48", "24" });
        iconDims.push_back({ "32", "8" });
        struct IconType {
            const char* mimetype;
            const char* ext;
        };
        std::vector<IconType> iconTypes;
        iconTypes.push_back({ DESC_ICON_PNG_MIMETYPE, ".png" });
        iconTypes.push_back({ DESC_ICON_BMP_MIMETYPE, ".bmp" });
        iconTypes.push_back({ DESC_ICON_JPG_MIMETYPE, ".jpg" });

        for (auto const& d : iconDims) {
            for (auto const& t : iconTypes) {
                auto icon = iconList.append_child("icon");
                icon.append_child("mimetype").append_child(pugi::node_pcdata).set_value(t.mimetype);
                icon.append_child("width").append_child(pugi::node_pcdata).set_value(d.dim);
                icon.append_child("height").append_child(pugi::node_pcdata).set_value(d.dim);
                icon.append_child("depth").append_child(pugi::node_pcdata).set_value(d.depth);
                std::string url = fmt::format("/icons/mt-icon{}{}", d.dim, t.ext);
                icon.append_child("url").append_child(pugi::node_pcdata).set_value(url.c_str());
            }
        }
    }

    // add services
    {
        auto serviceList = device.append_child("serviceList");

        struct ServiceInfo {
            const char* serviceType;
            const char* serviceId;
            const char* SCPDURL;
            const char* controlURL;
            const char* eventSubURL;
        };
        std::vector<ServiceInfo> services;

        // cm
        services.push_back({ DESC_CM_SERVICE_TYPE, DESC_CM_SERVICE_ID,
            DESC_CM_SCPD_URL, DESC_CM_CONTROL_URL, DESC_CM_EVENT_URL });
        // cds
        services.push_back({ DESC_CDS_SERVICE_TYPE, DESC_CDS_SERVICE_ID,
            DESC_CDS_SCPD_URL, DESC_CDS_CONTROL_URL, DESC_CDS_EVENT_URL });
        // media receiver registrar service for the Xbox 360
        services.push_back({ DESC_MRREG_SERVICE_TYPE, DESC_MRREG_SERVICE_ID,
            DESC_MRREG_SCPD_URL, DESC_MRREG_CONTROL_URL, DESC_MRREG_EVENT_URL });

        for (auto const& s : services) {
            auto service = serviceList.append_child("service");
            service.append_child("serviceType").append_child(pugi::node_pcdata).set_value(s.serviceType);
            service.append_child("serviceId").append_child(pugi::node_pcdata).set_value(s.serviceId);
            service.append_child("SCPDURL").append_child(pugi::node_pcdata).set_value(s.SCPDURL);
            service.append_child("controlURL").append_child(pugi::node_pcdata).set_value(s.controlURL);
            service.append_child("eventSubURL").append_child(pugi::node_pcdata).set_value(s.eventSubURL);
        }
    }

    return doc;
}

void UpnpXMLBuilder::renderResource(const std::string& URL, const std::map<std::string, std::string>& attributes, pugi::xml_node* parent)
{
    auto res = parent->append_child("res");
    res.append_child(pugi::node_pcdata).set_value(URL.c_str());

    for (const auto& attribute : attributes) {
        res.append_attribute(attribute.first.c_str()) = attribute.second.c_str();
    }
}

void UpnpXMLBuilder::renderCaptionInfo(const std::string& URL, pugi::xml_node* parent)
{
    auto cap = parent->append_child("sec:CaptionInfoEx");

    // Samsung DLNA clients don't follow this URL and
    // obtain subtitle location from video HTTP headers.
    // We don't need to know here what the subtitle type
    // is and even if there is a subtitle.
    // This tag seems to be only a hint for Samsung devices,
    // though it's necessary.

    size_t endp = URL.rfind('.');
    cap.append_child(pugi::node_pcdata).set_value((URL.substr(0, endp) + ".srt").c_str());
    cap.append_attribute("sec:type") = "srt";
}

void UpnpXMLBuilder::renderCreator(const std::string& creator, pugi::xml_node* parent)
{
    parent->append_child("dc:creator").append_child(pugi::node_pcdata).set_value(creator.c_str());
}

void UpnpXMLBuilder::renderAlbumArtURI(const std::string& uri, pugi::xml_node* parent)
{
    parent->append_child("upnp:albumArtURI").append_child(pugi::node_pcdata).set_value(uri.c_str());
}

void UpnpXMLBuilder::renderComposer(const std::string& composer, pugi::xml_node* parent)
{
    parent->append_child("upnp:composer").append_child(pugi::node_pcdata).set_value(composer.c_str());
}

void UpnpXMLBuilder::renderConductor(const std::string& conductor, pugi::xml_node* parent)
{
    parent->append_child("upnp:Conductor").append_child(pugi::node_pcdata).set_value(conductor.c_str());
}

void UpnpXMLBuilder::renderOrchestra(const std::string& orchestra, pugi::xml_node* parent)
{
    parent->append_child("upnp:orchestra").append_child(pugi::node_pcdata).set_value(orchestra.c_str());
}

void UpnpXMLBuilder::renderAlbumDate(const std::string& date, pugi::xml_node* parent)
{
    parent->append_child("upnp:date").append_child(pugi::node_pcdata).set_value(date.c_str());
}

std::unique_ptr<UpnpXMLBuilder::PathBase> UpnpXMLBuilder::getPathBase(const std::shared_ptr<CdsItem>& item, bool forceLocal)
{
    auto pathBase = std::make_unique<PathBase>();
    /// \todo resource options must be read from configuration files

    std::map<std::string, std::string> dict;
    dict[URL_OBJECT_ID] = std::to_string(item->getID());

    pathBase->addResID = false;
    /// \todo move this down into the "for" loop and create different urls
    /// for each resource once the io handlers are ready
    int objectType = item->getObjectType();
    if (IS_CDS_ITEM_INTERNAL_URL(objectType)) {
        pathBase->pathBase = std::string(_URL_PARAM_SEPARATOR) + CONTENT_SERVE_HANDLER + _URL_PARAM_SEPARATOR + item->getLocation().string();
        return pathBase;
    }

    if (IS_CDS_ITEM_EXTERNAL_URL(objectType)) {
        if (!item->getFlag(OBJECT_FLAG_PROXY_URL) && (!forceLocal)) {
            pathBase->pathBase = item->getLocation();
            return pathBase;
        }

        if ((item->getFlag(OBJECT_FLAG_ONLINE_SERVICE) && item->getFlag(OBJECT_FLAG_PROXY_URL)) || forceLocal) {
            pathBase->pathBase = std::string(_URL_PARAM_SEPARATOR) + CONTENT_ONLINE_HANDLER + _URL_PARAM_SEPARATOR + dict_encode_simple(dict) + _URL_PARAM_SEPARATOR + URL_RESOURCE_ID + _URL_PARAM_SEPARATOR;
            pathBase->addResID = true;
            return pathBase;
        }
    }

    pathBase->pathBase = std::string(_URL_PARAM_SEPARATOR) + CONTENT_MEDIA_HANDLER + _URL_PARAM_SEPARATOR + dict_encode_simple(dict) + _URL_PARAM_SEPARATOR + URL_RESOURCE_ID + _URL_PARAM_SEPARATOR;
    pathBase->addResID = true;
    return pathBase;
}

std::string UpnpXMLBuilder::getFirstResourcePath(const std::shared_ptr<CdsItem>& item)
{
    std::string result;
    auto urlBase = getPathBase(item);
    int objectType = item->getObjectType();

    if (IS_CDS_ITEM_EXTERNAL_URL(objectType) && !urlBase->addResID) { // a remote resource
        result = urlBase->pathBase;
    } else if (urlBase->addResID) { // a proxy, remote, resource
        result = SERVER_VIRTUAL_DIR + urlBase->pathBase + std::to_string(0);
    } else { // a local resource
        result = SERVER_VIRTUAL_DIR + urlBase->pathBase;
    }
    return result;
}

std::string UpnpXMLBuilder::getArtworkUrl(const std::shared_ptr<CdsItem>& item)
{
    // FIXME: This is temporary until we do artwork properly.
    log_debug("Building Art url for {}", item->getID());

    auto urlBase = getPathBase(item);
    if (urlBase->addResID) {
        return virtualURL + urlBase->pathBase + std::to_string(1) + "/rct/aa";
    }
    return virtualURL + urlBase->pathBase;
}

std::string UpnpXMLBuilder::renderExtension(const std::string& contentType, const std::string& location)
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

    return "";
}

void UpnpXMLBuilder::addResources(const std::shared_ptr<CdsItem>& item, pugi::xml_node* parent)
{
    auto urlBase = getPathBase(item);
    bool skipURL = ((IS_CDS_ITEM_INTERNAL_URL(item->getObjectType()) || IS_CDS_ITEM_EXTERNAL_URL(item->getObjectType())) && (!item->getFlag(OBJECT_FLAG_PROXY_URL)));

    bool isExtThumbnail = false; // this sucks
    auto mappings = config->getDictionaryOption(CFG_IMPORT_MAPPINGS_MIMETYPE_TO_CONTENTTYPE_LIST);

#if defined(HAVE_FFMPEG) && defined(HAVE_FFMPEGTHUMBNAILER)
    if (config->getBoolOption(CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_ENABLED) && (startswith(item->getMimeType(), "video") || item->getFlag(OBJECT_FLAG_OGG_THEORA))) {
        std::string videoresolution = item->getResource(0)->getAttribute(MetadataHandler::getResAttrName(R_RESOLUTION));
        int x;
        int y;

        if (string_ok(videoresolution) && check_resolution(videoresolution, &x, &y)) {
            std::string thumb_mimetype = getValueOrDefault(mappings, CONTENT_TYPE_JPG);
            if (!string_ok(thumb_mimetype))
                thumb_mimetype = "image/jpeg";

            auto ffres = std::make_shared<CdsResource>(CH_FFTH);
            ffres->addParameter(RESOURCE_HANDLER, std::to_string(CH_FFTH));
            ffres->addAttribute(MetadataHandler::getResAttrName(R_PROTOCOLINFO),
                renderProtocolInfo(thumb_mimetype));
            ffres->addOption(RESOURCE_CONTENT_TYPE, THUMBNAIL);

            y = config->getIntOption(CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_THUMBSIZE) * y / x;
            x = config->getIntOption(CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_THUMBSIZE);
            std::string resolution = std::to_string(x) + "x" + std::to_string(y);
            ffres->addAttribute(MetadataHandler::getResAttrName(R_RESOLUTION),
                resolution);
            item->addResource(ffres);
            log_debug("Adding resource for video thumbnail");
        }
    }
#endif // FFMPEGTHUMBNAILER

    // this will be used to count only the "real" resources, omitting the
    // transcoded ones
    int realCount = 0;
    bool hide_original_resource = false;
    int original_resource = 0;

    std::unique_ptr<PathBase> urlBase_tr;

    // once proxying is a feature that can be turned off or on in
    // config manager we should use that setting
    //
    // TODO: allow transcoding for URLs

    // now get the profile
    auto tlist = config->getTranscodingProfileListOption(CFG_TRANSCODING_PROFILE_LIST);
    auto tp_mt = tlist->get(item->getMimeType());
    if (tp_mt != nullptr) {
        for (const auto& it : *tp_mt) {
            auto tp = it.second;

            if (tp == nullptr)
                throw std::runtime_error("Invalid profile encountered!");

            std::string ct = getValueOrDefault(mappings, item->getMimeType());
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
                        for (const auto& f : fcc_list) {
                            if (current_fcc == f)
                                fcc_match = true;
                        }

                        if (!fcc_match && (fcc_mode == FCC_Process))
                            continue;

                        if (fcc_match && (fcc_mode == FCC_Ignore))
                            continue;
                    }
                }
            }

            auto t_res = std::make_shared<CdsResource>(CH_TRANSCODE);
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
                        targetMimeType.append(";rate=").append(frequency);
                    }
                } else if (freq != OFF) {
                    t_res->addAttribute(MetadataHandler::getResAttrName(R_SAMPLEFREQUENCY), std::to_string(freq));
                    targetMimeType.append(";rate=").append(std::to_string(freq));
                }

                int chan = tp->getNumChannels();
                if (chan == SOURCE) {
                    std::string nchannels = item->getResource(0)->getAttribute(MetadataHandler::getResAttrName(R_NRAUDIOCHANNELS));
                    if (string_ok(nchannels)) {
                        t_res->addAttribute(MetadataHandler::getResAttrName(R_NRAUDIOCHANNELS), nchannels);
                        targetMimeType.append(";channels=").append(nchannels);
                    }
                } else if (chan != OFF) {
                    t_res->addAttribute(MetadataHandler::getResAttrName(R_NRAUDIOCHANNELS), std::to_string(chan));
                    targetMimeType.append(";channels=").append(std::to_string(chan));
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

        auto res = item->getResource(i);
        auto res_attrs = res->getAttributes();
        auto res_params = res->getParameters();
        std::string protocolInfo = getValueOrDefault(res_attrs, MetadataHandler::getResAttrName(R_PROTOCOLINFO));
        std::string mimeType = getMTFromProtocolInfo(protocolInfo);

        size_t pos = mimeType.find(';');
        if (pos != std::string::npos) {
            mimeType = mimeType.substr(0, pos);
        }

        assert(string_ok(mimeType));
        std::string contentType = getValueOrDefault(mappings, mimeType);
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
        bool transcoded = (getValueOrDefault(res_params, URL_PARAM_TRANSCODE) == URL_VALUE_TRANSCODE);
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
        if (!res_params.empty()) {
            url.append(_URL_PARAM_SEPARATOR);
            url.append(dict_encode_simple(res_params));
        }

        // ok this really sucks, I guess another rewrite of the resource manager
        // is necessary
        if ((i > 0) && (res->getHandlerType() == CH_EXTURL) && ((res->getOption(RESOURCE_CONTENT_TYPE) == THUMBNAIL) || (res->getOption(RESOURCE_CONTENT_TYPE) == ID3_ALBUM_ART))) {
            url = res->getOption(RESOURCE_OPTION_URL);
            if (!string_ok(url))
                throw std::runtime_error("missing thumbnail URL!");

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
                    auto aa = parent->append_child(MetadataHandler::getMetaFieldName(M_ALBUMARTURI).c_str());
                    aa.append_child(pugi::node_pcdata).set_value((virtualURL + url).c_str());
                    if (config->getBoolOption(CFG_SERVER_EXTEND_PROTOCOLINFO)) {
                        /// \todo clean this up, make sure to check the mimetype and
                        /// provide the profile correctly
                        aa.append_attribute("xmlns:dlna") = "urn:schemas-dlna-org:metadata-1-0";
                        aa.append_attribute("dlna:profileID") = "JPEG_TN";
                    }
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
                    url.append(renderExtension(contentType, ""));
                else
                    url.append(renderExtension(contentType, item->getLocation()));
            }
        }
        if (config->getBoolOption(CFG_SERVER_EXTEND_PROTOCOLINFO)) {
            std::string extend;
            if (contentType == CONTENT_TYPE_JPG) {
                std::string resolution = getValueOrDefault(res_attrs, MetadataHandler::getResAttrName(R_RESOLUTION));
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
                    extend.append(";");
            }

            // we do not support seeking at all, so 00
            // and the media is converted, so set CI to 1
            if (!isExtThumbnail && transcoded) {
                extend.append(D_OP).append("=").append(D_OP_SEEK_DISABLED).append(";").append(D_CONVERSION_INDICATOR).append("=" D_CONVERSION);

                if (startswith(mimeType, "audio") || startswith(mimeType, "video"))
                    extend.append(";" D_FLAGS "=" D_TR_FLAGS_AV);
            } else {
                if (config->getBoolOption(CFG_SERVER_EXTEND_PROTOCOLINFO_DLNA_SEEK))
                    extend.append(D_OP).append("=").append(D_OP_SEEK_ENABLED).append(";");
                else
                    extend.append(D_OP).append("=").append(D_OP_SEEK_DISABLED).append(";");
                extend.append(D_CONVERSION_INDICATOR).append("=").append(D_NO_CONVERSION);
            }

            protocolInfo = protocolInfo.substr(0, protocolInfo.rfind(':') + 1).append(extend);
            res_attrs[MetadataHandler::getResAttrName(R_PROTOCOLINFO)] = protocolInfo;

            if (config->getBoolOption(CFG_SERVER_EXTEND_PROTOCOLINFO_SM_HACK)) {
                if (startswith(mimeType, "video")) {
                    renderCaptionInfo(url, parent);
                }
            }

            log_debug("extended protocolInfo: {}", protocolInfo.c_str());
        }
        // URL is path until now
        int objectType = item->getObjectType();
        if (!IS_CDS_ITEM_EXTERNAL_URL(objectType)) {
            url.insert(0, virtualURL);
        }

        if (!hide_original_resource || transcoded || (hide_original_resource && (original_resource != i)))
            renderResource(url, res_attrs, parent);
    }
}
