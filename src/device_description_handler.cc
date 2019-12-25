/*GRB*

Gerbera - https://gerbera.io/

    device_description_handler.cc - this file is part of Gerbera.

    Copyright (C) 2019 Gerbera Contributors

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

#include "device_description_handler.h"
#include "mem_io_handler.h"
#include "tools.h"

DeviceDescriptionHandler::DeviceDescriptionHandler(UpnpXMLBuilder* xmlBuilder)
    : RequestHandler()
    , xmlBuilder(xmlBuilder) {}

void DeviceDescriptionHandler::getInfo(const char* filename, UpnpFileInfo* info)
{
    // We should be able to do the generation here, but libupnp doesnt support the request cookies yet
    UpnpFileInfo_set_FileLength(info, -1);
    UpnpFileInfo_set_ContentType(info, "application/xml");
    UpnpFileInfo_set_IsReadable(info, 1);
    UpnpFileInfo_set_IsDirectory(info, 0);
}

zmm::Ref<IOHandler> DeviceDescriptionHandler::open(const char* filename, enum UpnpOpenFileMode mode, std::string range)
{
    log_debug("Device description requested\n");
    if (!string_ok(deviceDescription)) { // This always true for now
        deviceDescription = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" + xmlBuilder->renderDeviceDescription()->print();
    }
    auto t = zmm::Ref<IOHandler>(new MemIOHandler(deviceDescription));
    t->open(mode);
    return t;
}
