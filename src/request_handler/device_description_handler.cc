/*GRB*

    Gerbera - https://gerbera.io/

    device_description_handler.cc - this file is part of Gerbera.

    Copyright (C) 2020-2025 Gerbera Contributors

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
#define GRB_LOG_FAC GrbLogFacility::device

#include "device_description_handler.h" // API

#include "config/config.h"
#include "config/config_option_enum.h"
#include "config/config_val.h"
#include "iohandler/mem_io_handler.h"
#include "upnp/clients.h"
#include "upnp/quirks.h"
#include "upnp/upnp_common.h"
#include "upnp/xml_builder.h"
#include "util/grb_net.h"
#include "util/logger.h"
#include "util/tools.h"

#include <array>
#include <sstream>
#include <utility>

DeviceDescriptionHandler::DeviceDescriptionHandler(const std::shared_ptr<Content>& content,
    const std::shared_ptr<UpnpXMLBuilder>& xmlBuilder, const std::shared_ptr<Quirks>& quirks,
    const std::string& ip, in_port_t port)
    : RequestHandler(content, xmlBuilder, quirks)
    , ip(ip)
    , port(port)
    , useDynamicDescription(config->getBoolOption(ConfigVal::UPNP_DYNAMIC_DESCRIPTION))
{
    deviceDescription = renderDeviceDescription(ip, port, nullptr);
}

bool DeviceDescriptionHandler::getInfo(const char* filename, UpnpFileInfo* info)
{
    log_debug("Device description requested {}", filename);

    if (quirks && useDynamicDescription) {
        deviceDescription = renderDeviceDescription(ip, port, quirks);
    }
    log_debug("hasQuirks: {}, size {}", (bool)quirks, deviceDescription.size());

    UpnpFileInfo_set_FileLength(info, deviceDescription.length());
    UpnpFileInfo_set_ContentType(info, "text/xml");
    UpnpFileInfo_set_IsReadable(info, 1);
    UpnpFileInfo_set_IsDirectory(info, 0);
    UpnpFileInfo_set_LastModified(info, currentTime().count());
    return quirks && quirks->getClient();
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

std::string DeviceDescriptionHandler::getPresentationUrl(const std::string& ip, in_port_t port) const
{
    std::string presentationURL = config->getOption(ConfigVal::SERVER_PRESENTATION_URL);
    if (presentationURL.empty()) {
        presentationURL = fmt::format("http://{}/", GrbNet::renderWebUri(ip, port));
    } else {
        auto appendto = EnumOption<UrlAppendMode>::getEnumOption(config, ConfigVal::SERVER_APPEND_PRESENTATION_URL_TO);
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

struct ServiceCapabilityValues {
    std::string value;
    QuirkFlags quirkFlags;
};

struct ServiceCapabilities {
    std::string_view serviceType;
    QuirkFlags quirkFlags;
    std::vector<ServiceCapabilityValues> serviceValues;
};

std::string DeviceDescriptionHandler::renderDeviceDescription(const std::string& ip, in_port_t port, const std::shared_ptr<Quirks>& quirks) const
{
    auto doc = std::make_unique<pugi::xml_document>();

    auto style = doc->prepend_child(pugi::node_pi);
    style.set_name("xml-stylesheet");
    style.set_value("href=\"" UPNP_DESC_STYLESHEET "\" type=\"text/css\"");

    auto decl = doc->prepend_child(pugi::node_declaration);
    decl.append_attribute("version") = "1.0";
    decl.append_attribute("encoding") = "UTF-8";

    auto root = doc->append_child("root");
    root.append_attribute("xmlns") = "urn:schemas-upnp-org:device-1-0";
    root.append_attribute(UPNP_XML_SEC_NAMESPACE_ATTR) = UPNP_XML_SEC_NAMESPACE;
    root.append_attribute(UPNP_XML_DLNA_NAMESPACE_ATTR) = UPNP_XML_DLNA_NAMESPACE;

    auto specVersion = root.append_child("specVersion");
    specVersion.append_child("major").append_child(pugi::node_pcdata).set_value("1");
    specVersion.append_child("minor").append_child(pugi::node_pcdata).set_value("0");

    auto device = root.append_child("device");

    // add configuration values
    {
        constexpr std::array deviceConfigProperties {
            std::pair("friendlyName", ConfigVal::SERVER_NAME),
            std::pair("manufacturer", ConfigVal::SERVER_MANUFACTURER),
            std::pair("manufacturerURL", ConfigVal::SERVER_MANUFACTURER_URL),
            std::pair("modelDescription", ConfigVal::SERVER_MODEL_DESCRIPTION),
            std::pair("modelName", ConfigVal::SERVER_MODEL_NAME),
            std::pair("modelNumber", ConfigVal::SERVER_MODEL_NUMBER),
            std::pair("modelURL", ConfigVal::SERVER_MODEL_URL),
            std::pair("serialNumber", ConfigVal::SERVER_SERIAL_NUMBER),
            std::pair("UDN", ConfigVal::SERVER_UDN),
        };
        for (auto&& [tag, field] : deviceConfigProperties) {
            device.append_child(tag).append_child(pugi::node_pcdata).set_value(config->getOption(field).c_str());
        }
    }
    // add service details
    {
        const ServiceCapabilities deviceStringProperties[] = {
            { "dlna:X_DLNACAP", QUIRK_FLAG_NONE, {} },
            { "dlna:X_DLNADOC", QUIRK_FLAG_NONE, { { "DMS-1.50", QUIRK_FLAG_NONE } } },
            { "dlna:X_DLNADOC", QUIRK_FLAG_SAMSUNG, { { "M-DMS-1.50", QUIRK_FLAG_NONE } } },
            { "deviceType", QUIRK_FLAG_NONE, { { "urn:schemas-upnp-org:device:MediaServer:1", QUIRK_FLAG_NONE } } },
            { "presentationURL", QUIRK_FLAG_NONE, { { getPresentationUrl(ip, port), QUIRK_FLAG_NONE } } },
            { "sec:ProductCap", QUIRK_FLAG_NONE, {
                                                     { "smi", QUIRK_FLAG_NONE },
                                                     { "DCM10", QUIRK_FLAG_SAMSUNG },
                                                     { "getMediaInfo.sec", QUIRK_FLAG_NONE },
                                                     { "getCaptionInfo.sec", QUIRK_FLAG_NONE },
                                                 } },
            { "sec:X_ProductCap", QUIRK_FLAG_SAMSUNG, {
                                                          { "smi", QUIRK_FLAG_NONE },
                                                          { "DCM10", QUIRK_FLAG_NONE },
                                                          { "getMediaInfo.sec", QUIRK_FLAG_NONE },
                                                          { "getCaptionInfo.sec", QUIRK_FLAG_NONE },
                                                      } },
        };
        for (auto&& [tag, serviceFlags, values] : deviceStringProperties) {
            if (!quirks || serviceFlags == QUIRK_FLAG_NONE || quirks->hasFlag(serviceFlags)) {
                std::vector<std::string> serviceValue;
                for (auto&& [value, valueFlags] : values) {
                    if (!quirks || valueFlags == QUIRK_FLAG_NONE || quirks->hasFlag(valueFlags)) {
                        serviceValue.push_back(value);
                    }
                }
                auto serviceEntry = device.append_child(tag.data());
                if (!serviceValue.empty())
                    serviceEntry.append_child(pugi::node_pcdata).set_value(fmt::format("{}", fmt::join(serviceValue, ",")).c_str());
            }
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
            { UPNP_DESC_CM_SERVICE_TYPE, UPNP_DESC_CM_SERVICE_ID, UPNP_DESC_SCPD_URL UPNP_DESC_CM_SCPD_URL, UPNP_DESC_CM_CONTROL_URL, UPNP_DESC_CM_EVENT_URL },
            // cds
            { UPNP_DESC_CDS_SERVICE_TYPE, UPNP_DESC_CDS_SERVICE_ID, UPNP_DESC_SCPD_URL UPNP_DESC_CDS_SCPD_URL, UPNP_DESC_CDS_CONTROL_URL, UPNP_DESC_CDS_EVENT_URL },
            // media receiver registrar service for the Xbox 360
            { UPNP_DESC_MRREG_SERVICE_TYPE, UPNP_DESC_MRREG_SERVICE_ID, UPNP_DESC_SCPD_URL UPNP_DESC_MRREG_SCPD_URL, UPNP_DESC_MRREG_CONTROL_URL, UPNP_DESC_MRREG_EVENT_URL },
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
    doc->print(buf, "  ", pugi::format_indent);
    return buf.str();
}
