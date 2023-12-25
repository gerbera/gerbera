/*MT*

    MediaTomb - http://www.mediatomb.cc/

    upnp_xml.cc - this file is part of MediaTomb.

    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>

    Copyright (C) 2006-2010 Gena Batyan <bgeradz@mediatomb.cc>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>,
                            Leonhard Wimmer <leo@mediatomb.cc>

    Copyright (C) 2016-2023 Gerbera Contributors

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
#define LOG_FAC log_facility_t::xml

#include "upnp_xml.h" // API

#include <array>
#include <fmt/chrono.h>
#include <sstream>

#include "cds/cds_container.h"
#include "cds/cds_item.h"
#include "config/config_definition.h"
#include "config/config_manager.h"
#include "database/database.h"
#include "metadata/metadata_handler.h"
#include "request_handler.h"
#include "transcoding/transcoding.h"
#include "util/upnp_clients.h"
#include "util/url_utils.h"

#define URL_FILE_EXTENSION "ext"

UpnpXMLBuilder::UpnpXMLBuilder(const std::shared_ptr<Context>& context,
    std::string virtualUrl, std::string presentationURL)
    : config(context->getConfig())
    , database(context->getDatabase())
    , virtualURL(std::move(virtualUrl))
    , presentationURL(std::move(presentationURL))
{
    for (auto&& entry : this->config->getArrayOption(CFG_IMPORT_RESOURCES_ORDER)) {
        orderedHandler.push_back(MetadataHandler::remapContentHandler(entry));
    }
    entrySeparator = config->getOption(CFG_IMPORT_LIBOPTS_ENTRY_SEP);
    multiValue = config->getBoolOption(CFG_UPNP_MULTI_VALUES_ENABLED);
    ctMappings = config->getDictionaryOption(CFG_IMPORT_MAPPINGS_MIMETYPE_TO_CONTENTTYPE_LIST);
    profMappings = config->getVectorOption(CFG_IMPORT_MAPPINGS_CONTENTTYPE_TO_DLNAPROFILE_LIST);
    transferMappings = config->getDictionaryOption(CFG_IMPORT_MAPPINGS_CONTENTTYPE_TO_DLNATRANSFER_LIST);
}

std::unique_ptr<pugi::xml_document> UpnpXMLBuilder::createResponse(const std::string& actionName, const std::string& serviceType) const
{
    auto response = std::make_unique<pugi::xml_document>();
    auto root = response->append_child(fmt::format("u:{}Response", actionName).c_str());
    root.append_attribute("xmlns:u") = serviceType.c_str();

    return response;
}

static std::string encodeEscapes(std::string& s)
{
    replaceAllString(s, "&", "&amp;");
    replaceAllString(s, "'", "&apos;");
    replaceAllString(s, "<", "&lt;");
    replaceAllString(s, ">", "&gt;");
    replaceAllString(s, "\"", "&quote;");
    return s;
}

static constexpr std::string_view ellipse("...");

static std::string limitString(std::size_t stringLimit, const std::string& s)
{
    // Do nothing if string is already short enough
    if (s.length() <= stringLimit)
        return s;

    ssize_t cutPosition = getValidUTF8CutPosition(s, stringLimit - ellipse.size());
    return fmt::format("{}{}", s.substr(0, cutPosition), ellipse);
}

static std::string formatXmlString(bool strictXml, std::size_t stringLimit, const std::string& input)
{
    std::string s = input;
    // Do nothing if disabled
    if (strictXml)
        s = encodeEscapes(s);
    // Do nothing if disabled
    if (stringLimit != std::string::npos)
        s = limitString(stringLimit, s);
    return s;
}

void UpnpXMLBuilder::addPropertyList(bool strictXml, std::size_t stringLimit, pugi::xml_node& result, const std::vector<std::pair<std::string, std::string>>& meta, const std::map<std::string, std::string>& auxData,
    config_option_t itemProps, config_option_t nsProp) const
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
                addField(result, tag, formatXmlString(strictXml, stringLimit, mvalue));
                wasMeta = true;
            }
        }
        if (!wasMeta) {
            auto avalue = getValueOrDefault(auxData, field);
            if (!avalue.empty()) {
                addField(result, tag, formatXmlString(strictXml, stringLimit, avalue));
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

void UpnpXMLBuilder::addField(pugi::xml_node& entry, const std::string& key, const std::string& val) const
{
    auto i = key.find('@');
    auto j = key.find('[', i + 1);
    if (i != std::string::npos && j != std::string::npos && key[key.length() - 1] == ']') {
        // e.g. used for M_ALBUMARTIST
        // name@attr[val] => <name attr="val">
        std::string attrName = key.substr(i + 1, j - i - 1);
        std::string attrValue = key.substr(j + 1, key.length() - j - 2);
        std::string name = key.substr(0, i);
        auto node = entry.append_child(name.c_str());
        node.append_attribute(attrName.c_str()) = attrValue.c_str();
        node.append_child(pugi::node_pcdata).set_value(val.c_str());
    } else if (i != std::string::npos) {
        // name@attr val => <name attr="val">
        std::string name = key.substr(0, i);
        std::string attrName = key.substr(i + 1);
        auto child = entry.child(name.c_str());
        if (child) {
            child.append_attribute(attrName.c_str()) = val.c_str();
        } else {
            entry.append_child(key.c_str()).append_attribute(attrName.c_str()) = val.c_str();
        }
    } else {
        entry.append_child(key.c_str()).append_child(pugi::node_pcdata).set_value(val.c_str());
    }
}

void UpnpXMLBuilder::renderObject(const std::shared_ptr<CdsObject>& obj, std::size_t stringLimit, pugi::xml_node& parent, const std::unique_ptr<Quirks>& quirks) const
{
    auto result = parent.append_child("");

    result.append_attribute("id") = obj->getID();
    result.append_attribute("parentID") = obj->getParentID();
    result.append_attribute("restricted") = obj->isRestricted() ? "1" : "0";

    auto strictXml = quirks && quirks->needsStrictXml();
    const std::string title = obj->getTitle();
    const std::string upnpClass = obj->getClass();

    result.append_child("dc:title").append_child(pugi::node_pcdata).set_value(formatXmlString(strictXml, stringLimit, title).c_str());
    result.append_child("upnp:class").append_child(pugi::node_pcdata).set_value(upnpClass.c_str());

    auto auxData = obj->getAuxData();
    auto mvMeta = multiValue;
    auto simpleDate = false;

    if (obj->isItem()) {
        auto item = std::static_pointer_cast<CdsItem>(obj);

        // client specific properties
        if (quirks) {
            quirks->restoreSamsungBookMarkedPosition(item, result, config->getIntOption(CFG_CLIENTS_BOOKMARK_OFFSET));
            mvMeta = quirks->getMultiValue();
            simpleDate = quirks->needsSimpleDate();
        }

        auto metaGroups = obj->getMetaGroups();

        // add metadata
        for (auto&& [key, group] : metaGroups) {
            if (mvMeta) {
                for (auto&& val : group) {
                    // Trim metadata value as needed
                    auto str = formatXmlString(strictXml, stringLimit, val);
                    if (key == MetadataHandler::getMetaFieldName(M_DESCRIPTION))
                        result.append_child(key.c_str()).append_child(pugi::node_pcdata).set_value(str.c_str());
                    else if (startswith(upnpClass, UPNP_CLASS_MUSIC_TRACK) && key == MetadataHandler::getMetaFieldName(M_TRACKNUMBER))
                        result.append_child(key.c_str()).append_child(pugi::node_pcdata).set_value(str.c_str());
                    else if (simpleDate && key == MetadataHandler::getMetaFieldName(M_DATE))
                        addField(result, key, makeSimpleDate(str));
                    else if (key != MetadataHandler::getMetaFieldName(M_TITLE))
                        addField(result, key, str);
                }
            } else {
                // Trim metadata value as needed
                auto str = formatXmlString(strictXml, stringLimit, fmt::format("{}", fmt::join(group, entrySeparator)));
                if (key == MetadataHandler::getMetaFieldName(M_DESCRIPTION))
                    result.append_child(key.c_str()).append_child(pugi::node_pcdata).set_value(str.c_str());
                else if (startswith(upnpClass, UPNP_CLASS_MUSIC_TRACK) && key == MetadataHandler::getMetaFieldName(M_TRACKNUMBER))
                    result.append_child(key.c_str()).append_child(pugi::node_pcdata).set_value(str.c_str());
                else if (simpleDate && key == MetadataHandler::getMetaFieldName(M_DATE))
                    addField(result, key, makeSimpleDate(str));
                else if (key != MetadataHandler::getMetaFieldName(M_TITLE))
                    addField(result, key, str);
            }
        }
        auto meta = obj->getMetaData();

        // add thumbnail
        auto artAdded = renderItemImageURL(item);
        if (artAdded) {
            meta.emplace_back(MetadataHandler::getMetaFieldName(M_ALBUMARTURI), artAdded.value());
        }

        // add playback statistics
        auto playStatus = item->getPlayStatus();
        if (playStatus) {
            auxData["upnp:playbackCount"] = fmt::format("{}", playStatus->getPlayCount());
            auxData["upnp:lastPlaybackTime"] = fmt::format("{:%Y-%m-%d T %H:%M:%S}", fmt::localtime(playStatus->getLastPlayed().count()));
            auxData["upnp:lastPlaybackPosition"] = fmt::format("{}", millisecondsToHMSF(playStatus->getLastPlayedPosition().count()));
            addField(result, "upnp:playbackCount", auxData["upnp:playbackCount"]);
            addField(result, "upnp:lastPlaybackTime", auxData["upnp:lastPlaybackTime"]);
            addField(result, "upnp:lastPlaybackPosition", auxData["upnp:lastPlaybackPosition"]);
        }

        addPropertyList(strictXml, stringLimit, result, meta, auxData, CFG_UPNP_TITLE_PROPERTIES, CFG_UPNP_TITLE_NAMESPACES);
        addResources(item, result, quirks);

        result.set_name("item");
    } else if (obj->isContainer()) {
        auto cont = std::static_pointer_cast<CdsContainer>(obj);

        result.set_name("container");
        int childCount = cont->getChildCount();
        if (childCount >= 0)
            result.append_attribute("childCount") = childCount;

        // add metadata
        log_debug("container is class: {}", upnpClass.c_str());
        auto&& meta = obj->getMetaData();
        if (startswith(upnpClass, UPNP_CLASS_MUSIC_ALBUM)) {
            addPropertyList(strictXml, stringLimit, result, meta, auxData, CFG_UPNP_ALBUM_PROPERTIES, CFG_UPNP_ALBUM_NAMESPACES);
        } else if (startswith(upnpClass, UPNP_CLASS_MUSIC_ARTIST)) {
            addPropertyList(strictXml, stringLimit, result, meta, auxData, CFG_UPNP_ARTIST_PROPERTIES, CFG_UPNP_ARTIST_NAMESPACES);
        } else if (startswith(upnpClass, UPNP_CLASS_MUSIC_GENRE)) {
            addPropertyList(strictXml, stringLimit, result, meta, auxData, CFG_UPNP_GENRE_PROPERTIES, CFG_UPNP_GENRE_NAMESPACES);
        } else if (startswith(upnpClass, UPNP_CLASS_PLAYLIST_CONTAINER)) {
            addPropertyList(strictXml, stringLimit, result, meta, auxData, CFG_UPNP_PLAYLIST_PROPERTIES, CFG_UPNP_PLAYLIST_NAMESPACES);
        }
        if (startswith(upnpClass, UPNP_CLASS_MUSIC_ALBUM) || startswith(upnpClass, UPNP_CLASS_MUSIC_ARTIST) || startswith(upnpClass, UPNP_CLASS_CONTAINER) || startswith(upnpClass, UPNP_CLASS_PLAYLIST_CONTAINER)) {
            auto url = renderContainerImageURL(cont);
            if (url) {
                result.append_child(MetadataHandler::getMetaFieldName(M_ALBUMARTURI).data()).append_child(pugi::node_pcdata).set_value(url.value().c_str());
            }
        }
    }

    // make sure a date is set
    auto dateNode = result.child("dc:date");
    if (!dateNode) {
        auto fDate = fmt::format("{:%FT%T%z}", fmt::localtime(obj->getMTime().count()));
        result.append_child("dc:date").append_child(pugi::node_pcdata).set_value(fDate.c_str());
    }
    log_debug("Rendered DIDL: {}", printXml(result, "  "));
}

std::unique_ptr<pugi::xml_document> UpnpXMLBuilder::createEventPropertySet() const
{
    auto doc = std::make_unique<pugi::xml_document>();

    auto propset = doc->append_child("e:propertyset");
    propset.append_attribute("xmlns:e") = "urn:schemas-upnp-org:event-1-0";

    propset.append_child("e:property");

    return doc;
}

std::unique_ptr<pugi::xml_document> UpnpXMLBuilder::renderDeviceDescription() const
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

void UpnpXMLBuilder::renderResource(const CdsObject& object, const CdsResource& resource, pugi::xml_node& parent, const std::map<std::string, std::string>& clientSpecificAttrs, const std::string& clientGroup, const std::map<std::string, std::string>& mimeMappings) const
{
    auto res = parent.append_child("res");

    auto url = renderResourceURL(object, resource, mimeMappings, clientGroup);

    res.append_child(pugi::node_pcdata).set_value(url.c_str());

    for (auto&& [attr, val] : resource.getAttributes()) {
        if (CdsResource::isPrivateAttribute(attr)) {
            continue;
        }
        res.append_attribute(CdsResource::getAttributeName(attr).c_str()) = val.c_str();
    }

    for (auto&& [k, v] : clientSpecificAttrs) {
        res.append_attribute(k.c_str()) = v.c_str();
    }
}

std::string UpnpXMLBuilder::renderResourceURL(const CdsObject& item, const CdsResource& res, const std::map<std::string, std::string>& mimeMappings, const std::string& clientGroup) const
{
    std::string url;

    if (item.isContainer()) {
        auto resFile = res.getAttribute(CdsResource::Attribute::RESOURCE_FILE);
        if (!resFile.empty()) {
            url = virtualURL + URLUtils::joinUrl({ SERVER_VIRTUAL_DIR, CONTENT_MEDIA_HANDLER, URL_OBJECT_ID, fmt::to_string(item.getID()), URL_RESOURCE_ID, fmt::to_string(res.getResId()) });
        }

        auto resObjID = res.getAttribute(CdsResource::Attribute::FANART_OBJ_ID);
        if (!resObjID.empty()) {
            auto objID = stoiString(resObjID);
            auto resID = stoiString(res.getAttribute(CdsResource::Attribute::FANART_RES_ID));
            try {
                auto resObj = (objID > CDS_ID_ROOT && objID != item.getID()) ? database->loadObject(objID) : nullptr;
                while (resObj && resObj->isContainer()) {
                    auto resRes = resObj->getResource(resID);
                    auto subObjID = stoiString(resRes->getAttribute(CdsResource::Attribute::FANART_OBJ_ID));
                    if (subObjID > CDS_ID_ROOT && subObjID != resObj->getID() && resRes->getAttribute(CdsResource::Attribute::RESOURCE_FILE).empty()) {
                        resObj = database->loadObject(subObjID);
                        objID = subObjID;
                        resID = stoiString(resRes->getAttribute(CdsResource::Attribute::FANART_RES_ID));
                    } else {
                        resObj = nullptr;
                    }
                }
            } catch (const std::runtime_error& ex) {
                log_error(" {}", ex.what());
            }
            if (objID <= CDS_ID_ROOT)
                objID = item.getID();
            url = virtualURL + URLUtils::joinUrl({ SERVER_VIRTUAL_DIR, CONTENT_MEDIA_HANDLER, URL_OBJECT_ID, fmt::to_string(objID), URL_RESOURCE_ID, fmt::to_string(resID) });
        }
    } else if (item.isExternalItem()) {
        if (res.getPurpose() == CdsResource::Purpose::Content) {
            // Remote URL is just passed straight out
            // FIXME: OBJECT_FLAG_PROXY_URL and location should be on the resource not the item!
            if (!item.getFlag(OBJECT_FLAG_PROXY_URL)) {
                return item.getLocation();
            }

            // Proxied remote URL
            if (item.getFlag(OBJECT_FLAG_ONLINE_SERVICE) && item.getFlag(OBJECT_FLAG_PROXY_URL)) {
                url = virtualURL + URLUtils::joinUrl({ SERVER_VIRTUAL_DIR, CONTENT_ONLINE_HANDLER, URL_OBJECT_ID, fmt::to_string(item.getID()), URL_RESOURCE_ID, fmt::to_string(res.getResId()) });
            }
        } else if (res.getPurpose() == CdsResource::Purpose::Transcode) {
            // Transcoded resources dont set a resId, uses pr_name from params instead.
            url = virtualURL + URLUtils::joinUrl({ SERVER_VIRTUAL_DIR, CONTENT_ONLINE_HANDLER, URL_OBJECT_ID, fmt::to_string(item.getID()), URL_RESOURCE_ID, URL_VALUE_TRANSCODE_NO_RES_ID });
        }
    }

    // Externally hosted thumnbnails?
    if (res.getPurpose() == CdsResource::Purpose::Thumbnail && res.getHandlerType() == ContentHandler::EXTURL) {
        url = res.getOption(RESOURCE_OPTION_URL);
        if (url.empty())
            throw_std_runtime_error("missing thumbnail URL");
    }

    // Standard Resource
    if (url.empty()) {
        if (res.getPurpose() == CdsResource::Purpose::Transcode) {
            // Transcoded resources dont set a resId, uses pr_name from params instead.
            url = virtualURL + URLUtils::joinUrl({ SERVER_VIRTUAL_DIR, CONTENT_MEDIA_HANDLER, URL_OBJECT_ID, fmt::to_string(item.getID()), URL_RESOURCE_ID, URL_VALUE_TRANSCODE_NO_RES_ID });
        } else {
            url = virtualURL + URLUtils::joinUrl({ SERVER_VIRTUAL_DIR, CONTENT_MEDIA_HANDLER, URL_OBJECT_ID, fmt::to_string(item.getID()), URL_RESOURCE_ID, fmt::to_string(res.getResId()) });
        }
    }

    // Add Params
    auto&& resParams = res.getParameters();
    if (!resParams.empty()) {
        url.append(URL_PARAM_SEPARATOR);
        url.append(URLUtils::dictEncodeSimple(resParams));
    }

    // Inject client group for content
    if (!clientGroup.empty() && (res.getPurpose() == CdsResource::Purpose::Content || res.getPurpose() == CdsResource::Purpose::Transcode)) {
        url.append(URLUtils::joinUrl({ CLIENT_GROUP_TAG, clientGroup }));
    }

    // Add the filename.ext part
    // We don't use this for anything, but it makes various clients happy
    auto lang = res.getAttribute(CdsResource::Attribute::LANGUAGE);
    auto ext = renderExtension("", res.getAttribute(CdsResource::Attribute::RESOURCE_FILE), lang); // try extension from resource file
    if (ext.empty()) {
        auto mimeType = getMimeType(res, mimeMappings);
        auto contentType = getValueOrDefault(ctMappings, mimeType);
        ext = renderExtension(contentType, res.getPurpose() == CdsResource::Purpose::Transcode ? "" : item.getLocation(), lang);
    }
    url.append(ext);

    return url;
}

/// \brief build path for first resource from item
/// depending on the item type it returns the url to the media
std::string UpnpXMLBuilder::getFirstResourcePath(const std::shared_ptr<CdsItem>& item) const
{
    auto res = item->getResource(CdsResource::Purpose::Content);
    if (res)
        return renderResourceURL(*item, *res, {});
    // Fabricate a fake resource for URL based items
    // FIXME: This should be done at object creation
    auto temp = CdsResource { ContentHandler::DEFAULT, CdsResource::Purpose::Content };
    temp.setResId(0);
    return renderResourceURL(*item, temp, {});
}

std::optional<std::string> UpnpXMLBuilder::renderContainerImageURL(const std::shared_ptr<CdsContainer>& cont) const
{
    auto orderedResources = getOrderedResources(*cont);
    auto resFound = std::find_if(orderedResources.begin(), orderedResources.end(), [](auto&& res) { return res->getPurpose() == CdsResource::Purpose::Thumbnail; });
    if (resFound != orderedResources.end()) {
        return renderResourceURL(*cont, **resFound, {});
    }
    return {};
}

std::optional<std::string> UpnpXMLBuilder::renderItemImageURL(const std::shared_ptr<CdsItem>& item) const
{
    auto orderedResources = getOrderedResources(*item);
    auto resFound = std::find_if(orderedResources.begin(), orderedResources.end(), [](auto&& res) { return res->getPurpose() == CdsResource::Purpose::Thumbnail; });
    if (resFound != orderedResources.end()) {
        return renderResourceURL(*item, **resFound, {});
    }
    return {};
}

std::optional<std::string> UpnpXMLBuilder::renderSubtitleURL(const std::shared_ptr<CdsItem>& item, const std::map<std::string, std::string>& mimeMappings) const
{
    auto resFound = item->getResource(CdsResource::Purpose::Subtitle);
    if (resFound) {
        return renderResourceURL(*item, *resFound, mimeMappings);
    }
    return {};
}

std::string UpnpXMLBuilder::renderExtension(const std::string& contentType, const fs::path& location, const std::string& language) const
{
    auto urlExt = URLUtils::joinUrl({ URL_FILE_EXTENSION, "file" });

    if (!contentType.empty() && (contentType != CONTENT_TYPE_PLAYLIST)) {
        return fmt::format("{}.{}", urlExt, contentType);
    }

    if (!location.empty() && location.has_extension()) {
        std::string extension = URLUtils::urlEscape(location.filename().extension().string());
        if (!language.empty())
            return fmt::format("{}.{}{}", urlExt, URLUtils::urlEscape(language), extension);
        return fmt::format("{}{}", urlExt, extension);
    }

    return {};
}

std::string UpnpXMLBuilder::getDLNAContentHeader(const std::string& contentType, const std::shared_ptr<CdsResource>& res) const
{
    std::string contentParameter = dlnaProfileString(*res, contentType);
    return fmt::format("{}{}={};{}={};{}={}", contentParameter, //
        UPNP_DLNA_OP, UPNP_DLNA_OP_SEEK_RANGE, //
        UPNP_DLNA_CONVERSION_INDICATOR, UPNP_DLNA_NO_CONVERSION, //
        UPNP_DLNA_FLAGS, UPNP_DLNA_ORG_FLAGS_AV);
}

std::string UpnpXMLBuilder::getDLNATransferHeader(const std::string& mimeType) const
{
    return getValueOrDefault(transferMappings, mimeType);
}

std::string UpnpXMLBuilder::dlnaProfileString(const CdsResource& res, const std::string& contentType, bool formatted) const
{
    std::string dlnaProfile = res.getOption("dlnaProfile");
    if (contentType == CONTENT_TYPE_JPG) {
        std::string resValue = res.getAttributeValue(CdsResource::Attribute::RESOLUTION);
        if (res.getPurpose() == CdsResource::Purpose::Thumbnail) {
            dlnaProfile = UPNP_DLNA_PROFILE_JPEG_TN;
        }
        if (res.getPurpose() == CdsResource::Purpose::Content && !resValue.empty()) {
            if (resValue == "SD")
                dlnaProfile = UPNP_DLNA_PROFILE_JPEG_SM;
            else if (resValue == "HD")
                dlnaProfile = UPNP_DLNA_PROFILE_JPEG_MED;
            else if (resValue == "UHD")
                dlnaProfile = UPNP_DLNA_PROFILE_JPEG_LRG;
        }
    }
    if (dlnaProfile.empty()) {
        /* handle audio/video content */
        dlnaProfile = findDlnaProfile(res, contentType);
    }

    if (formatted && !dlnaProfile.empty())
        return fmt::format("{}={};", UPNP_DLNA_PROFILE, dlnaProfile);
    return dlnaProfile;
}

