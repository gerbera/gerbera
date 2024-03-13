/*GRB*

Gerbera - https://gerbera.io/

    metadata_service.h - this file is part of Gerbera.

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

/// \file metadata_service.h
/// \brief Definition of the MetadataService class.
#ifndef __METADATA_SERVICE_H__
#define __METADATA_SERVICE_H__

#include <map>

#include "common.h"
#include "util/grb_fs.h"

// forward declaration
class CdsItem;
class Config;
class ContentManager;
class Context;
enum class ContentHandler;
class MetadataHandler;

enum class MetadataType {
    TagLib,
    Exiv2,
    LibExif,
    Matroska,
    WavPack,
    Ffmpeg,
    Thumbnailer,
    VideoThumbnailer,
    ImageThumbnailer,
    FanArt,
    ContainerArt,
    Subtitle,
    Metafile,
    ResourceFile,
};

class MetadataService {
private:
    std::shared_ptr<Context> context;
    std::shared_ptr<Config> config;
    std::shared_ptr<ContentManager> content;
    std::map<std::string, std::string> mappings;
    std::map<MetadataType, std::shared_ptr<MetadataHandler>> handlers;

public:
    explicit MetadataService(const std::shared_ptr<Context>& context, const std::shared_ptr<ContentManager>& content);

    void extractMetaData(const std::shared_ptr<CdsItem>& item, const fs::directory_entry& dirEnt);
    std::shared_ptr<MetadataHandler> getHandler(ContentHandler handlerType);
};

#endif // __METADATA_HANDLER_H__
