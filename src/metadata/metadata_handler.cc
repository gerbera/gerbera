/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    metadata_handler.cc - this file is part of MediaTomb.
    
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

/// \file metadata_handler.cc

#include "metadata_handler.h" // API

#include <filesystem>
#include <utility>

#include "cds_objects.h"
#include "config/config_manager.h"
#include "util/tools.h"

#ifdef HAVE_EXIV2
#include "metadata/exiv2_handler.h"
#endif

#ifdef HAVE_TAGLIB
#include "metadata/taglib_handler.h"
#endif // HAVE_TAGLIB

#ifdef HAVE_FFMPEG
#include "metadata/ffmpeg_handler.h"
#endif

#ifdef HAVE_LIBEXIF
#include "metadata/libexif_handler.h"
#endif

#ifdef HAVE_MATROSKA
#include "metadata/matroska_handler.h"
#endif

#include "metadata/metacontent_handler.h"

MetadataHandler::MetadataHandler(std::shared_ptr<Config> config, std::shared_ptr<Mime> mime)
    : config(std::move(config))
    , mime(std::move(mime))
{
}

void MetadataHandler::setMetadata(const std::shared_ptr<Config>& config, std::shared_ptr<Mime> mime, const std::shared_ptr<CdsItem>& item)
{
    std::string location = item->getLocation();
    std::error_code ec;
    if (!isRegularFile(location, ec))
        throw_std_runtime_error("Not a file: " + location);
    auto filesize = getFileSize(location);

    std::string mimetype = item->getMimeType();

    auto resource = std::make_shared<CdsResource>(CH_DEFAULT);
    resource->addAttribute(R_PROTOCOLINFO, renderProtocolInfo(mimetype));
    resource->addAttribute(R_SIZE, std::to_string(filesize));

    item->addResource(resource);

    auto mappings = config->getDictionaryOption(CFG_IMPORT_MAPPINGS_MIMETYPE_TO_CONTENTTYPE_LIST);
    std::string content_type = getValueOrDefault(mappings, mimetype);

    if ((content_type == CONTENT_TYPE_OGG) && (isTheora(item->getLocation()))) {
        item->setFlag(OBJECT_FLAG_OGG_THEORA);
    }

#ifdef HAVE_TAGLIB
    if ((content_type == CONTENT_TYPE_MP3) || ((content_type == CONTENT_TYPE_OGG) && (!item->getFlag(OBJECT_FLAG_OGG_THEORA))) || (content_type == CONTENT_TYPE_WMA) || (content_type == CONTENT_TYPE_WAVPACK) || (content_type == CONTENT_TYPE_FLAC) || (content_type == CONTENT_TYPE_PCM) || (content_type == CONTENT_TYPE_AIFF) || (content_type == CONTENT_TYPE_APE) || (content_type == CONTENT_TYPE_MP4)) {
        TagLibHandler(config, mime).fillMetadata(item);
    }
#endif // HAVE_TAGLIB

#ifdef HAVE_EXIV2
    if (content_type == CONTENT_TYPE_JPG) {
        Exiv2Handler(config, mime).fillMetadata(item);
    }
#endif

#ifdef HAVE_LIBEXIF
    if (content_type == CONTENT_TYPE_JPG) {
        LibExifHandler(config, mime).fillMetadata(item);
    }
#endif // HAVE_LIBEXIF

#ifdef HAVE_MATROSKA
    if (content_type == CONTENT_TYPE_MKV) {
        MatroskaHandler(config, mime).fillMetadata(item);
    }
#endif

#ifdef HAVE_FFMPEG
    if (content_type != CONTENT_TYPE_PLAYLIST && ((content_type == CONTENT_TYPE_OGG && item->getFlag(OBJECT_FLAG_OGG_THEORA)) || startswith(item->getMimeType(), "video") || startswith(item->getMimeType(), "audio"))) {
        FfmpegHandler(config, mime).fillMetadata(item);
    }
#else
    if (content_type == CONTENT_TYPE_AVI) {
        std::string fourcc = getAVIFourCC(item->getLocation());
        if (!fourcc.empty()) {
            item->getResource(0)->addOption(RESOURCE_OPTION_FOURCC,
                fourcc);
        }
    }
#endif // HAVE_FFMPEG

    // Fanart for audio and video
    if (startswith(mimetype, "video") || startswith(mimetype, "audio"))
        FanArtHandler(config, mime).fillMetadata(item);
    // Subtitles for videos
    if (startswith(mimetype, "video"))
        SubtitleHandler(config, mime).fillMetadata(item);
    ResourceHandler(config, mime).fillMetadata(item);
}

std::string MetadataHandler::getMetaFieldName(metadata_fields_t field)
{
    return mt_keys.at(field).second;
}

std::string MetadataHandler::getResAttrName(resource_attributes_t attr)
{
    return res_keys.at(attr).second;
}

std::unique_ptr<MetadataHandler> MetadataHandler::createHandler(const std::shared_ptr<Config>& config, std::shared_ptr<Mime> mime, int handlerType)
{
    switch (handlerType) {
#ifdef HAVE_LIBEXIF
    case CH_LIBEXIF:
        return std::make_unique<LibExifHandler>(config, mime);
#endif
#ifdef HAVE_TAGLIB
    case CH_ID3:
        return std::make_unique<TagLibHandler>(config, mime);
#endif
#ifdef HAVE_MATROSKA
    case CH_MATROSKA:
        return std::make_unique<MatroskaHandler>(config, mime);
#endif
#if defined(HAVE_FFMPEG) && defined(HAVE_FFMPEGTHUMBNAILER)
    case CH_FFTH:
        return std::make_unique<FfmpegHandler>(config, mime);
#endif
    case CH_FANART:
        return std::make_unique<FanArtHandler>(config, mime);
    case CH_SUBTITLE:
        return std::make_unique<SubtitleHandler>(config, mime);
    case CH_RESOURCE:
        return std::make_unique<ResourceHandler>(config, mime);
    default:
        throw_std_runtime_error("unknown content handler ID: " + std::to_string(handlerType));
    }
}

std::string MetadataHandler::getMimeType()
{
    return MIMETYPE_DEFAULT;
}

const char* MetadataHandler::mapContentHandler2String(int ch)
{
    switch (ch) {
    case CH_DEFAULT:
        return "Default";
#ifdef HAVE_LIBEXIF
    case CH_LIBEXIF:
        return "LibExif";
#endif
#ifdef HAVE_TAGLIB
    case CH_ID3:
        return "TagLib";
#endif
    case CH_TRANSCODE:
        return "Transcode";
    case CH_EXTURL:
        return "Exturl";
    case CH_MP4:
        return "MP4";
#if defined(HAVE_FFMPEG) && defined(HAVE_FFMPEGTHUMBNAILER)
    case CH_FFTH:
        return "FFmpegThumbnailer";
#endif
    case CH_FLAC:
        return "Flac";
    case CH_FANART:
        return "Fanart";
#ifdef HAVE_MATROSKA
    case CH_MATROSKA:
        return "Matroska";
#endif
    case CH_SUBTITLE:
        return "Subtitle";
    case CH_RESOURCE:
        return "Resource";
    default:
        return "Unknown";
    }
}
