/*GRB*

Gerbera - https://gerbera.io/

    metadata_service.h - this file is part of Gerbera.

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

/// \file metadata_service.h
/// \brief Definition of the MetadataService class.
#ifndef __METADATA_SERVICE_H__
#define __METADATA_SERVICE_H__

#include "util/grb_fs.h"

#include <map>

// forward declaration
class CdsItem;
class Config;
class Content;
class Context;
enum class ContentHandler;
class MetadataHandler;

enum class MetadataType {
#ifdef HAVE_TAGLIB
    TagLib,
#endif
#ifdef HAVE_EXIV2
    Exiv2,
#endif
#ifdef HAVE_LIBEXIF
    LibExif,
#endif
#ifdef HAVE_MATROSKA
    Matroska,
#endif
#ifdef HAVE_WAVPACK
    WavPack,
#endif
#ifdef HAVE_FFMPEG
    Ffmpeg,
#endif
#ifdef HAVE_FFMPEGTHUMBNAILER
    Thumbnailer,
    VideoThumbnailer,
    ImageThumbnailer,
#endif
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
    std::shared_ptr<Content> content;
    std::map<std::string, std::string> mappings;
    std::map<MetadataType, std::shared_ptr<MetadataHandler>> handlers;

public:
    explicit MetadataService(const std::shared_ptr<Context>& context, const std::shared_ptr<Content>& content);

    void extractMetaData(
        const std::shared_ptr<CdsItem>& item,
        const fs::directory_entry& dirEnt);
    void attachResourceFiles(
        const std::shared_ptr<CdsItem>& item,
        const fs::directory_entry& dirEnt);
    std::shared_ptr<MetadataHandler> getHandler(ContentHandler handlerType);
};

#endif // __METADATA_HANDLER_H__
