/*MT*

    MediaTomb - http://www.mediatomb.cc/

    metadata_handler.cc - this file is part of MediaTomb.

    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>

    Copyright (C) 2006-2010 Gena Batyan <bgeradz@mediatomb.cc>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>,
                            Leonhard Wimmer <leo@mediatomb.cc>

    Copyright (C) 2016-2026 Gerbera Contributors

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

/// @file metadata/metadata_service.cc
#define GRB_LOG_FAC GrbLogFacility::metadata

#include "metadata_service.h" // API

#include "cds/cds_enums.h"
#include "cds/cds_item.h"
#include "config/config.h"
#include "config/config_val.h"
#include "context.h"
#include "exceptions.h"
#include "metadata_enums.h"
#include "util/tools.h"

#ifdef HAVE_EXIV2
#include "metadata/exiv2_handler.h"
#endif

#ifdef HAVE_TAGLIB
#include "metadata/taglib_handler.h"
#endif

#ifdef HAVE_FFMPEG
#include "metadata/ffmpeg_handler.h"
#endif

#ifdef HAVE_LIBEXIF
#include "metadata/libexif_handler.h"
#endif

#ifdef HAVE_MATROSKA
#include "metadata/matroska_handler.h"
#endif

#ifdef HAVE_WAVPACK
#include "metadata/wavpack_handler.h"
#endif

#ifdef HAVE_FFMPEGTHUMBNAILER
#include "ffmpeg_thumbnailer_handler.h"
#endif

#include "metadata/metacontent_handler.h"

#include <array>

static const std::map<MetadataType, std::string_view> handlerNames {
#ifdef HAVE_TAGLIB
    { MetadataType::TagLib, "TagLib" },
#endif
#ifdef HAVE_EXIV2
    { MetadataType::Exiv2, "Exiv2" },
#endif
#ifdef HAVE_LIBEXIF
    { MetadataType::LibExif, "LibExif" },
#endif
#ifdef HAVE_MATROSKA
    { MetadataType::Matroska, "Matroska" },
#endif
#ifdef HAVE_WAVPACK
    { MetadataType::WavPack, "WavPack" },
#endif
#ifdef HAVE_FFMPEG
    { MetadataType::Ffmpeg, "Ffmpeg" },
#endif
#ifdef HAVE_FFMPEGTHUMBNAILER
    { MetadataType::Thumbnailer, "Thumbnailer" },
    { MetadataType::VideoThumbnailer, "VideoThumbnailer" },
    { MetadataType::ImageThumbnailer, "ImageThumbnailer" },
#endif
    { MetadataType::FanArt, "FanArt" },
    { MetadataType::ContainerArt, "ContainerArt" },
    { MetadataType::Subtitle, "Subtitle" },
    { MetadataType::Metafile, "Metafile" },
    { MetadataType::ResourceFile, "ResourceFile" },
};

MetadataService::MetadataService(const std::shared_ptr<Context>& context, const std::shared_ptr<Content>& content)
    : context(context)
    , config(context->getConfig())
    , content(content)
{
    mappings = config->getDictionaryOption(ConfigVal::IMPORT_MAPPINGS_MIMETYPE_TO_CONTENTTYPE_LIST);

    handlers = std::map<MetadataType, std::shared_ptr<MetadataHandler>> {
#ifdef HAVE_TAGLIB
        { MetadataType::TagLib, std::make_shared<TagLibHandler>(context) },
#endif
#ifdef HAVE_EXIV2
        { MetadataType::Exiv2, std::make_shared<Exiv2Handler>(context) },
#endif
#ifdef HAVE_LIBEXIF
        { MetadataType::LibExif, std::make_shared<LibExifHandler>(context) },
#endif
#ifdef HAVE_MATROSKA
        { MetadataType::Matroska, std::make_shared<MatroskaHandler>(context) },
#endif
#ifdef HAVE_WAVPACK
        { MetadataType::WavPack, std::make_shared<WavPackHandler>(context) },
#endif
#ifdef HAVE_FFMPEG
        { MetadataType::Ffmpeg, std::make_shared<FfmpegHandler>(context) },
#endif
#ifdef HAVE_FFMPEGTHUMBNAILER
        { MetadataType::VideoThumbnailer, std::make_shared<FfmpegThumbnailerHandler>(context, ConfigVal::SERVER_EXTOPTS_FFMPEGTHUMBNAILER_VIDEO_ENABLED, ObjectType::Video) },
        { MetadataType::ImageThumbnailer, std::make_shared<FfmpegThumbnailerHandler>(context, ConfigVal::SERVER_EXTOPTS_FFMPEGTHUMBNAILER_IMAGE_ENABLED, ObjectType::Image) },
        { MetadataType::Thumbnailer, std::make_shared<FfmpegThumbnailerHandler>(context, ConfigVal::SERVER_EXTOPTS_FFMPEGTHUMBNAILER_ENABLED, ObjectType::Unknown) },
#endif
        { MetadataType::FanArt, std::make_shared<FanArtHandler>(context) },
        { MetadataType::ContainerArt, std::make_shared<ContainerArtHandler>(context) },
        { MetadataType::Subtitle, std::make_shared<SubtitleHandler>(context) },
        { MetadataType::Metafile, std::make_shared<MetafileHandler>(context, content) },
        { MetadataType::ResourceFile, std::make_shared<ResourceHandler>(context) },
    };
}

bool MetadataService::extractMetaData(
    const std::shared_ptr<CdsItem>& item,
    const fs::directory_entry& dirEnt)
{
    std::error_code ec;
    if (!isRegularFile(dirEnt, ec))
        throw_std_runtime_error("Not a file: {}", dirEnt.path().c_str());
    auto filesize = getFileSize(dirEnt);

    std::string mimetype = item->getMimeType();

    auto resource = std::make_shared<CdsResource>(ContentHandler::DEFAULT, ResourcePurpose::Content);
    resource->addAttribute(ResourceAttribute::PROTOCOLINFO, renderProtocolInfo(mimetype));
    resource->addAttribute(ResourceAttribute::SIZE, fmt::to_string(filesize));

    item->addResource(resource);
    item->clearMetaData();

    std::string contentType = getValueOrDefault(mappings, mimetype);
    bool isOggTheora = false;
    if ((contentType == CONTENT_TYPE_OGG) && (isTheora(item->getLocation()))) {
        item->setFlag(OBJECT_FLAG_OGG_THEORA);
        isOggTheora = true;
    }

    auto mediaType = item->getMediaType(contentType);
    constexpr auto metaHandlers = std::array {
#ifdef HAVE_TAGLIB
        MetadataType::TagLib,
#endif
#ifdef HAVE_EXIV2
        MetadataType::Exiv2,
#endif
#ifdef HAVE_LIBEXIF
        MetadataType::LibExif,
#endif
#ifdef HAVE_MATROSKA
        MetadataType::Matroska,
#endif
#ifdef HAVE_WAVPACK
        MetadataType::WavPack,
#endif
#ifdef HAVE_FFMPEG
        MetadataType::Ffmpeg,
#endif
        // Metadata from text files
        MetadataType::Metafile,
    };
    bool result = false;
    for (auto handler : metaHandlers) {
        if (handlers.at(handler)->isSupported(contentType, isOggTheora, mimetype, mediaType)) {
            try {
                log_debug("Running {} for {}", handlerNames.at(handler), item->getLocation().c_str());
                auto handlerResult = handlers.at(handler)->fillMetadata(item);
                result = result || handlerResult;
            } catch (const std::exception& ex) {
                log_error("fillMetadata {} failed for {}: {}", handlerNames.at(handler), item->getLocation().c_str(), ex.what());
            }
        }
    }

#ifndef HAVE_FFMPEG
    if (contentType == CONTENT_TYPE_AVI) {
        std::string fourcc = getAVIFourCC(dirEnt.path());
        if (!fourcc.empty()) {
            resource->addOption(RESOURCE_OPTION_FOURCC, fourcc);
            result = true;
        }
    }
#endif // HAVE_FFMPEG

    return result;
}

bool MetadataService::attachResourceFiles(
    const std::shared_ptr<CdsItem>& item,
    const fs::directory_entry& dirEnt)
{
    std::error_code ec;
    if (!isRegularFile(dirEnt, ec))
        throw_std_runtime_error("Not a file: {}", dirEnt.path().c_str());

    auto mediaType = item->getMediaType();
    std::string mimeType = item->getMimeType();
    std::string contentType = getValueOrDefault(mappings, mimeType);

    constexpr auto metaHandlers = std::array {
#ifdef HAVE_FFMPEGTHUMBNAILER
        // Thumbnails for videos and images
        MetadataType::VideoThumbnailer,
        MetadataType::ImageThumbnailer,
#endif
        // Fanart for audio and video
        MetadataType::FanArt,
        // Subtitles for videos
        MetadataType::Subtitle,
        // Resource triggers
        MetadataType::ResourceFile,
    };
    bool result = false;
    for (auto handler : metaHandlers) {
        if (handlers.at(handler)->isSupported(contentType, false, mimeType, mediaType)) {
            try {
                log_debug("Running {} for {}", handlerNames.at(handler), item->getLocation().c_str());
                auto handlerResult = handlers.at(handler)->fillMetadata(item);
                result = result || handlerResult;
            } catch (const std::exception& ex) {
                log_error("fillMetadata {} failed for {}: {}", handlerNames.at(handler), item->getLocation().c_str(), ex.what());
            }
        } else {
            log_debug("Handler {} not supported for {}, {}, {}, {}", handlerNames.at(handler), item->getLocation().c_str(), contentType, mimeType, EnumMapper::mapObjectType(mediaType));
        }
    }

    return result;
}

std::shared_ptr<MetadataHandler> MetadataService::getHandler(ContentHandler handlerType)
{
    switch (handlerType) {
    case ContentHandler::LIBEXIF:
#ifdef HAVE_LIBEXIF
        return handlers[MetadataType::LibExif];
#else
        break;
#endif
    case ContentHandler::ID3:
#ifdef HAVE_TAGLIB
        return handlers[MetadataType::TagLib];
#else
        break;
#endif
    case ContentHandler::MATROSKA:
#ifdef HAVE_MATROSKA
        return handlers[MetadataType::Matroska];
#else
        break;
#endif
    case ContentHandler::WAVPACK:
#ifdef HAVE_WAVPACK
        return handlers[MetadataType::WavPack];
#else
        break;
#endif
    case ContentHandler::FFTH:
#ifdef HAVE_FFMPEGTHUMBNAILER
        return handlers[MetadataType::Thumbnailer];
#else
        break;
#endif
    case ContentHandler::FFMPEG:
#ifdef HAVE_FFMPEG
        return handlers[MetadataType::Ffmpeg];
#else
        break;
#endif
    case ContentHandler::FANART:
        return handlers[MetadataType::FanArt];
    case ContentHandler::CONTAINERART:
        return handlers[MetadataType::ContainerArt];
    case ContentHandler::SUBTITLE:
        return handlers[MetadataType::Subtitle];
    case ContentHandler::RESOURCE:
        return handlers[MetadataType::ResourceFile];
    case ContentHandler::METAFILE:
        return handlers[MetadataType::Metafile];
    case ContentHandler::DEFAULT:
    case ContentHandler::TRANSCODE:
    case ContentHandler::EXTURL:
    case ContentHandler::FLAC:
        break;
    }
    throw_std_runtime_error("Unknown content handler ID: {}", handlerType);
}
