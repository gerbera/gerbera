/*GRB*

    Gerbera - https://gerbera.io/

    upnp_desc_handler.cc - this file is part of Gerbera.

    Copyright (C) 2024 Gerbera Contributors

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

/// \file upnp_desc_handler.cc
#define LOG_FAC log_facility_t::requests

#include "upnp_desc_handler.h" // API

#include <sstream>

#include "iohandler/file_io_handler.h"
#include "iohandler/mem_io_handler.h"
#include "upnp/quirks.h"
#include "upnp/xml_builder.h"
#include "util/mime.h"
#include "util/tools.h"

UpnpDescHandler::UpnpDescHandler(const std::shared_ptr<ContentManager>& content, const std::shared_ptr<UpnpXMLBuilder>& xmlBuilder)
    : RequestHandler(content, xmlBuilder)
    , useDynamicDescription(config->getBoolOption(CFG_UPNP_DYNAMIC_DESCRIPTION))
{
}

const struct ClientInfo* UpnpDescHandler::getInfo(const char* filename, UpnpFileInfo* info)
{
    UpnpFileInfo_set_FileLength(info, -1);
    UpnpFileInfo_set_ContentType(info, "application/xml");
    UpnpFileInfo_set_IsReadable(info, 1);
    UpnpFileInfo_set_IsDirectory(info, 0);
    auto quirks = getQuirks(info);
    return quirks ? quirks->getInfo() : nullptr;
}

std::unique_ptr<IOHandler> UpnpDescHandler::open(const char* filename, const std::shared_ptr<Quirks>& quirks, enum UpnpOpenFileMode mode)
{
    std::string path = filename;
    // This is a hack, we shouldnt need to do this, because SCPDURL is defined as being relative to the description doc
    // Which is served at /upnp/description.xml
    // HOWEVER it seems like its pretty common to just ignore that and request the base URL instead :(
    // Ideally we would print client info here too TODO!
    if (!startswith(path, "/upnp/")) {
        log_warning("Bad client is not following the SCPDURL spec! (requesting {} not /upnp{}) Remapping it.", path, path);
        path = fmt::format("/upnp{}", path);
    }
    auto webroot = config->getOption(CFG_SERVER_WEBROOT);
    auto webFile = fmt::format("{}{}", webroot, path);
    log_debug("Upnp: file: {}", webFile);
    auto svcDescription = (useDynamicDescription && quirks) ? getServiceDescription(webFile, quirks) : "";

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
    doc->print(buf, "", 0);
    auto svcDescription = buf.str();
    log_debug("Upnp: {}\ncontentSize: {}", path, svcDescription.length());
    return svcDescription;
}
