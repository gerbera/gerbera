/*GRB*

    Gerbera - https://gerbera.io/

    upnp_desc_handler.h - this file is part of Gerbera.

    Copyright (C) 2024-2026 Gerbera Contributors

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

/// @file request_handler/upnp_desc_handler.h

#ifndef GERBERA_UPNP_DESC_HANDLER_H
#define GERBERA_UPNP_DESC_HANDLER_H

#include "request_handler.h"
#include "util/grb_fs.h"
#include "util/grb_net.h"

#include <memory>

class UpnpXMLBuilder;

class UpnpDescHandler : public RequestHandler {
public:
    explicit UpnpDescHandler(const std::shared_ptr<Content>& content, const std::shared_ptr<UpnpXMLBuilder>& xmlBuilder, const std::shared_ptr<Quirks>& quirks);

    bool getInfo(const char* filename, UpnpFileInfo* info) override;
    std::unique_ptr<IOHandler> open(const char* filename, const std::shared_ptr<Quirks>& quirks, enum UpnpOpenFileMode mode) override;

private:
    std::string getServiceDescription(const std::string& path, const std::shared_ptr<Quirks>& quirks);
    fs::path getPath(const std::shared_ptr<Quirks>& quirks, std::string path);

    bool useDynamicDescription { false };
};

#endif // GERBERA_UPNP_DESC_HANDLER_H
