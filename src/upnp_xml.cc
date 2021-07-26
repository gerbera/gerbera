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

#include "config/config_manager.h"
#include "content/scripting/script_names.h"
#include "database/database.h"
#include "metadata/metadata_handler.h"
#include "request_handler.h"
#include "transcoding/transcoding.h"

std::vector<int> UpnpXMLBuilder::orderedHandler = {};

UpnpXMLBuilder::UpnpXMLBuilder(const std::shared_ptr<Context>& context,
    std::string virtualUrl, std::string presentationURL)
    : config(context->getConfig())
    , database(context->getDatabase())
    , virtualURL(std::move(virtualUrl))
    , presentationURL(std::move(presentationURL))
{
    if (orderedHandler.empty())
        for (auto&& entry : this->config->getArrayOption(CFG_IMPORT_RESOURCES_ORDER)) {
            auto ch = MetadataHandler::remapContentHandler(entry);
            if (ch > -1) {
                orderedHandler.push_back(ch);
            }
        }
}

std::unique_ptr<pugi::xml_document> UpnpXMLBuilder::createResponse(const std::string& actionName, const std::string& serviceType)
{
    auto response = std::make_unique<pugi::xml_document>();
    auto root = response->append_child(fmt::format("u:{}Response", actionName.c_str()).c_str());
    root.append_attribute("xmlns:u") = serviceType.c_str();

    return response;
}

metadata_fields_t UpnpXMLBuilder::remapMetaDataField(const std::string& fieldName)
{
    for (auto&& [f, s] : mt_names) {
        if (s == fieldName) {
            return f;
        }
    }
    return M_MAX;
}

