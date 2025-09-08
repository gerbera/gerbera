/*GRB*

    Gerbera - https://gerbera.io/

    device_description_handler.h - this file is part of Gerbera.

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

/// @file request_handler/device_description_handler.h

#ifndef GERBERA_DEVICE_DESCRIPTION_HANDLER_H
#define GERBERA_DEVICE_DESCRIPTION_HANDLER_H

#include "request_handler.h"

#include <memory>
#include <netinet/in.h>
#include <string>

class UpnpXMLBuilder;

class DeviceDescriptionHandler : public RequestHandler {
public:
    explicit DeviceDescriptionHandler(const std::shared_ptr<Content>& content,
        const std::shared_ptr<UpnpXMLBuilder>& xmlBuilder,
        const std::shared_ptr<Quirks>& quirks,
        const std::string& ip, in_port_t port);

    /// \inherit
    bool getInfo(const char* filename, UpnpFileInfo* info) override;

    /// @brief Prepares the output buffer and calls the process function.
    /// @param filename Requested URL
    /// @param quirks allows modifying the content of the response based on the client
    /// @param mode either UPNP_READ or UPNP_WRITE
    /// @return the appropriate IOHandler for the request.
    std::unique_ptr<IOHandler> open(const char* filename, const std::shared_ptr<Quirks>& quirks, enum UpnpOpenFileMode mode) override;
    std::string renderDeviceDescription(const std::string& ip, in_port_t port, const std::shared_ptr<Quirks>& quirks) const;

private:
    std::string getPresentationUrl(const std::string& ip, in_port_t port) const;

    std::string ip;
    in_port_t port;
    bool useDynamicDescription { false };

    std::string deviceDescription;
};

#endif // GERBERA_DEVICE_DESCRIPTION_HANDLER_H
