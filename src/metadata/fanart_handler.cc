/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    fanart_handler.cc - this file is part of MediaTomb.
    
    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>
    
    Copyright (C) 2006-2010 Gena Batyan <bgeradz@mediatomb.cc>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>,
                            Leonhard Wimmer <leo@mediatomb.cc>
    
    MediaTomb is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.
    
    MediaTomb is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    version 2 along with MediaTomb; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
    
    $Id$
*/

/// \file fanart_handler.cc

#include "fanart_handler.h" // API

#include <sys/stat.h>
#include <utility>

#include "common.h"
#include "iohandler/file_io_handler.h"
#include "util/tools.h"

// These must not have a leading slash, or the "/" operator will produce
// just this, not folder and this
static const char* names[] = {
    "folder.jpg",
    "poster.jpg"
};

FanArtHandler::FanArtHandler(std::shared_ptr<ConfigManager> config)
    : MetadataHandler(std::move(config))
{
}

fs::path FanArtHandler::getFanArtPath(const std::shared_ptr<CdsItem>& item)
{
    auto folder = item->getLocation().parent_path();
    log_debug("Folder name: {}", folder.c_str());

    fs::path found;
    for (const auto& name : names) {
        auto found = folder / name;
        std::error_code ec;
        bool exists = fs::is_regular_file(found, ec); // no error throwing, please
        log_debug("{}: {}", name, exists ? "found" : "missing");
        if (!exists)
            continue;
        return found;
    }
    return "";
}

void FanArtHandler::fillMetadata(std::shared_ptr<CdsItem> item)
{
    log_debug("Running fanart handler on {}", item->getLocation().c_str());
    auto path = getFanArtPath(item);

    if (!path.empty()) {
        auto resource = std::make_shared<CdsResource>(CH_FANART);
        resource->addAttribute(MetadataHandler::getResAttrName(R_PROTOCOLINFO), renderProtocolInfo("jpg"));
        resource->addParameter(RESOURCE_CONTENT_TYPE, ID3_ALBUM_ART);
        item->addResource(resource);
    }
}

std::unique_ptr<IOHandler> FanArtHandler::serveContent(std::shared_ptr<CdsItem> item, int resNum)
{
    auto path = getFanArtPath(item);
    log_debug("FanArt: Opening name: {}", path.c_str());

    auto io_handler = std::make_unique<FileIOHandler>(path);
    return io_handler;
}
