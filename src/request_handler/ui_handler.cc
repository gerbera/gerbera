/*GRB*

    Gerbera - https://gerbera.io/

    ui_handler.cc - this file is part of Gerbera.

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

/// \file ui_handler.cc
#define GRB_LOG_FAC GrbLogFacility::requests

#include "ui_handler.h" // API

#include "config/config.h"
#include "config/config_val.h"
#include "context.h"
#include "iohandler/file_io_handler.h"
#include "server.h"
#include "upnp/headers.h"
#include "upnp/quirks.h"
#include "util/grb_time.h"
#include "util/logger.h"
#include "util/mime.h"
#include "util/tools.h"

#include <sstream>

UIHandler::UIHandler(const std::shared_ptr<Content>& content,
    const std::shared_ptr<UpnpXMLBuilder>& xmlBuilder, const std::shared_ptr<Quirks>& quirks,
    std::shared_ptr<Server> server)
    : RequestHandler(content, xmlBuilder, quirks)
    , webRoot(config->getOption(ConfigVal::SERVER_WEBROOT))
    , uiEnabled(config->getBoolOption(ConfigVal::SERVER_UI_ENABLED))
    , server(std::move(server))
{
}

std::string getMime(const std::shared_ptr<Mime>& mime, std::string_view path)
{
    if (endswith(path, ".html")) {
        return "text/html";
    }
    if (endswith(path, ".js")) {
        return "application/javascript";
    }
    if (endswith(path, ".json")) {
        return "application/json";
    }
    if (endswith(path, ".css")) {
        return "text/css";
    }

    auto [err, mimeType] = mime->getMimeType(path, "application/octet-stream");
    if (!err) {
        return mimeType;
    }
    return "application/octet-stream";
}

bool UIHandler::getInfo(const char* filename, UpnpFileInfo* info)
{
    std::string path = filename;
    if (path == "/") {
        path = "/index.html";
    }

    auto headers = Headers();
    headers.addHeader("Content-Security-Policy", fmt::format("default-src {} 'unsafe-inline'; img-src *; media-src *; child-src 'none';", fmt::join(server->getCorsHosts(), " ")));
    headers.addHeader("SameSite", "Lax");
    if (quirks)
        quirks->updateHeaders(headers);
    headers.writeHeaders(info);

    if (!uiEnabled && !startswith(path, "/icons")) {
        log_warning("UI is disabled!");
        UpnpFileInfo_set_FileLength(info, -1);
        UpnpFileInfo_set_ContentType(info, "text/html");
        UpnpFileInfo_set_IsReadable(info, -1);
        UpnpFileInfo_set_IsDirectory(info, -1);
        UpnpFileInfo_set_LastModified(info, currentTime().count());
        return quirks && quirks->getClient();
    }

    auto webFile = fmt::format("{}{}", webRoot, path);
    log_debug("UI: file: {}", webFile);

    auto mime = getMime(context->getMime(), webFile);
    log_debug("Got mime: {}", mime);

    // We should be able to do the generation here, but libupnp doesn't support the request cookies yet
    UpnpFileInfo_set_FileLength(info, -1);
    UpnpFileInfo_set_ContentType(info, mime.c_str());
    UpnpFileInfo_set_IsReadable(info, 1);
    UpnpFileInfo_set_IsDirectory(info, 0);
    UpnpFileInfo_set_LastModified(info, currentTime().count());

    return quirks && quirks->getClient();
}

std::unique_ptr<IOHandler> UIHandler::open(const char* filename, const std::shared_ptr<Quirks>& quirks, enum UpnpOpenFileMode mode)
{
    log_debug("UI: requested {}", filename);

    std::string path = filename;
    if (path == "/") {
        path = "/index.html";
    }

    auto webFile = fmt::format("{}{}", webRoot, path);
    log_debug("UI: file: {}", webFile);

    auto ioHandler = std::make_unique<FileIOHandler>(webFile);
    ioHandler->open(mode);
    return ioHandler;
}
