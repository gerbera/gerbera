/*GRB*

    Gerbera - https://gerbera.io/

    ui_handler.h - this file is part of Gerbera.

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

/// @file request_handler/ui_handler.h

#ifndef GERBERA_UI_HANDLER_H
#define GERBERA_UI_HANDLER_H

#include "request_handler.h"
#include "util/grb_fs.h"

#include <map>
#include <memory>
#include <string>

class Server;
class UpnpXMLBuilder;

class UIHandler : public RequestHandler {
public:
    explicit UIHandler(const std::shared_ptr<Content>& content,
        const std::shared_ptr<UpnpXMLBuilder>& xmlBuilder, const std::shared_ptr<Quirks>& quirks,
        std::shared_ptr<Server> server);

    bool getInfo(const char* filename, UpnpFileInfo* info) override;
    std::unique_ptr<IOHandler> open(const char* filename, const std::shared_ptr<Quirks>& quirks, enum UpnpOpenFileMode mode) override;

private:
    std::string webRoot;
    bool uiEnabled;
    std::string csp;
    std::string defaultMimetype;
    std::map<std::string, std::string> extensionMimetypeMapping;
    std::shared_ptr<Server> server;

    std::string getMimeType(const fs::path& path) const;
};

#endif // GERBERA_UI_HANDLER_H
