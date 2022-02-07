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

#include "upnp_xml.h" // API

#include <sstream>

#include "config/config_manager.h"
#include "database/database.h"
#include "metadata/metadata_handler.h"
#include "request_handler.h"
#include "transcoding/transcoding.h"

UpnpXMLBuilder::UpnpXMLBuilder(const std::shared_ptr<Context>& context,
    std::string virtualUrl, std::string presentationURL)
    : config(context->getConfig())
    , database(context->getDatabase())
    , virtualURL(std::move(virtualUrl))
    , presentationURL(std::move(presentationURL))
{
    for (auto&& entry : this->config->getArrayOption(CFG_IMPORT_RESOURCES_ORDER)) {
        auto ch = MetadataHandler::remapContentHandler(entry);
        if (ch > -1) {
            orderedHandler.push_back(ch);
        }
    }
    entrySeparator = config->getOption(CFG_IMPORT_LIBOPTS_ENTRY_SEP);
    multiValue = config->getBoolOption(CFG_UPNP_MULTI_VALUES_ENABLED);
    ctMappings = config->getDictionaryOption(CFG_IMPORT_MAPPINGS_MIMETYPE_TO_CONTENTTYPE_LIST);
}

std::unique_ptr<pugi::xml_document> UpnpXMLBuilder::createResponse(const std::string& actionName, const std::string& serviceType)
{
    auto response = std::make_unique<pugi::xml_document>();
    auto root = response->append_child(fmt::format("u:{}Response", actionName).c_str());
    root.append_attribute("xmlns:u") = serviceType.c_str();

    return response;
}

void UpnpXMLBuilder::addPropertyList(pugi::xml_node& result, const std::vector<std::pair<std::string, std::string>>& meta, const std::map<std::string, std::string>& auxData,
    config_option_t itemProps, config_option_t nsProp)
{
    auto namespaceMap = config->getDictionaryOption(nsProp);
    for (auto&& [xmlns, uri] : namespaceMap) {
        result.append_attribute(fmt::format("xmlns:{}", xmlns).c_str()) = uri.c_str();
    }
    auto propertyMap = config->getDictionaryOption(itemProps);
    for (auto&& [tag, field] : propertyMap) {
        auto metaField = MetadataHandler::remapMetaDataField(field);
        bool wasMeta = false;
        for (auto&& [mkey, mvalue] : meta) {
            if ((metaField != M_MAX && mkey == MetadataHandler::getMetaFieldName(metaField)) || mkey == field) {
                addField(result, tag, mvalue);
                wasMeta = true;
            }
        }
        if (!wasMeta) {
            auto avalue = getValueOrDefault(auxData, field);
            if (!avalue.empty()) {
                addField(result, tag, avalue);
            }
        }
    }
}

std::string UpnpXMLBuilder::printXml(const pugi::xml_node& entry, const char* indent, int flags)
{
    std::ostringstream buf;
    entry.print(buf, indent, flags);
    return buf.str();
}

void UpnpXMLBuilder::addField(pugi::xml_node& entry, const std::string& key, const std::string& val)
{
    // e.g. used for M_ALBUMARTIST
    // name@attr[val] => <name attr="val">
    auto i = key.find('@');
    auto j = key.find('[', i + 1);
    if (i != std::string::npos
        && j != std::string::npos
        && key[key.length() - 1] == ']') {
        std::string attrName = key.substr(i + 1, j - i - 1);
        std::string attrValue = key.substr(j + 1, key.length() - j - 2);
        std::string name = key.substr(0, i);
        auto node = entry.append_child(name.c_str());
        node.append_attribute(attrName.c_str()) = attrValue.c_str();
        node.append_child(pugi::node_pcdata).set_value(val.c_str());
    } else {
        entry.append_child(key.c_str()).append_child(pugi::node_pcdata).set_value(val.c_str());
    }
}

