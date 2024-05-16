/*GRB*

    Gerbera - https://gerbera.io/

    device_description_handler.cc - this file is part of Gerbera.

    Copyright (C) 2020-2024 Gerbera Contributors

    Gerbera is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.

    Gerbera is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Gerbera.  If not, see <http://www.gnu.org/licenses/>.
*/

/// \file device_description_handler.cc
#define LOG_FAC log_facility_t::requests

#include "device_description_handler.h" // API

#include "cds/cds_container.h"
#include "cds/cds_item.h"
#include "config/config_definition.h"
#include "config/config_manager.h"
#include "config/config_option_enum.h"
#include "config/result/transcoding.h"
#include "database/database.h"
#include "iohandler/mem_io_handler.h"
#include "request_handler.h"
#include "upnp/clients.h"
#include "upnp/quirks.h"
#include "upnp/xml_builder.h"
#include "util/grb_net.h"
#include "util/url_utils.h"

#include <array>
#include <fmt/chrono.h>
#include <sstream>

DeviceDescriptionHandler::DeviceDescriptionHandler(const std::shared_ptr<ContentManager>& content, const std::shared_ptr<UpnpXMLBuilder>& xmlBuilder, std::string ip, in_port_t port)
    : RequestHandler(content, xmlBuilder)
    , ip(ip)
    , port(port)
    , useDynamicDescription(config->getBoolOption(CFG_UPNP_DYNAMIC_DESCRIPTION))
{
    deviceDescription = renderDeviceDescription(ip, port, nullptr);
}

const struct ClientInfo* DeviceDescriptionHandler::getInfo(const char* filename, UpnpFileInfo* info)
{
    log_info("Device description requested {}", filename);
    auto quirks = info ? getQuirks(info) : nullptr;

    if (quirks && useDynamicDescription) {
        deviceDescription = renderDeviceDescription(ip, port, quirks);
    }
    log_debug("hasQuirks: {}, size {}", (bool)quirks, deviceDescription.size());
    // We should be able to do the generation here, but libupnp doesn't support the request cookies yet
    UpnpFileInfo_set_FileLength(info, deviceDescription.length());
    UpnpFileInfo_set_ContentType(info, "application/xml");
    UpnpFileInfo_set_IsReadable(info, 1);
    UpnpFileInfo_set_IsDirectory(info, 0);
    return quirks ? quirks->getInfo() : nullptr;
}

std::unique_ptr<IOHandler> DeviceDescriptionHandler::open(const char* filename, const std::shared_ptr<Quirks>& quirks, enum UpnpOpenFileMode mode)
{
    log_debug("Device description opened {}", filename);

    if (quirks && useDynamicDescription) {
        deviceDescription = renderDeviceDescription(ip, port, quirks);
    }
    log_debug("hasQuirks: {}, size {}", (bool)quirks, deviceDescription.size());

    auto ioHandler = std::make_unique<MemIOHandler>(deviceDescription);
    ioHandler->open(mode);
    return ioHandler;
}

std::string DeviceDescriptionHandler::getPresentationUrl(std::string ip, in_port_t port) const
{
    std::string presentationURL = config->getOption(CFG_SERVER_PRESENTATION_URL);
    if (presentationURL.empty()) {
        presentationURL = fmt::format("http://{}/", GrbNet::renderWebUri(ip, port));
    } else {
        auto appendto = EnumOption<UrlAppendMode>::getEnumOption(config, CFG_SERVER_APPEND_PRESENTATION_URL_TO);
        if (appendto == UrlAppendMode::ip) {
            presentationURL = fmt::format("{}/{}", GrbNet::renderWebUri(ip, 0), presentationURL);
        } else if (appendto == UrlAppendMode::port) {
            presentationURL = fmt::format("{}/{}", GrbNet::renderWebUri(ip, port), presentationURL);
        } // else appendto is none and we take the URL as it entered by user
    }
    if (!startswith(presentationURL, "http")) { // url does not start with http
        presentationURL = fmt::format("http://{}", presentationURL);
    }
    return presentationURL;
}

std::string DeviceDescriptionHandler::renderDeviceDescription(std::string ip, in_port_t port, const std::shared_ptr<Quirks>& quirks) const
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
    if (quirks && quirks->hasFlag(QUIRK_FLAG_SAMSUNG))
        dlnaDoc.append_child(pugi::node_pcdata).set_value("M-DMS-1.50");

    constexpr std::array deviceConfigProperties {
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
    for (auto&& [tag, field] : deviceConfigProperties) {
        device.append_child(tag).append_child(pugi::node_pcdata).set_value(config->getOption(field).c_str());
    }
    {
        const static struct ServiceCapabilities {
            std::string_view serviceType;
            std::string serviceId;
            QuirkFlags quirkFlags;
        } deviceStringProperties[] = {
            { "deviceType", UPNP_DESC_DEVICE_TYPE, QUIRK_FLAG_NONE },
            { "presentationURL", getPresentationUrl(ip, port), QUIRK_FLAG_NONE },
            { "sec:ProductCap", UPNP_DESC_PRODUCT_CAPS, QUIRK_FLAG_NONE },
            { "sec:X_ProductCap", UPNP_DESC_PRODUCT_CAPS, QUIRK_FLAG_SAMSUNG },
        };
        for (auto&& [tag, value, quirkFlags] : deviceStringProperties) {
            if (!quirks || quirkFlags == QUIRK_FLAG_NONE || quirks->hasFlag(quirkFlags))
                device.append_child(tag.data()).append_child(pugi::node_pcdata).set_value(value.c_str());
        }
    }

    // add icons
    {
        auto iconList = device.append_child("iconList");

        constexpr std::array iconDims {
            std::pair("120", "24"),
            std::pair("48", "24"),
            std::pair("32", "8"),
        };

        constexpr std::array iconTypes {
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

        constexpr struct ServiceInfo {
            const char* serviceType;
            const char* serviceId;
            const char* scpdurl;
            const char* controlURL;
            const char* eventSubURL;
        } services[] = {
            // cm
            { UPNP_DESC_CM_SERVICE_TYPE, UPNP_DESC_CM_SERVICE_ID, UPNP_DESC_CM_SCPD_URL, UPNP_DESC_CM_CONTROL_URL, UPNP_DESC_CM_EVENT_URL },
            // cds
            { UPNP_DESC_CDS_SERVICE_TYPE, UPNP_DESC_CDS_SERVICE_ID, UPNP_DESC_CDS_SCPD_URL, UPNP_DESC_CDS_CONTROL_URL, UPNP_DESC_CDS_EVENT_URL },
            // media receiver registrar service for the Xbox 360
            { UPNP_DESC_MRREG_SERVICE_TYPE, UPNP_DESC_MRREG_SERVICE_ID, UPNP_DESC_MRREG_SCPD_URL, UPNP_DESC_MRREG_CONTROL_URL, UPNP_DESC_MRREG_EVENT_URL },
        };

        for (auto&& s : services) {
            auto service = serviceList.append_child("service");
            service.append_child("serviceType").append_child(pugi::node_pcdata).set_value(s.serviceType);
            service.append_child("serviceId").append_child(pugi::node_pcdata).set_value(s.serviceId);
            service.append_child("SCPDURL").append_child(pugi::node_pcdata).set_value(s.scpdurl);
            service.append_child("controlURL").append_child(pugi::node_pcdata).set_value(s.controlURL);
            service.append_child("eventSubURL").append_child(pugi::node_pcdata).set_value(s.eventSubURL);
        }
    }

    std::ostringstream buf;
    doc->print(buf, "", 0);
    return buf.str();
}