std::string UpnpXMLBuilder::findDlnaProfile(const CdsResource& res, const std::string& contentType) const
{
    std::string dlnaProfile;
    static auto fromKey = ConfigDefinition::removeAttribute(ATTR_IMPORT_MAPPINGS_MIMETYPE_FROM);
    static auto toKey = ConfigDefinition::removeAttribute(ATTR_IMPORT_MAPPINGS_MIMETYPE_TO);
    auto legacyKey = fmt::format("{}-{}-{}", contentType, res.getAttribute(CdsResource::Attribute::VIDEOCODEC), res.getAttribute(CdsResource::Attribute::AUDIOCODEC));
    std::size_t matchLength = 0;
    for (auto&& map : profMappings) {
        std::string profCand;
        bool match = true;
        for (auto&& [key, val] : map) {
            if (key == fromKey && (val.empty() || (val != contentType && val != legacyKey))) {
                match = false;
            }
            if (key == toKey && !val.empty()) {
                profCand = val;
            }
            for (auto&& attr : ResourceAttributeIterator()) {
                auto attrName = CdsResource::getAttributeDisplay(attr);
                if (key == attrName && val != res.getAttributeValue(attr) && val != res.getAttribute(attr)) {
                    match = false;
                }
            }
        }
        if (match && matchLength < map.size() && !profCand.empty()) {
            matchLength = map.size();
            dlnaProfile = profCand;
        }
    }
    return dlnaProfile;
}

