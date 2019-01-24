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
/// \brief Implementeation of the FanArtHelper class.

#include "fanart_handler.h"
#include <sys/stat.h>

#include "common.h"
#include "file_io_handler.h"
#include "tools.h"

using namespace zmm;

const int num_names = 2;
const char* names[] = {
    "/folder.jpg",
    "/poster.jpg"
};

FanArtHandler::FanArtHandler()
    : MetadataHandler()
{
}

inline bool path_exists(String name)
{
    struct stat buffer;
    return (stat(name.c_str(), &buffer) == 0);
}

String getFolderName(Ref<CdsItem> item)
{
    String folder = item->getLocation().substring(0, item->getLocation().rindex('/'));
    log_debug("Folder name: %s\n", folder.c_str());
    return folder;
}

String getFanArtPath(String folder)
{
    String found;
    for (int i = 0; i < num_names; i++) {
        bool exists = path_exists(folder + names[i]);
        log_debug("%s: %s\n", names[i], exists ? "found" : "missing");
        if (!exists)
            continue;
        found = folder + names[i];
        break;
    }
    return found;
}

void FanArtHandler::fillMetadata(Ref<CdsItem> item)
{
    log_debug("Running fanart handler on %s\n", item->getLocation().c_str());

    String found = getFanArtPath(getFolderName(item));

    if (found != nullptr) {
        Ref<CdsResource> resource(new CdsResource(CH_FANART));
        resource->addAttribute(MetadataHandler::getResAttrName(R_PROTOCOLINFO), renderProtocolInfo("jpg"));
        resource->addParameter(_(RESOURCE_CONTENT_TYPE), _(ID3_ALBUM_ART));
        item->addResource(resource);
    }
}

Ref<IOHandler> FanArtHandler::serveContent(Ref<CdsItem> item, int resNum, off_t* data_size)
{
    String path = getFanArtPath(getFolderName(item));

    log_debug("FanArt: Opening name: %s\n", path.c_str());

    *data_size = -1;
    Ref<IOHandler> io_handler(new FileIOHandler(path));
    return io_handler;
}
