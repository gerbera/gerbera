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

#include <utility>

#include "config/config_manager.h"
#include "database/database.h"
#include "metadata/metadata_handler.h"
#include "server.h"
#include "transcoding/transcoding.h"

UpnpXMLBuilder::UpnpXMLBuilder(std::shared_ptr<Config> config,
    std::shared_ptr<Database> database,
    std::string virtualUrl, std::string presentationURL)
    : config(std::move(config))
    , database(std::move(database))
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

void UpnpXMLBuilder::renderObject(const std::shared_ptr<CdsObject>& obj, size_t stringLimit, pugi::xml_node* parent)
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

    if (obj->isItem()) {
        auto item = std::static_pointer_cast<CdsItem>(obj);

        auto meta = obj->getMetadata();
        std::string upnp_class = obj->getClass();

        for (const auto& [key, val] : meta) {
            if (key == MetadataHandler::getMetaFieldName(M_DESCRIPTION)) {
                tmp = val;
                if ((stringLimit > 0) && (tmp.length() > stringLimit)) {
                    tmp = tmp.substr(0, getValidUTF8CutPosition(tmp, stringLimit - 3));
                    tmp.append("...");
                }
                result.append_child(key.c_str()).append_child(pugi::node_pcdata).set_value(tmp.c_str());
            } else if (key == MetadataHandler::getMetaFieldName(M_TRACKNUMBER)) {
                if (upnp_class == UPNP_CLASS_MUSIC_TRACK)
                    result.append_child(key.c_str()).append_child(pugi::node_pcdata).set_value(val.c_str());
            } else if (key != MetadataHandler::getMetaFieldName(M_TITLE)) {
                // e.g. used for M_ALBUMARTIST
                // name@attr[val] => <name attr="val">
                std::size_t i, j;
                if (((i = key.find('@')) != std::string::npos)
                    && ((j = key.find('[', i + 1)) != std::string::npos)
                    && (key[key.length() - 1] == ']')) {
                    std::string attr_name = key.substr(i + 1, j - i - 1);
                    std::string attr_value = key.substr(j + 1, key.length() - j - 2);
                    std::string name = key.substr(0, i);
                    auto node = result.append_child(name.c_str());
                    node.append_attribute(attr_name.c_str()) = attr_value.c_str();
                    node.append_child(pugi::node_pcdata).set_value(val.c_str());
                } else {
                    result.append_child(key.c_str()).append_child(pugi::node_pcdata).set_value(val.c_str());
                }
            }
        }

        addResources(item, &result);

        result.set_name("item");
    } else if (obj->isContainer()) {
        auto cont = std::static_pointer_cast<CdsContainer>(obj);

        result.set_name("container");
        int childCount = cont->getChildCount();
        if (childCount >= 0)
            result.append_attribute("childCount") = childCount;

        std::string upnp_class = obj->getClass();
        log_debug("container is class: {}", upnp_class.c_str());
        if (upnp_class == UPNP_CLASS_MUSIC_ALBUM) {
            auto meta = obj->getMetadata();

            std::string creator = getValueOrDefault(meta, MetadataHandler::getMetaFieldName(M_ALBUMARTIST));
            if (creator.empty())
                creator = getValueOrDefault(meta, MetadataHandler::getMetaFieldName(M_ARTIST));
            if (!creator.empty())
                result.append_child("dc:creator").append_child(pugi::node_pcdata).set_value(creator.c_str());

            std::string composer = getValueOrDefault(meta, MetadataHandler::getMetaFieldName(M_COMPOSER));
            if (!composer.empty())
                result.append_child("upnp:composer").append_child(pugi::node_pcdata).set_value(composer.c_str());

            std::string conductor = getValueOrDefault(meta, MetadataHandler::getMetaFieldName(M_CONDUCTOR));
            if (!conductor.empty())
                result.append_child("upnp:Conductor").append_child(pugi::node_pcdata).set_value(conductor.c_str());

            std::string orchestra = getValueOrDefault(meta, MetadataHandler::getMetaFieldName(M_ORCHESTRA));
            if (!orchestra.empty())
                result.append_child("upnp:orchestra").append_child(pugi::node_pcdata).set_value(orchestra.c_str());

            std::string date = getValueOrDefault(meta, MetadataHandler::getMetaFieldName(M_UPNP_DATE));
            if (!date.empty())
                result.append_child("upnp:date").append_child(pugi::node_pcdata).set_value(date.c_str());
        }
        if (upnp_class == UPNP_CLASS_MUSIC_ALBUM || upnp_class == UPNP_CLASS_CONTAINER) {
            std::string aa_id = database->findFolderImage(cont->getID(), std::string());

            if (!aa_id.empty()) {
                log_debug("Using folder image as artwork for container");
                std::map<std::string, std::string> dict;
                dict[URL_OBJECT_ID] = aa_id;

                std::string url = virtualURL + _URL_PARAM_SEPARATOR + CONTENT_MEDIA_HANDLER + _URL_PARAM_SEPARATOR + dictEncodeSimple(dict) + _URL_PARAM_SEPARATOR + URL_RESOURCE_ID + _URL_PARAM_SEPARATOR + "0";
                result.append_child("upnp:albumArtURI").append_child(pugi::node_pcdata).set_value(url.c_str());

            } else if (upnp_class == UPNP_CLASS_MUSIC_ALBUM) {
                // try to find the first track and use its artwork
                auto items = database->getObjects(cont->getID(), true);
                if (items != nullptr) {

                    bool artAdded = false;
                    for (const auto& id : *items) {
                        if (artAdded)
                            break;

                        auto obj = database->loadObject(id);
                        if (obj->getClass() != UPNP_CLASS_MUSIC_TRACK)
                            continue;

                        auto item = std::static_pointer_cast<CdsItem>(obj);

                        auto resources = item->getResources();

                        artAdded = std::any_of(resources.begin(), resources.end(),
                            [](const auto& i) { return (i->getHandlerType() == CH_ID3) || (i->getHandlerType() == CH_MP4) || (i->getHandlerType() == CH_FLAC) || (i->getHandlerType() == CH_FANART) || (i->getHandlerType() == CH_EXTURL); });

                        if (artAdded) {
                            std::string url = getArtworkUrl(item);
                            result.append_child("upnp:albumArtURI").append_child(pugi::node_pcdata).set_value(url.c_str());
                        }
                    }
                }
            }
        }
    }
    // log_debug("Rendered DIDL: {}", result->print().c_str());
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

    auto specVersion = root.append_child("specVersion");
    specVersion.append_child("major").append_child(pugi::node_pcdata).set_value(UPNP_DESC_SPEC_VERSION_MAJOR);
    specVersion.append_child("minor").append_child(pugi::node_pcdata).set_value(UPNP_DESC_SPEC_VERSION_MINOR);

    auto device = root.append_child("device");

    auto dlnaDoc = device.append_child("dlna:X_DLNADOC");
    dlnaDoc.append_attribute("xmlns:dlna") = "urn:schemas-dlna-org:device-1-0";
    dlnaDoc.append_child(pugi::node_pcdata).set_value("DMS-1.50");
    // dlnaDoc.append_child(pugi::node_pcdata).set_value("M-DMS-1.50");

    device.append_child("deviceType").append_child(pugi::node_pcdata).set_value(UPNP_DESC_DEVICE_TYPE);
    if (presentationURL.empty())
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

        constexpr std::array<std::pair<const char*, const char*>, 3> iconDims { {
            { "120", "24" },
            { "48", "24" },
            { "32", "8" },
        } };

        constexpr std::array<std::pair<const char*, const char*>, 3> iconTypes { {
            { UPNP_DESC_ICON_PNG_MIMETYPE, ".png" },
            { UPNP_DESC_ICON_BMP_MIMETYPE, ".bmp" },
            { UPNP_DESC_ICON_JPG_MIMETYPE, ".jpg" },
        } };

        for (const auto& [dim, depth] : iconDims) {
            for (const auto& [mimetype, ext] : iconTypes) {
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

    for (const auto& [key, val] : attributes) {
        res.append_attribute(key.c_str()) = val.c_str();
    }
}

void UpnpXMLBuilder::renderCaptionInfo(const std::string& URL, pugi::xml_node* parent) const
{
    auto cap = parent->append_child("sec:CaptionInfoEx");

    // Samsung DLNA clients don't follow this URL and
    // obtain subtitle location from video HTTP headers.
    // However other software players (like VLC) use
    // it to add a subtitle track.

    size_t endp = URL.rfind('.');
    cap.append_child(pugi::node_pcdata).set_value((virtualURL + URL.substr(0, endp) + ".srt").c_str());
    cap.append_attribute("sec:type") = "srt";
}

std::unique_ptr<UpnpXMLBuilder::PathBase> UpnpXMLBuilder::getPathBase(const std::shared_ptr<CdsItem>& item, bool forceLocal)
{
    auto pathBase = std::make_unique<PathBase>();
    /// \todo resource options must be read from configuration files

    std::map<std::string, std::string> dict;
    dict[URL_OBJECT_ID] = std::to_string(item->getID());

    pathBase->addResID = false;
    /// \todo move this down into the "for" loop and create different urls
    /// for each resource once the io handlers are ready    int objectType = ;
    if (item->isExternalItem()) {
        if (!item->getFlag(OBJECT_FLAG_PROXY_URL) && (!forceLocal)) {
            pathBase->pathBase = item->getLocation();
            return pathBase;
        }

        if ((item->getFlag(OBJECT_FLAG_ONLINE_SERVICE) && item->getFlag(OBJECT_FLAG_PROXY_URL)) || forceLocal) {
            pathBase->pathBase = std::string(_URL_PARAM_SEPARATOR) + CONTENT_ONLINE_HANDLER + _URL_PARAM_SEPARATOR + dictEncodeSimple(dict) + _URL_PARAM_SEPARATOR + URL_RESOURCE_ID + _URL_PARAM_SEPARATOR;
            pathBase->addResID = true;
            return pathBase;
        }
    }

    pathBase->pathBase = std::string(_URL_PARAM_SEPARATOR) + CONTENT_MEDIA_HANDLER + _URL_PARAM_SEPARATOR + dictEncodeSimple(dict) + _URL_PARAM_SEPARATOR + URL_RESOURCE_ID + _URL_PARAM_SEPARATOR;
    pathBase->addResID = true;
    return pathBase;
}

std::string UpnpXMLBuilder::getFirstResourcePath(const std::shared_ptr<CdsItem>& item)
{
    std::string result;
    auto urlBase = getPathBase(item);

    if (item->isExternalItem() && !urlBase->addResID) { // a remote resource
        result = urlBase->pathBase;
    } else if (urlBase->addResID) { // a proxy, remote, resource
        result = SERVER_VIRTUAL_DIR + urlBase->pathBase + std::to_string(0);
    } else { // a local resource
        result = SERVER_VIRTUAL_DIR + urlBase->pathBase;
    }
    return result;
}

std::string UpnpXMLBuilder::getArtworkUrl(const std::shared_ptr<CdsItem>& item) const
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

    if (!contentType.empty() && (contentType != CONTENT_TYPE_PLAYLIST)) {
        ext = ext + "." + contentType;
        return ext;
    }

    if (!location.empty()) {
        size_t dot = location.rfind('.');
        if (dot != std::string::npos) {
            std::string extension = location.substr(dot);
            // make sure that the extension does not contain the separator character
            if (extension.find(URL_PARAM_SEPARATOR) == std::string::npos) {
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
    bool skipURL = (item->isExternalItem() && !item->getFlag(OBJECT_FLAG_PROXY_URL));

    bool isExtThumbnail = false; // this sucks
    auto mappings = config->getDictionaryOption(CFG_IMPORT_MAPPINGS_MIMETYPE_TO_CONTENTTYPE_LIST);

#if defined(HAVE_FFMPEG) && defined(HAVE_FFMPEGTHUMBNAILER)
    if (config->getBoolOption(CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_ENABLED) && (startswith(item->getMimeType(), "video") || item->getFlag(OBJECT_FLAG_OGG_THEORA))) {
        std::string videoresolution = item->getResource(0)->getAttribute(R_RESOLUTION);
        int x;
        int y;

        if (!videoresolution.empty() && checkResolution(videoresolution, &x, &y)) {
            auto it = mappings.find(CONTENT_TYPE_JPG);
            std::string thumb_mimetype = it != mappings.end() && !it->second.empty() ? it->second : "image/jpeg";

            auto ffres = std::make_shared<CdsResource>(CH_FFTH);
            ffres->addParameter(RESOURCE_HANDLER, std::to_string(CH_FFTH));
            ffres->addAttribute(R_PROTOCOLINFO, renderProtocolInfo(thumb_mimetype));
            ffres->addOption(RESOURCE_CONTENT_TYPE, THUMBNAIL);

            y = config->getIntOption(CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_THUMBSIZE) * y / x;
            x = config->getIntOption(CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_THUMBSIZE);
            std::string resolution = std::to_string(x) + "x" + std::to_string(y);
            ffres->addAttribute(R_RESOLUTION, resolution);
            item->addResource(ffres);
            log_debug("Adding resource for video thumbnail");
        }
    }
#endif // FFMPEGTHUMBNAILER

    // this will be used to count only the "real" resources, omitting the
    // transcoded ones
    int realCount = 0;
    bool hide_original_resource = false;
    size_t original_resource = 0;

    std::unique_ptr<PathBase> urlBase_tr;

    // once proxying is a feature that can be turned off or on in
    // config manager we should use that setting
    //
    // TODO: allow transcoding for URLs

    // now get the profile
    auto tlist = config->getTranscodingProfileListOption(CFG_TRANSCODING_PROFILE_LIST);
    auto tp_mt = tlist->get(item->getMimeType());
    if (tp_mt != nullptr) {
        for (const auto& [key, tp] : *tp_mt) {
            if (tp == nullptr)
                throw_std_runtime_error("Invalid profile encountered");

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
                    if (current_fcc.empty()) {
                        // the process mode specifies that we will transcode
                        // ONLY if the fourcc matches the list; since an invalid
                        // fourcc can not match anything we will skip the item
                        if (fcc_mode == FCC_Process)
                            continue;
                    }
                    // we have the current and hopefully valid fcc string
                    // let's have a look if it matches the list
                    else {
                        bool fcc_match = std::any_of(fcc_list.begin(), fcc_list.end(), [&](const auto& f) { return current_fcc == f; });

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
                std::string duration = item->getResource(0)->getAttribute(R_DURATION);
                if (!duration.empty())
                    t_res->addAttribute(R_DURATION, duration);

                int freq = tp->getSampleFreq();
                if (freq == SOURCE) {
                    std::string frequency = item->getResource(0)->getAttribute(R_SAMPLEFREQUENCY);
                    if (!frequency.empty()) {
                        t_res->addAttribute(R_SAMPLEFREQUENCY, frequency);
                        targetMimeType.append(";rate=").append(frequency);
                    }
                } else if (freq != OFF) {
                    t_res->addAttribute(R_SAMPLEFREQUENCY, std::to_string(freq));
                    targetMimeType.append(";rate=").append(std::to_string(freq));
                }

                int chan = tp->getNumChannels();
                if (chan == SOURCE) {
                    std::string nchannels = item->getResource(0)->getAttribute(R_NRAUDIOCHANNELS);
                    if (!nchannels.empty()) {
                        t_res->addAttribute(R_NRAUDIOCHANNELS, nchannels);
                        targetMimeType.append(";channels=").append(nchannels);
                    }
                } else if (chan != OFF) {
                    t_res->addAttribute(R_NRAUDIOCHANNELS, std::to_string(chan));
                    targetMimeType.append(";channels=").append(std::to_string(chan));
                }
            }

            t_res->addAttribute(R_PROTOCOLINFO, renderProtocolInfo(targetMimeType));

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

    size_t resCount = item->getResourceCount();
    for (size_t i = 0; i < resCount; i++) {

        /// \todo what if the resource has a different mimetype than the item??
        /*        std::string mimeType = item->getMimeType();
                  if (mimeType.empty()) mimeType = DEFAULT_MIMETYPE; */

        auto res = item->getResource(i);
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
            url.append(dictEncodeSimple(res_params));
        }

        // ok this really sucks, I guess another rewrite of the resource manager
        // is necessary
        int handlerType = res->getHandlerType();
        if ((i > 0) && (handlerType == CH_EXTURL) && ((res->getOption(RESOURCE_CONTENT_TYPE) == THUMBNAIL) || (res->getOption(RESOURCE_CONTENT_TYPE) == ID3_ALBUM_ART))) {
            url = res->getOption(RESOURCE_OPTION_URL);
            if (url.empty())
                throw_std_runtime_error("missing thumbnail URL");

            isExtThumbnail = true;
        }

        /// FIXME: currently resource is misused for album art

        // only add upnp:AlbumArtURI if we have an AA, skip the resource
        if (i > 0) {
            if (handlerType == CH_ID3 || (handlerType == CH_MP4) || handlerType == CH_FLAC || handlerType == CH_FANART || handlerType == CH_EXTURL || handlerType == CH_SUBTITLE) {

                std::string rct;
                if (res->getHandlerType() == CH_EXTURL)
                    rct = res->getOption(RESOURCE_CONTENT_TYPE);
                else
                    rct = res->getParameter(RESOURCE_CONTENT_TYPE);

                if (rct == ID3_ALBUM_ART) {
                    auto aa = parent->append_child(MetadataHandler::getMetaFieldName(M_ALBUMARTURI).c_str());
                    aa.append_child(pugi::node_pcdata).set_value((virtualURL + url).c_str());

                    /// \todo clean this up, make sure to check the mimetype and
                    /// provide the profile correctly
                    aa.append_attribute("xmlns:dlna") = "urn:schemas-dlna-org:metadata-1-0";
                    aa.append_attribute("dlna:profileID") = "JPEG_TN";
                    continue;
                }

                if (rct == VIDEO_SUB) {
                    auto vs = parent->append_child("sec:CaptionInfoEx");
                    vs.append_child(pugi::node_pcdata).set_value((virtualURL + url).c_str());
                    vs.append_attribute("sec:type") = res->getAttribute(R_TYPE).c_str();
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

        std::string extend;
        if (contentType == CONTENT_TYPE_JPG) {
            std::string resolution = getValueOrDefault(res_attrs, MetadataHandler::getResAttrName(R_RESOLUTION));
            int x;
            int y;
            if (!resolution.empty() && checkResolution(resolution, &x, &y)) {

                if ((i > 0) && (((item->getResource(i)->getHandlerType() == CH_LIBEXIF) && (item->getResource(i)->getParameter(RESOURCE_CONTENT_TYPE) == EXIF_THUMBNAIL)) || (item->getResource(i)->getOption(RESOURCE_CONTENT_TYPE) == EXIF_THUMBNAIL) || (item->getResource(i)->getOption(RESOURCE_CONTENT_TYPE) == THUMBNAIL)) && (x <= 160) && (y <= 160))
                    extend = std::string(UPNP_DLNA_PROFILE) + "=" + UPNP_DLNA_PROFILE_JPEG_TN + ";";
                else if ((x <= 640) && (y <= 420))
                    extend = std::string(UPNP_DLNA_PROFILE) + "=" + UPNP_DLNA_PROFILE_JPEG_SM + ";";
                else if ((x <= 1024) && (y <= 768))
                    extend = std::string(UPNP_DLNA_PROFILE) + "=" + UPNP_DLNA_PROFILE_JPEG_MED + ";";
                else if ((x <= 4096) && (y <= 4096))
                    extend = std::string(UPNP_DLNA_PROFILE) + "=" + UPNP_DLNA_PROFILE_JPEG_LRG + ";";
            }
        } else {
            /* handle audio/video content */
            extend = getDLNAprofileString(contentType);
            if (!extend.empty())
                extend.append(";");
        }

        // we do not support seeking at all, so 00
        // and the media is converted, so set CI to 1
        if (!isExtThumbnail && transcoded) {
            extend.append(UPNP_DLNA_OP).append("=").append(UPNP_DLNA_OP_SEEK_DISABLED).append(";").append(UPNP_DLNA_CONVERSION_INDICATOR).append("=" UPNP_DLNA_CONVERSION);

            if (startswith(mimeType, "audio") || startswith(mimeType, "video"))
                extend.append(";" UPNP_DLNA_FLAGS "=" UPNP_DLNA_ORG_FLAGS_AV);
        } else {
            extend.append(UPNP_DLNA_OP).append("=").append(UPNP_DLNA_OP_SEEK_RANGE).append(";");
            extend.append(UPNP_DLNA_CONVERSION_INDICATOR).append("=").append(UPNP_DLNA_NO_CONVERSION);
        }

        protocolInfo = protocolInfo.substr(0, protocolInfo.rfind(':') + 1).append(extend);
        res_attrs[MetadataHandler::getResAttrName(R_PROTOCOLINFO)] = protocolInfo;

        log_debug("protocolInfo: {}", protocolInfo.c_str());

        // URL is path until now
        if (!item->isExternalItem() || (hide_original_resource && item->isExternalItem())) {
            url.insert(0, virtualURL);
        }

        if (!hide_original_resource || transcoded || (hide_original_resource && (original_resource != i)))
            renderResource(url, res_attrs, parent);
    }
}