std::deque<std::shared_ptr<CdsResource>> UpnpXMLBuilder::getOrderedResources(const CdsObject& object) const
{
    // Order resources according to index defined by orderedHandler
    std::deque<std::shared_ptr<CdsResource>> orderedResources;
    auto&& resources = object.getResources();
    for (ContentHandler oh : orderedHandler) {
        std::copy_if(resources.begin(), resources.end(), std::back_inserter(orderedResources), [oh](auto&& res) { return oh == res->getHandlerType(); });
    }

    // Append resources not listed in orderedHandler
    for (auto&& res : resources) {
        auto ch = res->getHandlerType();
        if (std::find(orderedHandler.begin(), orderedHandler.end(), ch) == orderedHandler.end()) {
            orderedResources.push_back(res);
        }
    }
    return orderedResources;
}

std::pair<bool, int> UpnpXMLBuilder::insertTempTranscodingResource(const std::shared_ptr<CdsItem>& item, const std::unique_ptr<Quirks>& quirks, std::deque<std::shared_ptr<CdsResource>>& orderedResources, bool skipURL) const
{
    bool hideOriginalResource = false;
    int originalResource = -1;

    // once proxying is a feature that can be turned off or on in
    // config manager we should use that setting
    //
    // TODO: allow transcoding for URLs
    // now get the profile
    auto tlist = config->getTranscodingProfileListOption(CFG_TRANSCODING_PROFILE_LIST);
    auto filterList = tlist->getFilterList();
    if (!filterList.empty()) {
        auto mainResource = item->getResource(CdsResource::Purpose::Content);
        auto itemMime = item->getMimeType();
        std::string ct = getValueOrDefault(ctMappings, itemMime);
        auto sourceProfile = dlnaProfileString(*mainResource, ct, false);
        for (auto&& filter : filterList) {
            if (!filter)
                throw_std_runtime_error("Invalid profile encountered");
            // check for mimetype and filter if no match
            auto fMime = filter->getMimeType();
            if (!fMime.empty()) {
                std::vector<std::string> parts = splitString(fMime, '/');
                if (parts.size() == 2 && parts[1] == "*") {
                    if (!startswith(itemMime, parts[0] + "/")) {
                        continue;
                    }
                    // check for mime types to skip
                    auto noTranscodingFor = filter->getNoTranscodingMimeTypes();
                    if (std::find(noTranscodingFor.begin(), noTranscodingFor.end(), itemMime) != noTranscodingFor.end()) {
                        continue;
                    }
                } else if (fMime != itemMime) {
                    continue;
                }
            }
            // check for client profile prop and filter if no match
            if (!filter->getSourceProfile().empty() && filter->getSourceProfile() != sourceProfile) {
                continue;
            }
            // check for client flags and filter if no match
            if (quirks && filter->getClientFlags() > 0 && quirks->checkFlags(filter->getClientFlags()) == 0) {
                continue;
            }

            auto tp = filter->getTranscodingProfile();
            // check for transcoding profile
            if (!tp)
                continue;

            // check for transcoding profile
            if (!tp->isEnabled())
                continue;

            // check for client profile prop and filter if no match
            if (quirks && tp->getClientFlags() > 0 && quirks->checkFlags(tp->getClientFlags()) == 0) {
                continue;
            }
            if (ct == CONTENT_TYPE_OGG) {
                if ((item->getFlag(OBJECT_FLAG_OGG_THEORA) && !tp->isTheora()) || (!item->getFlag(OBJECT_FLAG_OGG_THEORA) && tp->isTheora())) {
                    continue;
                }
            } else if (ct == CONTENT_TYPE_AVI) {
                // check user fourcc settings
                AviFourccListmode fccMode = tp->getAVIFourCCListMode();

                const auto& fccList = tp->getAVIFourCCList();
                // mode is either process or ignore, so we will have to take a
                // look at the settings
                if (fccMode != AviFourccListmode::None) {
                    std::string currentFcc = mainResource->getOption(RESOURCE_OPTION_FOURCC);
                    // we can not do much if the item has no fourcc info,
                    // so we will transcode it anyway
                    if (currentFcc.empty()) {
                        // the process mode specifies that we will transcode
                        // ONLY if the fourcc matches the list; since an invalid
                        // fourcc can not match anything we will skip the item
                        if (fccMode == AviFourccListmode::Process)
                            continue;
                    } else {
                        // we have the current and hopefully valid fcc string
                        // let's have a look if it matches the list
                        bool fccMatch = std::find(fccList.begin(), fccList.end(), currentFcc) != fccList.end();
                        if (!fccMatch && (fccMode == AviFourccListmode::Process))
                            continue;

                        if (fccMatch && (fccMode == AviFourccListmode::Ignore))
                            continue;
                    }
                }
            }

            auto tRes = std::make_shared<CdsResource>(ContentHandler::TRANSCODE, CdsResource::Purpose::Transcode);
            tRes->setResId(std::numeric_limits<int>::max());
            tRes->addParameter(URL_PARAM_TRANSCODE_PROFILE_NAME, tp->getName());
            // after transcoding resource was added we can not rely on
            // index 0, so we will make sure the ogg option is there
            tRes->addOption(CONTENT_TYPE_OGG, mainResource->getOption(CONTENT_TYPE_OGG));
            tRes->addParameter(URL_PARAM_TRANSCODE, URL_VALUE_TRANSCODE);

            std::string targetMimeType = tp->getTargetMimeType();

            if (tp->isThumbnail()) {
                tRes->setPurpose(CdsResource::Purpose::Thumbnail);
            } else {
                // duration should be the same for transcoded media, so we can
                // take the value from the original resource
                std::string duration = mainResource->getAttribute(CdsResource::Attribute::DURATION);
                if (!duration.empty())
                    tRes->addAttribute(CdsResource::Attribute::DURATION, duration);

                int freq = tp->getSampleFreq();
                if (freq == SOURCE) {

                    std::string frequency = mainResource->getAttribute(CdsResource::Attribute::SAMPLEFREQUENCY);
                    if (!frequency.empty()) {
                        tRes->addAttribute(CdsResource::Attribute::SAMPLEFREQUENCY, frequency);
                        targetMimeType.append(fmt::format(";rate={}", frequency));
                    }
                } else if (freq != OFF) {
                    tRes->addAttribute(CdsResource::Attribute::SAMPLEFREQUENCY, fmt::to_string(freq));
                    targetMimeType.append(fmt::format(";rate={}", freq));
                }

                int chan = tp->getNumChannels();
                if (chan == SOURCE) {
                    std::string nchannels = mainResource->getAttribute(CdsResource::Attribute::NRAUDIOCHANNELS);
                    if (!nchannels.empty()) {
                        tRes->addAttribute(CdsResource::Attribute::NRAUDIOCHANNELS, nchannels);
                        targetMimeType.append(fmt::format(";channels={}", nchannels));
                    }
                } else if (chan != OFF) {
                    tRes->addAttribute(CdsResource::Attribute::NRAUDIOCHANNELS, fmt::to_string(chan));
                    targetMimeType.append(fmt::format(";channels={}", chan));
                }
            }

            tRes->addAttribute(CdsResource::Attribute::PROTOCOLINFO, renderProtocolInfo(targetMimeType));

            tRes->mergeAttributes(tp->getAttributeOverrides());

            if (!tp->getDlnaProfile().empty())
                tRes->addOption("dlnaProfile", tp->getDlnaProfile());

            if (tp->hideOriginalResource())
                hideOriginalResource = true;

            if (tp->getFirstResource()) {
                orderedResources.push_front(std::move(tRes));
                originalResource = 0;
            } else
                orderedResources.push_back(std::move(tRes));
        }
    }

    return { hideOriginalResource, originalResource };
}

