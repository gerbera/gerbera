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
    if (!string_ok(deviceDescription)) {
        deviceDescription = _("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n") + xmlBuilder->renderDeviceDescription()->print();
    }
    UpnpFileInfo_set_FileLength(info, deviceDescription.length());
    UpnpFileInfo_set_ContentType(info, "application/xml");
    UpnpFileInfo_set_IsReadable(info, 1);
    UpnpFileInfo_set_IsDirectory(info, 0);
}

zmm::Ref<IOHandler> DeviceDescriptionHandler::open(const char* filename, enum UpnpOpenFileMode mode, zmm::String range)
{
    log_debug("Device description requested!");
    return zmm::Ref<IOHandler>(new MemIOHandler(deviceDescription.c_str(), deviceDescription.length()));
}