void UpnpXMLBuilder::renderObject(const std::shared_ptr<CdsObject>& obj, std::size_t stringLimit, pugi::xml_node& parent, const std::unique_ptr<Quirks>& quirks)
{
    auto result = parent.append_child("");

    result.append_attribute("id") = obj->getID();
    result.append_attribute("parentID") = obj->getParentID();
    result.append_attribute("restricted") = obj->isRestricted() ? "1" : "0";

    auto limitString = [stringLimit](const std::string& s) {
        // Do nothing if disabled, or string is already short enough
        if (stringLimit == std::string::npos || s.length() <= stringLimit)
            return s;

        ssize_t cutPosition = getValidUTF8CutPosition(s, stringLimit - std::strlen("..."));
        return s.substr(0, cutPosition) + "...";
    };

    const std::string title = obj->getTitle();
    const std::string upnpClass = obj->getClass();

    result.append_child("dc:title").append_child(pugi::node_pcdata).set_value(limitString(title).c_str());
    result.append_child("upnp:class").append_child(pugi::node_pcdata).set_value(upnpClass.c_str());

    auto auxData = obj->getAuxData();

    if (obj->isItem()) {
        auto item = std::static_pointer_cast<CdsItem>(obj);

        if (quirks)
            quirks->restoreSamsungBookMarkedPosition(item, result);

        auto metaGroups = obj->getMetaGroups();

        for (auto&& [key, group] : metaGroups) {
            if (multiValue) {
                for (auto&& val : group) {
                    // Trim metadata value as needed
                    auto str = limitString(val);
                    if (key == MetadataHandler::getMetaFieldName(M_DESCRIPTION))
                        result.append_child(key.c_str()).append_child(pugi::node_pcdata).set_value(str.c_str());
                    else if (upnpClass == UPNP_CLASS_MUSIC_TRACK && key == MetadataHandler::getMetaFieldName(M_TRACKNUMBER))
                        result.append_child(key.c_str()).append_child(pugi::node_pcdata).set_value(str.c_str());
                    else if (key != MetadataHandler::getMetaFieldName(M_TITLE))
                        addField(result, key, limitString(str));
                }
            } else {
                // Trim metadata value as needed
                auto str = limitString(fmt::format("{}", fmt::join(group, entrySeparator)));
                if (key == MetadataHandler::getMetaFieldName(M_DESCRIPTION))
                    result.append_child(key.c_str()).append_child(pugi::node_pcdata).set_value(str.c_str());
                else if (upnpClass == UPNP_CLASS_MUSIC_TRACK && key == MetadataHandler::getMetaFieldName(M_TRACKNUMBER))
                    result.append_child(key.c_str()).append_child(pugi::node_pcdata).set_value(str.c_str());
                else if (key != MetadataHandler::getMetaFieldName(M_TITLE))
                    addField(result, key, limitString(str));
            }
        }
        auto meta = obj->getMetaData();
        auto [url, artAdded] = renderItemImage(virtualURL, item);
        if (artAdded) {
            meta.emplace_back(MetadataHandler::getMetaFieldName(M_ALBUMARTURI), url);
        }

        addPropertyList(result, meta, auxData, CFG_UPNP_TITLE_PROPERTIES, CFG_UPNP_TITLE_NAMESPACES);
        addResources(item, result, quirks);

        result.set_name("item");
    } else if (obj->isContainer()) {
        auto cont = std::static_pointer_cast<CdsContainer>(obj);

        result.set_name("container");
        int childCount = cont->getChildCount();
        if (childCount >= 0)
            result.append_attribute("childCount") = childCount;

        log_debug("container is class: {}", upnpClass.c_str());
        auto&& meta = obj->getMetaData();
        if (upnpClass == UPNP_CLASS_MUSIC_ALBUM) {
            addPropertyList(result, meta, auxData, CFG_UPNP_ALBUM_PROPERTIES, CFG_UPNP_ALBUM_NAMESPACES);
        } else if (upnpClass == UPNP_CLASS_MUSIC_ARTIST) {
            addPropertyList(result, meta, auxData, CFG_UPNP_ARTIST_PROPERTIES, CFG_UPNP_ARTIST_NAMESPACES);
        } else if (upnpClass == UPNP_CLASS_MUSIC_GENRE) {
            addPropertyList(result, meta, auxData, CFG_UPNP_GENRE_PROPERTIES, CFG_UPNP_GENRE_NAMESPACES);
        } else if (upnpClass == UPNP_CLASS_PLAYLIST_CONTAINER) {
            addPropertyList(result, meta, auxData, CFG_UPNP_PLAYLIST_PROPERTIES, CFG_UPNP_PLAYLIST_NAMESPACES);
        }
        if (upnpClass == UPNP_CLASS_MUSIC_ALBUM || upnpClass == UPNP_CLASS_MUSIC_ARTIST || upnpClass == UPNP_CLASS_CONTAINER || upnpClass == UPNP_CLASS_PLAYLIST_CONTAINER) {
            auto [url, artAdded] = renderContainerImage(virtualURL, cont);
            if (artAdded) {
                result.append_child(MetadataHandler::getMetaFieldName(M_ALBUMARTURI).c_str()).append_child(pugi::node_pcdata).set_value(url.c_str());
            }
        }
    }
    log_debug("Rendered DIDL: {}", printXml(result, "  "));
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
    root.append_attribute("xmlns") = UPNP_DESC_DEVICE_NAMESPACE;
    root.append_attribute(UPNP_XML_SEC_NAMESPACE_ATTR) = UPNP_XML_SEC_NAMESPACE;

    auto specVersion = root.append_child("specVersion");
    specVersion.append_child("major").append_child(pugi::node_pcdata).set_value(UPNP_DESC_SPEC_VERSION_MAJOR);
    specVersion.append_child("minor").append_child(pugi::node_pcdata).set_value(UPNP_DESC_SPEC_VERSION_MINOR);

    auto device = root.append_child("device");

    auto dlnaDoc = device.append_child("dlna:X_DLNADOC");
    dlnaDoc.append_attribute(UPNP_XML_DLNA_NAMESPACE_ATTR) = UPNP_XML_DLNA_NAMESPACE;
    dlnaDoc.append_child(pugi::node_pcdata).set_value("DMS-1.50");
    // dlnaDoc.append_child(pugi::node_pcdata).set_value("M-DMS-1.50");

    constexpr auto deviceProperties = std::array {
        std::pair("friendlyName", CFG_SERVER_NAME),
        std::pair("manufacturer", CFG_SERVER_MANUFACTURER),
        std::pair("manufacturerURL", CFG_SERVER_MANUFACTURER_URL),
        std::pair("modelDescription", CFG_SERVER_MODEL_DESCRIPTION),
        std::pair("modelName", CFG_SERVER_MODEL_NAME),
        std::pair("modelNumber", CFG_SERVER_MODEL_NUMBER),
        std::pair("modelURL", CFG_SERVER_MODEL_URL),
        std::pair("serialNumber", CFG_SERVER_SERIAL_NUMBER),
        std::pair("UDN", CFG_SERVER_UDN),
    };
    for (auto&& [tag, field] : deviceProperties) {
        device.append_child(tag).append_child(pugi::node_pcdata).set_value(config->getOption(field).c_str());
    }
    const auto deviceStringProperties = std::array {
        std::pair("deviceType", UPNP_DESC_DEVICE_TYPE),
        std::pair("presentationURL", presentationURL.empty() ? "/" : presentationURL.c_str()),
        std::pair("sec:ProductCap", UPNP_DESC_PRODUCT_CAPS),
        std::pair("sec:X_ProductCap", UPNP_DESC_PRODUCT_CAPS), // used by SAMSUNG
    };
    for (auto&& [tag, value] : deviceStringProperties) {
        device.append_child(tag).append_child(pugi::node_pcdata).set_value(value);
    }

    // add icons
    {
        auto iconList = device.append_child("iconList");

        constexpr auto iconDims = std::array {
            std::pair("120", "24"),
            std::pair("48", "24"),
            std::pair("32", "8"),
        };

        constexpr auto iconTypes = std::array {
            std::pair(UPNP_DESC_ICON_PNG_MIMETYPE, ".png"),
            std::pair(UPNP_DESC_ICON_BMP_MIMETYPE, ".bmp"),
            std::pair(UPNP_DESC_ICON_JPG_MIMETYPE, ".jpg"),
        };

        for (auto&& [dim, depth] : iconDims) {
            for (auto&& [mimetype, ext] : iconTypes) {
                auto icon = iconList.append_child("icon");
                icon.append_child("mimetype").append_child(pugi::node_pcdata).set_value(mimetype);
                icon.append_child("width").append_child(pugi::node_pcdata).set_value(dim);
                icon.append_child("height").append_child(pugi::node_pcdata).set_value(dim);
                icon.append_child("depth").append_child(pugi::node_pcdata).set_value(depth);
                std::string url = fmt::format("/icons/mt-icon{}{}", dim, ext);
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
            const char* scpdurl;
            const char* controlURL;
            const char* eventSubURL;
        };
        constexpr auto services = std::array<ServiceInfo, 3> { {
            // cm
            { UPNP_DESC_CM_SERVICE_TYPE, UPNP_DESC_CM_SERVICE_ID, UPNP_DESC_CM_SCPD_URL, UPNP_DESC_CM_CONTROL_URL, UPNP_DESC_CM_EVENT_URL },
            // cds
            { UPNP_DESC_CDS_SERVICE_TYPE, UPNP_DESC_CDS_SERVICE_ID, UPNP_DESC_CDS_SCPD_URL, UPNP_DESC_CDS_CONTROL_URL, UPNP_DESC_CDS_EVENT_URL },
            // media receiver registrar service for the Xbox 360
            { UPNP_DESC_MRREG_SERVICE_TYPE, UPNP_DESC_MRREG_SERVICE_ID, UPNP_DESC_MRREG_SCPD_URL, UPNP_DESC_MRREG_CONTROL_URL, UPNP_DESC_MRREG_EVENT_URL },
        } };

        for (auto&& s : services) {
            auto service = serviceList.append_child("service");
            service.append_child("serviceType").append_child(pugi::node_pcdata).set_value(s.serviceType);
            service.append_child("serviceId").append_child(pugi::node_pcdata).set_value(s.serviceId);
            service.append_child("SCPDURL").append_child(pugi::node_pcdata).set_value(s.scpdurl);
            service.append_child("controlURL").append_child(pugi::node_pcdata).set_value(s.controlURL);
            service.append_child("eventSubURL").append_child(pugi::node_pcdata).set_value(s.eventSubURL);
        }
    }

    return doc;
}

void UpnpXMLBuilder::renderResource(const std::string& url, const std::map<std::string, std::string>& attributes, pugi::xml_node& parent)
{
    auto res = parent.append_child("res");
    res.append_child(pugi::node_pcdata).set_value(url.c_str());

    for (auto&& attCfg : attributes) {
        auto attrFound = std::find_if(privateAttributes.begin(), privateAttributes.end(),
            [=](auto&& att) { return attCfg.first == MetadataHandler::getResAttrName(att); });
        if (attrFound == privateAttributes.end())
            res.append_attribute(attCfg.first.c_str()) = attCfg.second.c_str();
    }
}

std::unique_ptr<UpnpXMLBuilder::PathBase> UpnpXMLBuilder::getPathBase(const std::shared_ptr<CdsItem>& item, bool forceLocal)
{
    /// \todo resource options must be read from configuration files

    std::map<std::string, std::string> dict;
    dict[URL_OBJECT_ID] = fmt::to_string(item->getID());

    /// \todo move this down into the "for" loop and create different urls
    /// for each resource once the io handlers are ready    int objectType = ;
    if (item->isExternalItem()) {
        if (!item->getFlag(OBJECT_FLAG_PROXY_URL) && (!forceLocal)) {
            return std::make_unique<PathBase>(item->getLocation(), false);
        }

        if ((item->getFlag(OBJECT_FLAG_ONLINE_SERVICE) && item->getFlag(OBJECT_FLAG_PROXY_URL)) || forceLocal) {
            auto path = RequestHandler::joinUrl({ CONTENT_ONLINE_HANDLER, dictEncodeSimple(dict), URL_RESOURCE_ID }, true);
            return std::make_unique<PathBase>(path, true);
        }
    }
    auto path = RequestHandler::joinUrl({ CONTENT_MEDIA_HANDLER, dictEncodeSimple(dict), URL_RESOURCE_ID }, true);
    return std::make_unique<PathBase>(path, true);
}

/// \brief build path for first resource from item
/// depending on the item type it returns the url to the media
std::string UpnpXMLBuilder::getFirstResourcePath(const std::shared_ptr<CdsItem>& item)
{
    auto urlBase = getPathBase(item);

    if (item->isExternalItem() && !urlBase->addResID) { // a remote resource
        return urlBase->pathBase;
    }

    if (urlBase->addResID) { // a proxy, remote, resource
        return fmt::format(SERVER_VIRTUAL_DIR "{}0", urlBase->pathBase);
    }

    // a local resource
    return fmt::format(SERVER_VIRTUAL_DIR "{}", urlBase->pathBase);
}

std::pair<std::string, bool> UpnpXMLBuilder::renderContainerImage(const std::string& virtualURL, const std::shared_ptr<CdsContainer>& cont)
{
    auto orderedResources = getOrderedResources(cont);
    for (auto&& res : orderedResources) {
        if (!res->isMetaResource(ID3_ALBUM_ART))
            continue;

        auto resFile = res->getAttribute(R_RESOURCE_FILE);
        auto resObj = res->getAttribute(R_FANART_OBJ_ID);
        if (!resFile.empty()) {
            // found, FanArtHandler deals already with file
            std::map<std::string, std::string> dict;
            dict[URL_OBJECT_ID] = fmt::to_string(cont->getID());

            auto resParams = res->getParameters();
            auto url = virtualURL + RequestHandler::joinUrl({ CONTENT_MEDIA_HANDLER, dictEncodeSimple(dict), URL_RESOURCE_ID, fmt::to_string(res->getResId()), dictEncodeSimple(resParams) });
            return { url, true };
        }

        if (!resObj.empty()) {
            std::map<std::string, std::string> dict;
            dict[URL_OBJECT_ID] = resObj;

            auto resParams = res->getParameters();
            auto url = virtualURL + RequestHandler::joinUrl({ CONTENT_MEDIA_HANDLER, dictEncodeSimple(dict), URL_RESOURCE_ID, res->getAttribute(R_FANART_RES_ID), dictEncodeSimple(resParams) });
            return { url, true };
        }
    }
    return {};
}

std::string UpnpXMLBuilder::renderOneResource(const std::string& virtualURL, const std::shared_ptr<CdsItem>& item, const std::shared_ptr<CdsResource>& res)
{
    auto resParams = res->getParameters();
    auto urlBase = getPathBase(item);
    if (item->isExternalItem() && res->getHandlerType() == CH_DEFAULT)
        return item->getLocation();
    std::string url;
    if (urlBase->addResID) {
        url = fmt::format("{}{}{}{}", virtualURL, urlBase->pathBase, res->getResId(), _URL_PARAM_SEPARATOR);
    } else
        url = virtualURL + urlBase->pathBase;

    if (!resParams.empty()) {
        url.append(dictEncodeSimple(resParams));
    }
    return url;
}

std::pair<std::string, bool> UpnpXMLBuilder::renderItemImage(const std::string& virtualURL, const std::shared_ptr<CdsItem>& item)
{
    auto orderedResources = getOrderedResources(item);
    auto resFound = std::find_if(orderedResources.begin(), orderedResources.end(),
        [](auto&& res) { return res->isMetaResource(ID3_ALBUM_ART) //
                             || (res->getHandlerType() == CH_LIBEXIF && res->getParameter(RESOURCE_CONTENT_TYPE) == EXIF_THUMBNAIL) //
                             || (res->getHandlerType() == CH_FFTH && res->getOption(RESOURCE_CONTENT_TYPE) == THUMBNAIL); });
    if (resFound != orderedResources.end()) {
        return { renderOneResource(virtualURL, item, *resFound), true };
    }

    return {};
}

std::pair<std::string, bool> UpnpXMLBuilder::renderSubtitle(const std::string& virtualURL, const std::shared_ptr<CdsItem>& item)
{
    const auto& resources = item->getResources();
    auto resFound = std::find_if(resources.begin(), resources.end(),
        [](auto&& res) { return res->isMetaResource(VIDEO_SUB, CH_SUBTITLE); });
    if (resFound != resources.end()) {
        auto url = renderOneResource(virtualURL, item, *resFound) + renderExtension("", (*resFound)->getAttribute(R_RESOURCE_FILE));
        return { url, true };
    }
    return {};
}

std::string UpnpXMLBuilder::renderExtension(const std::string& contentType, const fs::path& location)
{
    auto urlExt = RequestHandler::joinUrl({ URL_FILE_EXTENSION, "file." });

    if (!contentType.empty() && (contentType != CONTENT_TYPE_PLAYLIST)) {
        return fmt::format("{}{}", urlExt, contentType);
    }

    if (!location.empty() && location.has_extension()) {
        // make sure that the filename does not contain the separator character
        std::string filename = urlEscape(location.filename().stem().string());
        std::string extension = location.filename().extension();
        return fmt::format("{}{}{}", urlExt, filename, extension);
    }

    return {};
}

std::string UpnpXMLBuilder::dlnaProfileString(const std::shared_ptr<CdsResource>& res, const std::string& contentType)
{
    std::string dlnaProfile = res->getOption("dlnaProfile");
    if (contentType == CONTENT_TYPE_JPG) {
        auto resAttrs = res->getAttributes();
        std::string resolution = getValueOrDefault(resAttrs, MetadataHandler::getResAttrName(R_RESOLUTION));
        auto [x, y] = checkResolution(resolution);
        if ((res->getResId() > 0) && !resolution.empty() && x && y) {
            if ((((res->getHandlerType() == CH_LIBEXIF) && (res->getParameter(RESOURCE_CONTENT_TYPE) == EXIF_THUMBNAIL)) || (res->getOption(RESOURCE_CONTENT_TYPE) == EXIF_THUMBNAIL) || (res->getOption(RESOURCE_CONTENT_TYPE) == THUMBNAIL)) && (x <= 160) && (y <= 160))
                return fmt::format("{}={};", UPNP_DLNA_PROFILE, UPNP_DLNA_PROFILE_JPEG_TN);
            if ((x <= 640) && (y <= 420))
                return fmt::format("{}={};", UPNP_DLNA_PROFILE, UPNP_DLNA_PROFILE_JPEG_SM);
            if ((x <= 1024) && (y <= 768))
                return fmt::format("{}={};", UPNP_DLNA_PROFILE, UPNP_DLNA_PROFILE_JPEG_MED);
            if ((x <= 4096) && (y <= 4096))
                return fmt::format("{}={};", UPNP_DLNA_PROFILE, UPNP_DLNA_PROFILE_JPEG_LRG);
        }
    }
    /* handle audio/video content */
    return dlnaProfile.empty() ? getDLNAprofileString(config, contentType, res->getAttribute(R_VIDEOCODEC), res->getAttribute(R_AUDIOCODEC)) : fmt::format("{}={};", UPNP_DLNA_PROFILE, dlnaProfile);
}

std::deque<std::shared_ptr<CdsResource>> UpnpXMLBuilder::getOrderedResources(const std::shared_ptr<CdsObject>& object)
{
    // Order resources according to index defined by orderedHandler
    std::deque<std::shared_ptr<CdsResource>> orderedResources;
    auto&& resources = object->getResources();
    for (int oh : orderedHandler) {
        std::copy_if(resources.begin(), resources.end(), std::back_inserter(orderedResources), [oh](auto&& res) { return oh == res->getHandlerType(); });
    }

    // Append resources not listed in orderedHandler
    for (auto&& res : resources) {
        int ch = res->getHandlerType();
        if (std::find(orderedHandler.begin(), orderedHandler.end(), ch) == orderedHandler.end()) {
            orderedResources.push_back(res);
        }
    }
    return orderedResources;
}

std::pair<bool, int> UpnpXMLBuilder::insertTempTranscodingResource(const std::shared_ptr<CdsItem>& item, const std::unique_ptr<Quirks>& quirks, std::deque<std::shared_ptr<CdsResource>>& orderedResources, bool skipURL)
{
    bool hideOriginalResource = false;
    int originalResource = -1;

    // once proxying is a feature that can be turned off or on in
    // config manager we should use that setting
    //
    // TODO: allow transcoding for URLs
    // now get the profile
    auto tlist = config->getTranscodingProfileListOption(CFG_TRANSCODING_PROFILE_LIST);
    auto tpMt = tlist->get(item->getMimeType());
    if (tpMt) {
        for (auto&& [key, tp] : *tpMt) {
            if (!tp)
                throw_std_runtime_error("Invalid profile encountered");
            // check for client profile prop and filter if no match
            if (quirks && tp->getClientFlags() > 0 && quirks->checkFlags(tp->getClientFlags()) == 0)
                continue;
            std::string ct = getValueOrDefault(ctMappings, item->getMimeType());
            if (ct == CONTENT_TYPE_OGG) {
                if (((item->getFlag(OBJECT_FLAG_OGG_THEORA)) && (!tp->isTheora())) || (!item->getFlag(OBJECT_FLAG_OGG_THEORA) && (tp->isTheora()))) {
                    continue;
                }
            } else if (ct == CONTENT_TYPE_AVI) {
                // check user fourcc settings
                avi_fourcc_listmode_t fccMode = tp->getAVIFourCCListMode();

                const auto& fccList = tp->getAVIFourCCList();
                // mode is either process or ignore, so we will have to take a
                // look at the settings
                if (fccMode != FCC_None) {
                    std::string currentFcc = item->getResource(0)->getOption(RESOURCE_OPTION_FOURCC);
                    // we can not do much if the item has no fourcc info,
                    // so we will transcode it anyway
                    if (currentFcc.empty()) {
                        // the process mode specifies that we will transcode
                        // ONLY if the fourcc matches the list; since an invalid
                        // fourcc can not match anything we will skip the item
                        if (fccMode == FCC_Process)
                            continue;
                    } else {
                        // we have the current and hopefully valid fcc string
                        // let's have a look if it matches the list
                        bool fccMatch = std::find(fccList.begin(), fccList.end(), currentFcc) != fccList.end();
                        if (!fccMatch && (fccMode == FCC_Process))
                            continue;

                        if (fccMatch && (fccMode == FCC_Ignore))
                            continue;
                    }
                }
            }

            auto tRes = std::make_shared<CdsResource>(CH_TRANSCODE);
            tRes->setResId(std::numeric_limits<int>::max());
            tRes->addParameter(URL_PARAM_TRANSCODE_PROFILE_NAME, tp->getName());
            // after transcoding resource was added we can not rely on
            // index 0, so we will make sure the ogg option is there
            tRes->addOption(CONTENT_TYPE_OGG, item->getResource(0)->getOption(CONTENT_TYPE_OGG));
            tRes->addParameter(URL_PARAM_TRANSCODE, URL_VALUE_TRANSCODE);

            std::string targetMimeType = tp->getTargetMimeType();

            if (!tp->isThumbnail()) {
                // duration should be the same for transcoded media, so we can
                // take the value from the original resource
                std::string duration = item->getResource(0)->getAttribute(R_DURATION);
                if (!duration.empty())
                    tRes->addAttribute(R_DURATION, duration);

                int freq = tp->getSampleFreq();
                if (freq == SOURCE) {
                    std::string frequency = item->getResource(0)->getAttribute(R_SAMPLEFREQUENCY);
                    if (!frequency.empty()) {
                        tRes->addAttribute(R_SAMPLEFREQUENCY, frequency);
                        targetMimeType.append(fmt::format(";rate={}", frequency));
                    }
                } else if (freq != OFF) {
                    tRes->addAttribute(R_SAMPLEFREQUENCY, fmt::to_string(freq));
                    targetMimeType.append(fmt::format(";rate={}", freq));
                }

                int chan = tp->getNumChannels();
                if (chan == SOURCE) {
                    std::string nchannels = item->getResource(0)->getAttribute(R_NRAUDIOCHANNELS);
                    if (!nchannels.empty()) {
                        tRes->addAttribute(R_NRAUDIOCHANNELS, nchannels);
                        targetMimeType.append(fmt::format(";channels={}", nchannels));
                    }
                } else if (chan != OFF) {
                    tRes->addAttribute(R_NRAUDIOCHANNELS, fmt::to_string(chan));
                    targetMimeType.append(fmt::format(";channels={}", chan));
                }
            }

            tRes->addAttribute(R_PROTOCOLINFO, renderProtocolInfo(targetMimeType));

            if (tp->isThumbnail())
                tRes->addOption(RESOURCE_CONTENT_TYPE, EXIF_THUMBNAIL);

            tRes->mergeAttributes(tp->getAttributes());

            if (!tp->dlnaProfile().empty())
                tRes->addOption("dlnaProfile", tp->dlnaProfile());

            if (tp->hideOriginalResource())
                hideOriginalResource = true;

            auto urlBase = [&] {
                if (skipURL)
                    return getPathBase(item, true);
                return getPathBase(item);
            }();
            tRes->addOption("urlBase", fmt::format("{}{}", urlBase->pathBase, URL_VALUE_TRANSCODE_NO_RES_ID));

            if (tp->firstResource()) {
                orderedResources.push_front(std::move(tRes));
                originalResource = 0;
            } else
                orderedResources.push_back(std::move(tRes));
        }
    }

    return { hideOriginalResource, originalResource };
}

void UpnpXMLBuilder::addResources(const std::shared_ptr<CdsItem>& item, pugi::xml_node& parent, const std::unique_ptr<Quirks>& quirks)
{
    auto urlBase = getPathBase(item);
    bool skipURL = (item->isExternalItem() && !item->getFlag(OBJECT_FLAG_PROXY_URL));

    auto orderedResources = getOrderedResources(item);

    std::vector<std::map<std::string, std::string>> captionInfoEx;
    for (auto&& res : orderedResources) {
        auto resAttrs = res->getAttributes();
        auto&& resParams = res->getParameters();
        std::string protocolInfo = getValueOrDefault(resAttrs, MetadataHandler::getResAttrName(R_PROTOCOLINFO));
        std::string url;

        if (urlBase->addResID) {
            url = fmt::format("{}{}", urlBase->pathBase, res->getResId());
        } else
            url = urlBase->pathBase;

        if (!resParams.empty()) {
            url.append(_URL_PARAM_SEPARATOR);
            url.append(dictEncodeSimple(resParams));
        }

        int handlerType = res->getHandlerType();
        if (res->getResId() > 0 && handlerType == CH_EXTURL && (res->getOption(RESOURCE_CONTENT_TYPE) == THUMBNAIL || res->getOption(RESOURCE_CONTENT_TYPE) == ID3_ALBUM_ART)) {
            url = res->getOption(RESOURCE_OPTION_URL);
            if (url.empty())
                throw_std_runtime_error("missing thumbnail URL");
        }

        // resource is used to point to album art
        // only add upnp:AlbumArtURI if we have an AA, skip the resource
        if (res->isMetaResource(ID3_ALBUM_ART) //
            || (res->getHandlerType() == CH_LIBEXIF && res->getParameter(RESOURCE_CONTENT_TYPE) == EXIF_THUMBNAIL) //
            || (res->getHandlerType() == CH_FFTH && res->getOption(RESOURCE_CONTENT_TYPE) == THUMBNAIL) //
        ) {
            auto aa = parent.append_child(MetadataHandler::getMetaFieldName(M_ALBUMARTURI).c_str());
            aa.append_child(pugi::node_pcdata).set_value((virtualURL + url).c_str());

            /// \todo clean this up, make sure to check the mimetype and
            /// provide the profile correctly
            aa.append_attribute(UPNP_XML_DLNA_NAMESPACE_ATTR) = UPNP_XML_DLNA_METADATA_NAMESPACE;
            aa.append_attribute("dlna:profileID") = "JPEG_TN";
        }
        if (res->isMetaResource(VIDEO_SUB, CH_SUBTITLE)) {
            auto captionInfo = std::map<std::string, std::string>();
            auto subUrl = url;
            subUrl.append(renderExtension("", res->getAttribute(R_RESOURCE_FILE)));
            captionInfo[""] = virtualURL + subUrl;
            captionInfo["sec:type"] = res->getAttribute(R_TYPE).empty() ? res->getParameter("type") : res->getAttribute(R_TYPE);
            if (!res->getAttribute(R_LANGUAGE).empty())
                captionInfo[MetadataHandler::getResAttrName(R_LANGUAGE)] = res->getAttribute(R_LANGUAGE);
            captionInfo[MetadataHandler::getResAttrName(R_PROTOCOLINFO)] = protocolInfo;
            captionInfoEx.push_back(std::move(captionInfo));
        }
    }
    if (!captionInfoEx.empty()) {
        auto count = (quirks && quirks->getCaptionInfoCount() > -1) ? quirks->getCaptionInfoCount() : config->getIntOption(CFG_UPNP_CAPTION_COUNT);
        for (auto&& captionInfo : captionInfoEx) {
            count--;
            if (count < 0)
                break;
            auto vs = parent.append_child("sec:CaptionInfoEx");
            for (auto&& [key, val] : captionInfo) {
                if (key.empty()) {
                    vs.append_child(pugi::node_pcdata).set_value(val.c_str());
                } else {
                    vs.append_attribute(key.c_str()) = val.c_str();
                }
            }
        }
    }

    auto [hideOriginalResource, originalResource] = insertTempTranscodingResource(item, quirks, orderedResources, skipURL);

    bool isExtThumbnail = false;
    for (auto&& res : orderedResources) {
        auto resAttrs = res->getAttributes();
        auto&& resParams = res->getParameters();
        std::string protocolInfo = getValueOrDefault(resAttrs, MetadataHandler::getResAttrName(R_PROTOCOLINFO));
        std::string mimeType = getMTFromProtocolInfo(protocolInfo);

        auto pos = mimeType.find(';');
        if (pos != std::string::npos) {
            mimeType = mimeType.substr(0, pos);
        }

        assert(!mimeType.empty());
        std::string contentType = getValueOrDefault(ctMappings, mimeType);
        std::string url;

        /// \todo who will sync mimetype that is part of the protocol info and
        /// that is lying in the resources with the information that is in the
        /// resource tags?

        // ok, here is the tricky part:
        // we add transcoded resources dynamically, that means that when
        // the object is loaded from database those resources are not there;
        // this again means, that we have to add the res_id parameter
        // accounting for those dynamic resources: i.e. the parameter should
        // still only count the "real" resources, because that's what the
        // file request handler will be getting.
        // for transcoded resources the res_id can be safely ignored,
        // because a transcoded resource is identified by the profile name
        // flag if we are dealing with the transcoded resource
        bool transcoded = (getValueOrDefault(resParams, URL_PARAM_TRANSCODE) == URL_VALUE_TRANSCODE);
        if (!transcoded) {
            if (urlBase->addResID) {
                url = fmt::format("{}{}", urlBase->pathBase, res->getResId());
            } else
                url = urlBase->pathBase;
        } else {
            url = res->getOption("urlBase");
        }
        if (!resParams.empty()) {
            url.append(_URL_PARAM_SEPARATOR);
            url.append(dictEncodeSimple(resParams));
        }

        // ok this really sucks, I guess another rewrite of the resource manager
        // is necessary
        int handlerType = res->getHandlerType();
        if (res->getResId() > 0 && handlerType == CH_EXTURL && (res->getOption(RESOURCE_CONTENT_TYPE) == THUMBNAIL || res->getOption(RESOURCE_CONTENT_TYPE) == ID3_ALBUM_ART)) {
            url = res->getOption(RESOURCE_OPTION_URL);
            if (url.empty())
                throw_std_runtime_error("missing thumbnail URL");

            isExtThumbnail = true;
        }

        // resource is used to point to album art
        // only add upnp:AlbumArtURI if we have an AA, skip the resource
        if (res->isMetaResource(ID3_ALBUM_ART)) {
            continue;
        }

        if (handlerType == CH_DEFAULT && !captionInfoEx.empty() && quirks && quirks->checkFlags(QUIRK_FLAG_PV_SUBTITLES)) {
            auto captionInfo = captionInfoEx[0];
            resAttrs["pv:subtitleFileType"] = toUpper(captionInfo["sec:type"]);
            resAttrs["pv:subtitleFileUri"] = captionInfo[""];
        }

        if (!isExtThumbnail) {
            // when transcoding is enabled the first (zero) resource can be the
            // transcoded stream, that means that we can only go with the
            // content type here and that we will not limit ourselves to the
            // first resource
            if (!skipURL) {
                auto ext = renderExtension("", res->getAttribute(R_RESOURCE_FILE)); // try extension from resource file
                if (ext.empty())
                    ext = renderExtension(contentType, transcoded ? "" : item->getLocation());
                url.append(ext);
            }
        }

        std::string extend = dlnaProfileString(res, contentType);

        // we do not support seeking at all, so 00
        // and the media is converted, so set CI to 1
        if (!isExtThumbnail && transcoded) {
            extend.append(fmt::format("{}={};{}={}", UPNP_DLNA_OP, UPNP_DLNA_OP_SEEK_DISABLED, UPNP_DLNA_CONVERSION_INDICATOR, UPNP_DLNA_CONVERSION));
        } else {
            extend.append(fmt::format("{}={};{}={}", UPNP_DLNA_OP, UPNP_DLNA_OP_SEEK_RANGE, UPNP_DLNA_CONVERSION_INDICATOR, UPNP_DLNA_NO_CONVERSION));
        }
        std::string dlnaFlags;
        if (startswith(mimeType, "audio") || startswith(mimeType, "video"))
            dlnaFlags = UPNP_DLNA_ORG_FLAGS_AV;
        else if (startswith(mimeType, "image"))
            dlnaFlags = UPNP_DLNA_ORG_FLAGS_IMAGE;
        if (res->isMetaResource(VIDEO_SUB, CH_SUBTITLE)) {
            dlnaFlags = UPNP_DLNA_ORG_FLAGS_SUB;
        }
        if (!dlnaFlags.empty())
            extend.append(fmt::format(";{}={}", UPNP_DLNA_FLAGS, dlnaFlags));

        protocolInfo = protocolInfo.substr(0, protocolInfo.rfind(':') + 1).append(extend);
        resAttrs[MetadataHandler::getResAttrName(R_PROTOCOLINFO)] = protocolInfo;

        log_debug("protocolInfo: {}", protocolInfo.c_str());

        // URL is path until now
        if (!item->isExternalItem() || (hideOriginalResource && item->isExternalItem())) {
            url = fmt::format("{}{}", virtualURL, url);
        }

        if (!hideOriginalResource || transcoded || originalResource != res->getResId())
            renderResource(url, resAttrs, parent);
    }
}