void UpnpXMLBuilder::addResources(const std::shared_ptr<CdsItem>& item, pugi::xml_node& parent, const std::unique_ptr<Quirks>& quirks) const
{
    bool isExternalURL = (item->isExternalItem() && !item->getFlag(OBJECT_FLAG_PROXY_URL));

    auto orderedResources = getOrderedResources(*item);

    // This loop we are looking for resources that contain information to populate item level info
    // Thumbnails are used to populate items AlbumArtUri
    // Subtitles are used to build CaptionInfoEx tags
    // Real resources are rendered lower down
    std::vector<std::map<std::string, std::string>> captionInfoEx;
    auto mimeMappings = quirks ? quirks->getMimeMappings() : std::map<std::string, std::string>();
    for (auto&& res : orderedResources) {
        auto purpose = res->getPurpose();
        if (purpose == CdsResource::Purpose::Content)
            continue;

        auto url = renderResourceURL(*item, *res, mimeMappings);
        // Set Album art if we have a thumbnail
        // Note we dont actually add these as res tags.
        if (purpose == CdsResource::Purpose::Thumbnail) {
            auto aa = parent.append_child(MetadataHandler::getMetaFieldName(M_ALBUMARTURI).data());
            aa.append_child(pugi::node_pcdata).set_value(url.c_str());

            /// \todo clean this up, make sure to check the mimetype and
            /// provide the profile correctly
            aa.append_attribute(UPNP_XML_DLNA_NAMESPACE_ATTR) = UPNP_XML_DLNA_METADATA_NAMESPACE;
            aa.append_attribute("dlna:profileID") = "JPEG_TN";
            continue;
        }

        if (purpose == CdsResource::Purpose::Subtitle) {
            auto captionInfo = std::map<std::string, std::string>();
            captionInfo[""] = url;
            captionInfo["sec:type"] = res->getAttribute(CdsResource::Attribute::TYPE).empty() ? res->getParameter("type") : res->getAttribute(CdsResource::Attribute::TYPE);
            if (!res->getAttribute(CdsResource::Attribute::LANGUAGE).empty()) {
                captionInfo[CdsResource::getAttributeName(CdsResource::Attribute::LANGUAGE)] = res->getAttribute(CdsResource::Attribute::LANGUAGE);
            }
            auto protocolInfo = res->getAttribute(CdsResource::Attribute::PROTOCOLINFO);
            for (auto&& [from, to] : mimeMappings) {
                replaceAllString(protocolInfo, from, to);
            }
            captionInfo[CdsResource::getAttributeName(CdsResource::Attribute::PROTOCOLINFO)] = protocolInfo;

            captionInfoEx.push_back(std::move(captionInfo));
            continue;
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

    auto [hideOriginalResource, originalResource] = insertTempTranscodingResource(item, quirks, orderedResources, isExternalURL);

    for (auto&& res : orderedResources) {
        auto purpose = res->getPurpose();
        if (quirks && !quirks->supportsResource(purpose)) {
            continue;
        }
        std::map<std::string, std::string> clientSpecficAttrs;
        if (res->getHandlerType() == ContentHandler::DEFAULT && !captionInfoEx.empty() && quirks && quirks->checkFlags(QUIRK_FLAG_PV_SUBTITLES)) {
            auto captionInfo = captionInfoEx[0];
            clientSpecficAttrs["pv:subtitleFileType"] = toUpper(captionInfo["sec:type"]);
            clientSpecficAttrs["pv:subtitleFileUri"] = captionInfo[""];
        }

        // bool transcoded = (getValueOrDefault(resParams, URL_PARAM_TRANSCODE) == URL_VALUE_TRANSCODE);
        bool transcoded = purpose == CdsResource::Purpose::Transcode;
        auto clientGroup = quirks ? quirks->getGroup() : DEFAULT_CLIENT_GROUP;

        buildProtocolInfo(*res, mimeMappings);

        if (!hideOriginalResource || transcoded || originalResource != res->getResId())
            renderResource(*item, *res, parent, clientSpecficAttrs, clientGroup, mimeMappings);
    }
}

std::string UpnpXMLBuilder::getMimeType(const CdsResource& resource, const std::map<std::string, std::string>& mimeMappings) const
{
    std::string protocolInfo = resource.getAttribute(CdsResource::Attribute::PROTOCOLINFO);
    std::string mimeType = getMTFromProtocolInfo(protocolInfo);

    auto pos = mimeType.find(';');
    if (pos != std::string::npos) {
        mimeType = mimeType.substr(0, pos);
    }
    for (auto&& [from, to] : mimeMappings) {
        replaceAllString(mimeType, from, to);
    }

    return mimeType;
}

std::string UpnpXMLBuilder::buildProtocolInfo(CdsResource& resource, const std::map<std::string, std::string>& mimeMappings) const
{
    // Why is this here? Just for transcoding maybe?

    auto mimeType = getMimeType(resource, mimeMappings);
    auto contentType = getValueOrDefault(ctMappings, mimeType);
    auto extend = dlnaProfileString(resource, contentType);
    // we do not support seeking at all, so 00
    // and the media is converted, so set CI to 1
    if (resource.getPurpose() == CdsResource::Purpose::Transcode) {
        extend.append(fmt::format("{}={};{}={}", UPNP_DLNA_OP, UPNP_DLNA_OP_SEEK_DISABLED, UPNP_DLNA_CONVERSION_INDICATOR, UPNP_DLNA_CONVERSION));
    } else {
        extend.append(fmt::format("{}={};{}={}", UPNP_DLNA_OP, UPNP_DLNA_OP_SEEK_RANGE, UPNP_DLNA_CONVERSION_INDICATOR, UPNP_DLNA_NO_CONVERSION));
    }
    std::string dlnaFlags;
    if (startswith(mimeType, "audio") || startswith(mimeType, "video"))
        dlnaFlags = UPNP_DLNA_ORG_FLAGS_AV;
    else if (startswith(mimeType, "image"))
        dlnaFlags = UPNP_DLNA_ORG_FLAGS_IMAGE;
    if (resource.getPurpose() == CdsResource::Purpose::Subtitle) {
        dlnaFlags = UPNP_DLNA_ORG_FLAGS_SUB;
    }
    if (!dlnaFlags.empty())
        extend.append(fmt::format(";{}={}", UPNP_DLNA_FLAGS, dlnaFlags));

    auto protocolInfo = resource.getAttribute(CdsResource::Attribute::PROTOCOLINFO);
    for (auto&& [from, to] : mimeMappings) {
        replaceAllString(protocolInfo, from, to);
    }

    protocolInfo = protocolInfo.substr(0, protocolInfo.rfind(':') + 1).append(extend);
    // FIXME: Should we be working protocol info out here?
    resource.addAttribute(CdsResource::Attribute::PROTOCOLINFO, protocolInfo);

    log_debug("protocolInfo: {}", protocolInfo.c_str());

    return protocolInfo;
}
