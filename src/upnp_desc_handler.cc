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
{
}

void UpnpDescHandler::getInfo(const char* filename, UpnpFileInfo* info)
{
    UpnpFileInfo_set_FileLength(info, -1);
    UpnpFileInfo_set_ContentType(info, "application/xml");
    UpnpFileInfo_set_IsReadable(info, 1);
    UpnpFileInfo_set_IsDirectory(info, 0);
}

std::unique_ptr<IOHandler> UpnpDescHandler::open(const char* filename, enum UpnpOpenFileMode mode)
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
    log_debug("UI: file: {}", webFile);

    auto ioHandler = std::make_unique<FileIOHandler>(webFile);
    ioHandler->open(mode);
    return ioHandler;
}
