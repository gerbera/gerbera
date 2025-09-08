/*GRB*

    Gerbera - https://gerbera.io/

    upnp_desc_handler.cc - this file is part of Gerbera.

    Copyright (C) 2024-2025 Gerbera Contributors

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

/// @file request_handler/upnp_desc_handler.cc
#define GRB_LOG_FAC GrbLogFacility::device

#include "upnp_desc_handler.h" // API

#include "config/config.h"
#include "config/config_val.h"
#include "iohandler/file_io_handler.h"
#include "iohandler/mem_io_handler.h"
#include "upnp/clients.h"
#include "upnp/quirks.h"
#include "upnp/upnp_common.h"
#include "upnp/xml_builder.h"
#include "util/grb_time.h"
#include "util/logger.h"
#include "util/tools.h"

#include <sstream>

UpnpDescHandler::UpnpDescHandler(const std::shared_ptr<Content>& content, const std::shared_ptr<UpnpXMLBuilder>& xmlBuilder, const std::shared_ptr<Quirks>& quirks)
    : RequestHandler(content, xmlBuilder, quirks)
    , useDynamicDescription(config->getBoolOption(ConfigVal::UPNP_DYNAMIC_DESCRIPTION))
{
}

fs::path UpnpDescHandler::getPath(const std::shared_ptr<Quirks>& quirks, std::string path)
{
    // This is a hack, we shouldnt need to do this, because SCPDURL is defined as being relative to the description doc
    // Which is served at /upnp/description.xml
    // HOWEVER it seems like its pretty common to just ignore that and request the base URL instead :(
    if (!startswith(path, UPNP_DESC_SCPD_URL)) {
        auto client = quirks ? quirks->getClient() : nullptr;
        auto ip = client && client->addr ? client->addr->getHostName() : "";
        auto userAgent = client ? client->userAgent : "";
        log_warning("Bad client {} (userAgent {}) is not following the SCPDURL spec! (requesting {} not /upnp{}) Remapping it.", ip, userAgent, path, path);
        path = fmt::format("{}{}", UPNP_DESC_SCPD_URL, path);
    }
    auto webroot = config->getOption(ConfigVal::SERVER_WEBROOT);
    auto webFile = fmt::format("{}{}", webroot, path);
    log_debug("Upnp: file: {}", webFile);
    return webFile;
}

bool UpnpDescHandler::getInfo(const char* filename, UpnpFileInfo* info)
{
    auto webFile = getPath(quirks, filename);
    auto svcDescription = (useDynamicDescription && quirks) ? getServiceDescription(webFile, quirks) : "";

    UpnpFileInfo_set_FileLength(info, !svcDescription.empty() ? svcDescription.length() : getFileSize(fs::directory_entry(webFile)));
    UpnpFileInfo_set_ContentType(info, "text/xml");
    UpnpFileInfo_set_IsReadable(info, 1);
    UpnpFileInfo_set_IsDirectory(info, 0);
    UpnpFileInfo_set_LastModified(info, currentTime().count());
    return quirks && quirks->getClient();
}

std::unique_ptr<IOHandler> UpnpDescHandler::open(const char* filename, const std::shared_ptr<Quirks>& quirks, enum UpnpOpenFileMode mode)
{
    auto webFile = getPath(quirks, filename);
    auto svcDescription = (useDynamicDescription && quirks) ? getServiceDescription(webFile, quirks) : "";
    log_debug("Upnp: {}\nsvcDescription: {}", webFile.c_str(), svcDescription);

    std::unique_ptr<IOHandler> ioHandler;
    if (svcDescription.empty())
        ioHandler = std::make_unique<FileIOHandler>(webFile);
    else
        ioHandler = std::make_unique<MemIOHandler>(svcDescription);
    ioHandler->open(mode);
    return ioHandler;
}

std::string UpnpDescHandler::getServiceDescription(const std::string& path, const std::shared_ptr<Quirks>& quirks)
{
    auto doc = std::make_unique<pugi::xml_document>();

    auto parseResult = doc->load_file(path.c_str());
    if (parseResult.status != pugi::xml_parse_status::status_ok)
        return "";

    auto style = doc->prepend_child(pugi::node_pi);
    style.set_name("xml-stylesheet");
    style.set_value("href=\"" UPNP_DESC_STYLESHEET "\" type=\"text/css\"");

    auto decl = doc->prepend_child(pugi::node_declaration);
    decl.append_attribute("version") = "1.0";
    decl.append_attribute("encoding") = "UTF-8";

    auto root = doc->document_element();
    pugi::xpath_node actionListNode = root.select_node("/scpd/actionList");
    if (actionListNode.node()) {
        const static std::map<std::string_view, QuirkFlags> actionFlags {
            // cm.xml
            std::pair("GetCurrentConnectionIDs", QUIRK_FLAG_NONE),
            std::pair("GetCurrentConnectionInfo", QUIRK_FLAG_NONE),
            std::pair("GetProtocolInfo", QUIRK_FLAG_NONE),
            // cds.xml
            std::pair("Browse", QUIRK_FLAG_NONE),
            std::pair("Search", QUIRK_FLAG_NONE),
            std::pair("GetSearchCapabilities", QUIRK_FLAG_NONE),
            std::pair("GetSortCapabilities", QUIRK_FLAG_NONE),
            std::pair("GetSystemUpdateID", QUIRK_FLAG_NONE),
            std::pair("GetFeatureList", QUIRK_FLAG_NONE),
            std::pair("GetSortExtensionCapabilities", QUIRK_FLAG_NONE),
            std::pair("X_GetFeatureList", QUIRK_FLAG_SAMSUNG),
            std::pair("X_GetObjectIDfromIndex", QUIRK_FLAG_SAMSUNG),
            std::pair("X_GetIndexfromRID", QUIRK_FLAG_SAMSUNG),
            std::pair("X_SetBookmark", QUIRK_FLAG_SAMSUNG),
            // mr_reg.xml
            std::pair("IsAuthorized", QUIRK_FLAG_NONE),
            std::pair("RegisterDevice", QUIRK_FLAG_NONE),
            std::pair("IsValidated", QUIRK_FLAG_NONE),
        };
        for (auto&& it : root.select_nodes("/scpd/actionList/action")) {
            pugi::xml_node actionNode = it.node();
            auto nameNode = actionNode.child("name");
            std::string_view name = nameNode.text().as_string();
            log_debug("Checking {} node {}", (bool)quirks, name);
            if (quirks && actionFlags.find(name) != actionFlags.end() && actionFlags.at(name) != QUIRK_FLAG_NONE && !quirks->hasFlag(actionFlags.at(name))) {
                actionListNode.node().remove_child(actionNode);
                log_debug("Removing node {}", name);
            }
        }
    }

    std::ostringstream buf;
    doc->print(buf, "  ", pugi::format_indent);
    auto svcDescription = buf.str();
    log_debug("Upnp: {}\ncontentSize: {}", path, svcDescription.length());
    return svcDescription;
}
