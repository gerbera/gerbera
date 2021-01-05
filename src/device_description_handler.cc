/*GRB*

    Gerbera - https://gerbera.io/

    device_description_handler.cc - this file is part of Gerbera.

    Copyright (C) 2020-2021 Gerbera Contributors

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

#include "device_description_handler.h" // API

#include <utility>

#include "iohandler/mem_io_handler.h"
#include "util/tools.h"

DeviceDescriptionHandler::DeviceDescriptionHandler(std::shared_ptr<ContentManager> content, UpnpXMLBuilder* xmlBuilder)
    : RequestHandler(std::move(content))
    , xmlBuilder(xmlBuilder)
{
    auto desc = xmlBuilder->renderDeviceDescription();

    std::ostringstream buf;
    desc->print(buf, "", 0);
    deviceDescription = buf.str();
}

void DeviceDescriptionHandler::getInfo(const char* filename, UpnpFileInfo* info)
{
    // We should be able to do the generation here, but libupnp doesnt support the request cookies yet
    UpnpFileInfo_set_FileLength(info, -1);
#if defined(USING_NPUPNP)
    UpnpFileInfo_set_ContentType(info, "application/xml");
#else
    UpnpFileInfo_set_ContentType(info, ixmlCloneDOMString("application/xml"));
#endif
    UpnpFileInfo_set_IsReadable(info, 1);
    UpnpFileInfo_set_IsDirectory(info, 0);
}

std::unique_ptr<IOHandler> DeviceDescriptionHandler::open(const char* filename, enum UpnpOpenFileMode mode)
{
    log_debug("Device description requested");

    auto t = std::make_unique<MemIOHandler>(deviceDescription);
    t->open(mode);
    return t;
}