void UpnpXMLBuilder::addPropertyList(pugi::xml_node& result, const std::map<std::string, std::string>& meta, const std::map<std::string, std::string>& auxData,
    const std::map<std::string, std::string>& propertyMap)
{
    for (auto&& [tag, field] : propertyMap) {
        auto metaField = remapMetaDataField(field);
        auto value = (metaField != M_MAX) ? getValueOrDefault(meta, MetadataHandler::getMetaFieldName(metaField)) : getValueOrDefault(auxData, field);
        if (!value.empty()) {
            addField(result, tag, value);
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
    std::size_t i, j;
    if (((i = key.find('@')) != std::string::npos)
        && ((j = key.find('[', i + 1)) != std::string::npos)
        && (key[key.length() - 1] == ']')) {
        std::string attr_name = key.substr(i + 1, j - i - 1);
        std::string attr_value = key.substr(j + 1, key.length() - j - 2);
        std::string name = key.substr(0, i);
        auto node = entry.append_child(name.c_str());
        node.append_attribute(attr_name.c_str()) = attr_value.c_str();
        node.append_child(pugi::node_pcdata).set_value(val.c_str());
    } else {
        entry.append_child(key.c_str()).append_child(pugi::node_pcdata).set_value(val.c_str());
    }
}

void UpnpXMLBuilder::renderObject(const std::shared_ptr<CdsObject>& obj, size_t stringLimit, pugi::xml_node* parent, const std::shared_ptr<Quirks>& quirks)
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

    auto auxData = obj->getAuxData();

    if (obj->isItem()) {
        auto item = std::static_pointer_cast<CdsItem>(obj);

        if (quirks)
            quirks->restoreSamsungBookMarkedPosition(item, &result);

        auto meta = obj->getMetadata();
        std::string upnp_class = obj->getClass();

        for (auto&& [key, val] : meta) {
            if (key == MetadataHandler::getMetaFieldName(M_DESCRIPTION)) {
                tmp = val;
                if ((stringLimit > 0) && (tmp.length() > stringLimit)) {
                    tmp = tmp.substr(0, getValidUTF8CutPosition(tmp, stringLimit - 3));
                    tmp.append("...");
                }
                result.append_child(key.c_str()).append_child(pugi::node_pcdata).set_value(tmp.c_str());
            } else if (key == MetadataHandler::getMetaFieldName(M_TRACKNUMBER)) {
                if (upnp_class == UPNP_CLASS_MUSIC_TRACK) {
                    result.append_child(key.c_str()).append_child(pugi::node_pcdata).set_value(val.c_str());
                }
            } else if (key != MetadataHandler::getMetaFieldName(M_TITLE)) {
                addField(result, key, val);
            }
        }
        const auto titleProperties = config->getDictionaryOption(CFG_UPNP_TITLE_PROPERTIES);
        addPropertyList(result, meta, auxData, titleProperties);
        addResources(item, &result, quirks);

        result.set_name("item");
    } else if (obj->isContainer()) {
        auto cont = std::static_pointer_cast<CdsContainer>(obj);

        result.set_name("container");
        int childCount = cont->getChildCount();
        if (childCount >= 0)
            result.append_attribute("childCount") = childCount;

        std::string upnp_class = obj->getClass();
        log_debug("container is class: {}", upnp_class.c_str());
        auto meta = obj->getMetadata();
        if (upnp_class == UPNP_CLASS_MUSIC_ALBUM) {
            const auto albumProperties = config->getDictionaryOption(CFG_UPNP_ALBUM_PROPERTIES);
            addPropertyList(result, meta, auxData, albumProperties);
        } else if (upnp_class == UPNP_CLASS_MUSIC_ARTIST) {
            const auto artistProperties = config->getDictionaryOption(CFG_UPNP_ARTIST_PROPERTIES);
            addPropertyList(result, meta, auxData, artistProperties);
        } else if (upnp_class == UPNP_CLASS_MUSIC_GENRE) {
            const auto genreProperties = config->getDictionaryOption(CFG_UPNP_GENRE_PROPERTIES);
            addPropertyList(result, meta, auxData, genreProperties);
        } else if (upnp_class == UPNP_CLASS_PLAYLIST_CONTAINER) {
            const auto genreProperties = config->getDictionaryOption(CFG_UPNP_PLAYLIST_PROPERTIES);
            addPropertyList(result, meta, auxData, genreProperties);
        }
        if (upnp_class == UPNP_CLASS_MUSIC_ALBUM || upnp_class == UPNP_CLASS_MUSIC_ARTIST || upnp_class == UPNP_CLASS_CONTAINER || upnp_class == UPNP_CLASS_PLAYLIST_CONTAINER) {
            std::string url;
            bool artAdded = renderContainerImage(virtualURL, cont, url);
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
            const char* SCPDURL;
            const char* controlURL;
            const char* eventSubURL;
        };
        constexpr std::array<ServiceInfo, 3> services { {
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

    for (auto&& [key, val] : attributes) {
        res.append_attribute(key.c_str()) = val.c_str();
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
            return std::make_unique<PathBase>(PathBase { item->getLocation(), false });
        }

        if ((item->getFlag(OBJECT_FLAG_ONLINE_SERVICE) && item->getFlag(OBJECT_FLAG_PROXY_URL)) || forceLocal) {
            auto path = RequestHandler::joinUrl({ CONTENT_ONLINE_HANDLER, dictEncodeSimple(dict), URL_RESOURCE_ID }, true);
            return std::make_unique<PathBase>(PathBase { path, true });
        }
    }
    auto path = RequestHandler::joinUrl({ CONTENT_MEDIA_HANDLER, dictEncodeSimple(dict), URL_RESOURCE_ID }, true);
    return std::make_unique<PathBase>(PathBase { path, true });
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
        return fmt::format(SERVER_VIRTUAL_DIR "{}0", urlBase->pathBase.c_str());
    }

    // a local resource
    return fmt::format(SERVER_VIRTUAL_DIR "{}", urlBase->pathBase.c_str());
}

std::string UpnpXMLBuilder::getArtworkUrl(const std::shared_ptr<CdsItem>& item) const
{
    log_debug("Building Art url for {}", item->getID());

    auto urlBase = getPathBase(item);
    if (urlBase->addResID) {
        return fmt::format("{}{}1/rct/aa", virtualURL, urlBase->pathBase);
    }
    return virtualURL + urlBase->pathBase;
}

bool UpnpXMLBuilder::renderContainerImage(const std::string& virtualURL, const std::shared_ptr<CdsContainer>& cont, std::string& url)
{
    bool artAdded = false;
    for (auto&& res : cont->getResources()) {
        if (res->isMetaResource(ID3_ALBUM_ART)) {
            auto&& resFile = res->getAttribute(R_RESOURCE_FILE);
            auto&& resObj = res->getAttribute(R_FANART_OBJ_ID);
            if (!resFile.empty()) {
                // found, FanArtHandler deals already with file
                std::map<std::string, std::string> dict;
                dict[URL_OBJECT_ID] = fmt::to_string(cont->getID());

                auto res_params = res->getParameters();
                res_params[RESOURCE_HANDLER] = fmt::to_string(res->getHandlerType());
                url.assign(virtualURL + RequestHandler::joinUrl({ CONTENT_MEDIA_HANDLER, dictEncodeSimple(dict), URL_RESOURCE_ID, fmt::to_string(res->getResId()), dictEncodeSimple(res_params) }));

                artAdded = true;
                break;
            }

            if (!resObj.empty()) {
                std::map<std::string, std::string> dict;
                dict[URL_OBJECT_ID] = resObj;

                auto res_params = res->getParameters();
                url.assign(virtualURL + RequestHandler::joinUrl({ CONTENT_MEDIA_HANDLER, dictEncodeSimple(dict), URL_RESOURCE_ID, res->getAttribute(R_FANART_RES_ID), dictEncodeSimple(res_params) }));

                artAdded = true;
                break;
            }
        }
    }
    return artAdded;
}

std::string UpnpXMLBuilder::renderOneResource(const std::string& virtualURL, const std::shared_ptr<CdsItem>& item, const std::shared_ptr<CdsResource>& res)
{
    auto&& res_params = res->getParameters();
    auto urlBase = getPathBase(item);
    std::string url;
    if (urlBase->addResID) {
        url = fmt::format("{}{}{}{}", virtualURL.c_str(), urlBase->pathBase.c_str(), res->getResId(), _URL_PARAM_SEPARATOR);
    } else
        url = virtualURL + urlBase->pathBase;

    if (!res_params.empty()) {
        url.append(dictEncodeSimple(res_params));
    }
    return url;
}

bool UpnpXMLBuilder::renderItemImage(const std::string& virtualURL, const std::shared_ptr<CdsItem>& item, std::string& url)
{
    auto orderedResources = getOrderedResources(item);
    auto resFound = std::find_if(orderedResources.begin(), orderedResources.end(),
        [](auto&& res) { return res->isMetaResource(ID3_ALBUM_ART) //
                             || (res->getHandlerType() == CH_LIBEXIF && res->getParameter(RESOURCE_CONTENT_TYPE) == EXIF_THUMBNAIL) //
                             || (res->getHandlerType() == CH_FFTH && res->getOption(RESOURCE_CONTENT_TYPE) == THUMBNAIL); });
    if (resFound != orderedResources.end()) {
        url = renderOneResource(virtualURL, item, *resFound);
        return true;
    }

    return false;
}

bool UpnpXMLBuilder::renderSubtitle(const std::string& virtualURL, const std::shared_ptr<CdsItem>& item, std::string& url)
{
    auto resources = item->getResources();
    auto resFound = std::find_if(resources.begin(), resources.end(),
        [](auto&& res) { return res->isMetaResource(VIDEO_SUB, CH_SUBTITLE); });
    if (resFound != resources.end()) {
        url = renderOneResource(virtualURL, item, *resFound);
        url.append(renderExtension("", (*resFound)->getAttribute(R_RESOURCE_FILE)));
        return true;
    }
    return false;
}

std::string UpnpXMLBuilder::renderExtension(const std::string& contentType, const fs::path& location)
{
    auto&& ext = RequestHandler::joinUrl({ URL_FILE_EXTENSION, "file" });

    if (!contentType.empty() && (contentType != CONTENT_TYPE_PLAYLIST)) {
        return fmt::format("{}.{}", ext, contentType);
    }

    if (!location.empty() && location.has_extension()) {
        // make sure that the filename does not contain the separator character
        auto&& filename = urlEscape(location.filename().stem().string());
        auto&& extension = location.filename().extension().string();
        return fmt::format("{}.{}{}", ext, (filename), extension);
    }

    return "";
}

std::vector<std::shared_ptr<CdsResource>> UpnpXMLBuilder::getOrderedResources(const std::shared_ptr<CdsItem>& item)
{
    // Order resources according to index defined by orderedHandler
    std::vector<std::shared_ptr<CdsResource>> orderedResources {};
    auto&& resources = item->getResources();
    for (auto&& oh : orderedHandler) {
        std::for_each(resources.begin(), resources.end(), [&](auto res) { if (oh == res->getHandlerType()) orderedResources.push_back(res); });
    }

    // Append resources not listed in orderedHandler
    for (auto&& res : resources) {
        auto ch = res->getHandlerType();
        if (std::none_of(orderedHandler.begin(), orderedHandler.end(), [ch](auto&& entry) { return ch == entry; })) {
            orderedResources.push_back(res);
        }
    }
    return orderedResources;
}

void UpnpXMLBuilder::addResources(const std::shared_ptr<CdsItem>& item, pugi::xml_node* parent, const std::shared_ptr<Quirks>& quirks)
{
    auto urlBase = getPathBase(item);
    bool skipURL = (item->isExternalItem() && !item->getFlag(OBJECT_FLAG_PROXY_URL));

    bool isExtThumbnail = false; // this sucks
    auto mappings = config->getDictionaryOption(CFG_IMPORT_MAPPINGS_MIMETYPE_TO_CONTENTTYPE_LIST);

    // this will be used to count only the "real" resources, omitting the
    // transcoded ones
    bool hide_original_resource = false;
    int original_resource = -1;

    std::unique_ptr<PathBase> urlBase_tr;

    // once proxying is a feature that can be turned off or on in
    // config manager we should use that setting
    //
    // TODO: allow transcoding for URLs
    auto orderedResources = getOrderedResources(item);

    // now get the profile
    auto tlist = config->getTranscodingProfileListOption(CFG_TRANSCODING_PROFILE_LIST);
    auto tp_mt = tlist->get(item->getMimeType());
    if (tp_mt) {
        for (auto&& [key, tp] : *tp_mt) {
            if (!tp)
                throw_std_runtime_error("Invalid profile encountered");
            // check for client profile prop and filter if no match
            if (quirks && tp->getClientFlags() > 0 && quirks->checkFlags(tp->getClientFlags()) == 0)
                continue;
            std::string ct = getValueOrDefault(mappings, item->getMimeType());
            if (ct == CONTENT_TYPE_OGG) {
                if (((item->getFlag(OBJECT_FLAG_OGG_THEORA)) && (!tp->isTheora())) || (!item->getFlag(OBJECT_FLAG_OGG_THEORA) && (tp->isTheora()))) {
                    continue;
                }
            } else if (ct == CONTENT_TYPE_AVI) {
                // check user fourcc settings
                avi_fourcc_listmode_t fcc_mode = tp->getAVIFourCCListMode();

                std::vector<std::string> fcc_list = tp->getAVIFourCCList();
                // mode is either process or ignore, so we will have to take a
                // look at the settings
                if (fcc_mode != FCC_None) {
                    std::string current_fcc = item->getResource(0)->getOption(RESOURCE_OPTION_FOURCC);
                    // we can not do much if the item has no fourcc info,
                    // so we will transcode it anyway
                    if (current_fcc.empty()) {
                        // the process mode specifies that we will transcode
                        // ONLY if the fourcc matches the list; since an invalid
                        // fourcc can not match anything we will skip the item
                        if (fcc_mode == FCC_Process)
                            continue;
                    } else {
                        // we have the current and hopefully valid fcc string
                        // let's have a look if it matches the list
                        bool fcc_match = std::find(fcc_list.begin(), fcc_list.end(), current_fcc) != fcc_list.end();
                        if (!fcc_match && (fcc_mode == FCC_Process))
                            continue;

                        if (fcc_match && (fcc_mode == FCC_Ignore))
                            continue;
                    }
                }
            }

            auto t_res = std::make_shared<CdsResource>(CH_TRANSCODE);
            t_res->setResId(std::numeric_limits<int>::max());
            t_res->addParameter(URL_PARAM_TRANSCODE_PROFILE_NAME, tp->getName());
            // after transcoding resource was added we can not rely on
            // index 0, so we will make sure the ogg option is there
            t_res->addOption(CONTENT_TYPE_OGG, item->getResource(0)->getOption(CONTENT_TYPE_OGG));
            t_res->addParameter(URL_PARAM_TRANSCODE, URL_VALUE_TRANSCODE);

            std::string targetMimeType = tp->getTargetMimeType();

            if (!tp->isThumbnail()) {
                // duration should be the same for transcoded media, so we can
                // take the value from the original resource
                std::string duration = item->getResource(0)->getAttribute(R_DURATION);
                if (!duration.empty())
                    t_res->addAttribute(R_DURATION, duration);

                int freq = tp->getSampleFreq();
                if (freq == SOURCE) {
                    std::string frequency = item->getResource(0)->getAttribute(R_SAMPLEFREQUENCY);
                    if (!frequency.empty()) {
                        t_res->addAttribute(R_SAMPLEFREQUENCY, frequency);
                        targetMimeType.append(fmt::format(";rate={}", frequency));
                    }
                } else if (freq != OFF) {
                    t_res->addAttribute(R_SAMPLEFREQUENCY, fmt::to_string(freq));
                    targetMimeType.append(fmt::format(";rate={}", freq));
                }

                int chan = tp->getNumChannels();
                if (chan == SOURCE) {
                    std::string nchannels = item->getResource(0)->getAttribute(R_NRAUDIOCHANNELS);
                    if (!nchannels.empty()) {
                        t_res->addAttribute(R_NRAUDIOCHANNELS, nchannels);
                        targetMimeType.append(fmt::format(";channels={}", nchannels));
                    }
                } else if (chan != OFF) {
                    t_res->addAttribute(R_NRAUDIOCHANNELS, fmt::to_string(chan));
                    targetMimeType.append(fmt::format(";channels={}", chan));
                }
            }

            t_res->addAttribute(R_PROTOCOLINFO, renderProtocolInfo(targetMimeType));

            if (tp->isThumbnail())
                t_res->addOption(RESOURCE_CONTENT_TYPE, EXIF_THUMBNAIL);

            t_res->mergeAttributes(tp->getAttributes());

            if (tp->hideOriginalResource())
                hide_original_resource = true;

            if (tp->firstResource()) {
                orderedResources.insert(orderedResources.begin(), std::move(t_res));
                original_resource = 0;
            } else
                orderedResources.push_back(std::move(t_res));
        }

        if (skipURL)
            urlBase_tr = getPathBase(item, true);
    }

    bool isFirstSub = true;
    for (auto&& res : orderedResources) {
        /// \todo what if the resource has a different mimetype than the item??
        /*        std::string mimeType = item->getMimeType();
                  if (mimeType.empty()) mimeType = DEFAULT_MIMETYPE; */

        auto res_attrs = res->getAttributes();
        auto res_params = res->getParameters();
        std::string protocolInfo = getValueOrDefault(res_attrs, MetadataHandler::getResAttrName(R_PROTOCOLINFO));
        std::string mimeType = getMTFromProtocolInfo(protocolInfo);

        size_t pos = mimeType.find(';');
        if (pos != std::string::npos) {
            mimeType = mimeType.substr(0, pos);
        }

        assert(!mimeType.empty());
        std::string contentType = getValueOrDefault(mappings, mimeType);
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
        bool transcoded = (getValueOrDefault(res_params, URL_PARAM_TRANSCODE) == URL_VALUE_TRANSCODE);
        if (!transcoded) {
            if (urlBase->addResID) {
                url = fmt::format("{}{}", urlBase->pathBase, res->getResId());
            } else
                url = urlBase->pathBase;
        } else {
            if (!skipURL)
                url = urlBase->pathBase + URL_VALUE_TRANSCODE_NO_RES_ID;
            else {
                assert(urlBase_tr);
                url = urlBase_tr->pathBase + URL_VALUE_TRANSCODE_NO_RES_ID;
            }
        }
        if (!res_params.empty()) {
            url.append(_URL_PARAM_SEPARATOR);
            url.append(dictEncodeSimple(res_params));
        }

        // ok this really sucks, I guess another rewrite of the resource manager
        // is necessary
        int handlerType = res->getHandlerType();
        if ((res->getResId() > 0) && (handlerType == CH_EXTURL) && ((res->getOption(RESOURCE_CONTENT_TYPE) == THUMBNAIL) || (res->getOption(RESOURCE_CONTENT_TYPE) == ID3_ALBUM_ART))) {
            url = res->getOption(RESOURCE_OPTION_URL);
            if (url.empty())
                throw_std_runtime_error("missing thumbnail URL");

            isExtThumbnail = true;
        }

        // resource is used to point to album art
        // only add upnp:AlbumArtURI if we have an AA, skip the resource
        if (res->isMetaResource(ID3_ALBUM_ART) //
            || (res->getHandlerType() == CH_LIBEXIF && res->getParameter(RESOURCE_CONTENT_TYPE) == EXIF_THUMBNAIL) //
            || (res->getHandlerType() == CH_FFTH && res->getOption(RESOURCE_CONTENT_TYPE) == THUMBNAIL) //
        ) {
            auto aa = parent->append_child(MetadataHandler::getMetaFieldName(M_ALBUMARTURI).c_str());
            aa.append_child(pugi::node_pcdata).set_value((virtualURL + url).c_str());

            /// \todo clean this up, make sure to check the mimetype and
            /// provide the profile correctly
            aa.append_attribute(UPNP_XML_DLNA_NAMESPACE_ATTR) = UPNP_XML_DLNA_METADATA_NAMESPACE;
            aa.append_attribute("dlna:profileID") = "JPEG_TN";
            if (res->isMetaResource(ID3_ALBUM_ART)) {
                continue;
            }
        }
        if (isFirstSub && res->isMetaResource(VIDEO_SUB, CH_SUBTITLE)) {
            auto vs = parent->append_child("sec:CaptionInfoEx");
            auto subUrl = url;
            subUrl.append(renderExtension("", res->getAttribute(R_RESOURCE_FILE)));
            vs.append_child(pugi::node_pcdata).set_value((virtualURL + subUrl).c_str());
            vs.append_attribute("sec:type") = res->getAttribute(R_TYPE).c_str();
            vs.append_attribute(MetadataHandler::getResAttrName(R_LANGUAGE).c_str()) = res->getAttribute(R_LANGUAGE).c_str();
            vs.append_attribute(MetadataHandler::getResAttrName(R_PROTOCOLINFO).c_str()) = protocolInfo.c_str();
            isFirstSub = false;
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

        std::string extend;
        if (contentType == CONTENT_TYPE_JPG) {
            std::string resolution = getValueOrDefault(res_attrs, MetadataHandler::getResAttrName(R_RESOLUTION));
            int x;
            int y;
            if ((res->getResId() > 0) && !resolution.empty() && checkResolution(resolution, &x, &y)) {
                if ((((res->getHandlerType() == CH_LIBEXIF) && (res->getParameter(RESOURCE_CONTENT_TYPE) == EXIF_THUMBNAIL)) || (res->getOption(RESOURCE_CONTENT_TYPE) == EXIF_THUMBNAIL) || (res->getOption(RESOURCE_CONTENT_TYPE) == THUMBNAIL)) && (x <= 160) && (y <= 160))
                    extend = fmt::format("{}={};", UPNP_DLNA_PROFILE, UPNP_DLNA_PROFILE_JPEG_TN);
                else if ((x <= 640) && (y <= 420))
                    extend = fmt::format("{}={};", UPNP_DLNA_PROFILE, UPNP_DLNA_PROFILE_JPEG_SM);
                else if ((x <= 1024) && (y <= 768))
                    extend = fmt::format("{}={};", UPNP_DLNA_PROFILE, UPNP_DLNA_PROFILE_JPEG_MED);
                else if ((x <= 4096) && (y <= 4096))
                    extend = fmt::format("{}={};", UPNP_DLNA_PROFILE, UPNP_DLNA_PROFILE_JPEG_LRG);
            }
        } else {
            /* handle audio/video content */
            extend = getDLNAprofileString(config, contentType);
        }

        // we do not support seeking at all, so 00
        // and the media is converted, so set CI to 1
        if (!isExtThumbnail && transcoded) {
            extend.append(fmt::format("{}={};{}={}", UPNP_DLNA_OP, UPNP_DLNA_OP_SEEK_DISABLED, UPNP_DLNA_CONVERSION_INDICATOR, UPNP_DLNA_CONVERSION));

            if (startswith(mimeType, "audio") || startswith(mimeType, "video"))
                extend.append(";" UPNP_DLNA_FLAGS "=" UPNP_DLNA_ORG_FLAGS_AV);
        } else {
            extend.append(fmt::format("{}={};{}={}", UPNP_DLNA_OP, UPNP_DLNA_OP_SEEK_RANGE, UPNP_DLNA_CONVERSION_INDICATOR, UPNP_DLNA_NO_CONVERSION));
        }

        protocolInfo = protocolInfo.substr(0, protocolInfo.rfind(':') + 1).append(extend);
        res_attrs[MetadataHandler::getResAttrName(R_PROTOCOLINFO)] = protocolInfo;

        log_debug("protocolInfo: {}", protocolInfo.c_str());

        // URL is path until now
        if (!item->isExternalItem() || (hide_original_resource && item->isExternalItem())) {
            url.insert(0, virtualURL);
        }

        if (!hide_original_resource || transcoded || (hide_original_resource && (original_resource != res->getResId())))
            renderResource(url, res_attrs, parent);
    }
}
