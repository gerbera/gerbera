/*MT*

    MediaTomb - http://www.mediatomb.cc/

    metadata_handler.cc - this file is part of MediaTomb.

    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>

    Copyright (C) 2006-2010 Gena Batyan <bgeradz@mediatomb.cc>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>,
                            Leonhard Wimmer <leo@mediatomb.cc>

    Copyright (C) 2016-2024 Gerbera Contributors

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

/// \file metadata_service.cc
#define GRB_LOG_FAC GrbLogFacility::metadata

#include "metadata_service.h" // API

#include "cds/cds_item.h"
#include "config/config.h"
#include "config/config_val.h"
#include "content/scripting/script_names.h"
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

#include "ffmpeg_thumbnailer_handler.h"
#include "metadata/metacontent_handler.h"

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
        { MetadataType::VideoThumbnailer, std::make_shared<FfmpegThumbnailerHandler>(context, ConfigVal::SERVER_EXTOPTS_FFMPEGTHUMBNAILER_VIDEO_ENABLED) },
        { MetadataType::ImageThumbnailer, std::make_shared<FfmpegThumbnailerHandler>(context, ConfigVal::SERVER_EXTOPTS_FFMPEGTHUMBNAILER_IMAGE_ENABLED) },
        { MetadataType::Thumbnailer, std::make_shared<FfmpegThumbnailerHandler>(context, ConfigVal::SERVER_EXTOPTS_FFMPEGTHUMBNAILER_ENABLED) },
#endif
        { MetadataType::FanArt, std::make_shared<FanArtHandler>(context) },
        { MetadataType::ContainerArt, std::make_shared<ContainerArtHandler>(context) },
        { MetadataType::Subtitle, std::make_shared<SubtitleHandler>(context) },
        { MetadataType::Metafile, std::make_shared<MetafileHandler>(context, content) },
        { MetadataType::ResourceFile, std::make_shared<ResourceHandler>(context) },
    };
}

void MetadataService::extractMetaData(const std::shared_ptr<CdsItem>& item, const fs::directory_entry& dirEnt)
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
    auto itemCls = item->getClass();

    if ((contentType == CONTENT_TYPE_OGG) && (isTheora(item->getLocation()))) {
        item->setFlag(OBJECT_FLAG_OGG_THEORA);
    }

#ifdef HAVE_TAGLIB
    if ((contentType == CONTENT_TYPE_MP3) || ((contentType == CONTENT_TYPE_OGG) && (!item->getFlag(OBJECT_FLAG_OGG_THEORA))) || (contentType == CONTENT_TYPE_WMA) || (contentType == CONTENT_TYPE_WAVPACK) || (contentType == CONTENT_TYPE_FLAC) || (contentType == CONTENT_TYPE_PCM) || (contentType == CONTENT_TYPE_AIFF) || (contentType == CONTENT_TYPE_APE) || (contentType == CONTENT_TYPE_MP4)) {
        handlers[MetadataType::TagLib]->fillMetadata(item);
    }
#endif // HAVE_TAGLIB

#ifdef HAVE_EXIV2
    if (startswith(itemCls, UPNP_CLASS_IMAGE_ITEM)) {
        handlers[MetadataType::Exiv2]->fillMetadata(item);
    }
#endif

#ifdef HAVE_LIBEXIF
    if (contentType == CONTENT_TYPE_JPG) {
        handlers[MetadataType::LibExif]->fillMetadata(item);
    }
#endif // HAVE_LIBEXIF

#ifdef HAVE_MATROSKA
    if (contentType == CONTENT_TYPE_MKV) {
        handlers[MetadataType::Matroska]->fillMetadata(item);
    }
#endif

#ifdef HAVE_WAVPACK
    if (contentType == CONTENT_TYPE_WAVPACK) {
        handlers[MetadataType::WavPack]->fillMetadata(item);
    }
#endif

#ifdef HAVE_FFMPEG
    if (contentType != CONTENT_TYPE_PLAYLIST && (startswith(itemCls, UPNP_CLASS_AUDIO_ITEM) || startswith(itemCls, UPNP_CLASS_VIDEO_ITEM))) {
        handlers[MetadataType::Ffmpeg]->fillMetadata(item);
    }
#else
    if (contentType == CONTENT_TYPE_AVI) {
        std::string fourcc = getAVIFourCC(dirEnt.path());
        if (!fourcc.empty()) {
            resource->addOption(RESOURCE_OPTION_FOURCC, fourcc);
        }
    }
#endif // HAVE_FFMPEG

#ifdef HAVE_FFMPEGTHUMBNAILER
    // Thumbnails for videos
    if (startswith(itemCls, UPNP_CLASS_VIDEO_ITEM))
        handlers[MetadataType::VideoThumbnailer]->fillMetadata(item);
    else if (startswith(itemCls, UPNP_CLASS_IMAGE_ITEM))
        handlers[MetadataType::ImageThumbnailer]->fillMetadata(item);
#endif

    // Fanart for audio and video
    if (startswith(itemCls, UPNP_CLASS_AUDIO_ITEM) || startswith(itemCls, UPNP_CLASS_VIDEO_ITEM))
        handlers[MetadataType::FanArt]->fillMetadata(item);

    // Subtitles for videos
    if (startswith(itemCls, UPNP_CLASS_VIDEO_ITEM))
        handlers[MetadataType::Subtitle]->fillMetadata(item);

    // Metadata from text files
    handlers[MetadataType::Metafile]->fillMetadata(item);

    handlers[MetadataType::ResourceFile]->fillMetadata(item);
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
    case ContentHandler::MP4:
    case ContentHandler::FLAC:
        break;
    }
    throw_std_runtime_error("Unknown content handler ID: {}", handlerType);
}
