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

#include <array>

#include "cds_objects.h"
#include "config/config_manager.h"
#include "content/scripting/script_names.h"
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

#ifdef HAVE_WAVPACK
#include "metadata/wavpack_handler.h"
#endif

#include "ffmpeg_thumbnailer_handler.h"
#include "metadata/metacontent_handler.h"

MetadataHandler::MetadataHandler(const std::shared_ptr<Context>& context)
    : config(context->getConfig())
    , mime(context->getMime())
{
}

void MetadataHandler::extractMetaData(const std::shared_ptr<Context>& context, const std::shared_ptr<CdsItem>& item, const fs::directory_entry& dirEnt)
{
    std::error_code ec;
    if (!isRegularFile(dirEnt, ec))
        throw_std_runtime_error("Not a file: {}", dirEnt.path().c_str());
    auto filesize = getFileSize(dirEnt);

    std::string mimetype = item->getMimeType();

    auto resource = std::make_shared<CdsResource>(ContentHandler::DEFAULT, CdsResource::Purpose::Content);
    resource->addAttribute(CdsResource::Attribute::PROTOCOLINFO, renderProtocolInfo(mimetype));
    resource->addAttribute(CdsResource::Attribute::SIZE, fmt::to_string(filesize));

    item->addResource(resource);
    item->clearMetaData();

    auto mappings = context->getConfig()->getDictionaryOption(CFG_IMPORT_MAPPINGS_MIMETYPE_TO_CONTENTTYPE_LIST);
    std::string contentType = getValueOrDefault(mappings, mimetype);
    auto itemCls = item->getClass();

    if ((contentType == CONTENT_TYPE_OGG) && (isTheora(item->getLocation()))) {
        item->setFlag(OBJECT_FLAG_OGG_THEORA);
    }

#ifdef HAVE_TAGLIB
    if ((contentType == CONTENT_TYPE_MP3) || ((contentType == CONTENT_TYPE_OGG) && (!item->getFlag(OBJECT_FLAG_OGG_THEORA))) || (contentType == CONTENT_TYPE_WMA) || (contentType == CONTENT_TYPE_WAVPACK) || (contentType == CONTENT_TYPE_FLAC) || (contentType == CONTENT_TYPE_PCM) || (contentType == CONTENT_TYPE_AIFF) || (contentType == CONTENT_TYPE_APE) || (contentType == CONTENT_TYPE_MP4)) {
        TagLibHandler(context).fillMetadata(item);
    }
#endif // HAVE_TAGLIB

#ifdef HAVE_EXIV2
    if (contentType == CONTENT_TYPE_JPG) {
        Exiv2Handler(context).fillMetadata(item);
    }
#endif

#ifdef HAVE_LIBEXIF
    if (contentType == CONTENT_TYPE_JPG) {
        LibExifHandler(context).fillMetadata(item);
    }
#endif // HAVE_LIBEXIF

#ifdef HAVE_MATROSKA
    if (contentType == CONTENT_TYPE_MKV) {
        MatroskaHandler(context).fillMetadata(item);
    }
#endif

#ifdef HAVE_WAVPACK
    if (contentType == CONTENT_TYPE_WAVPACK) {
        WavPackHandler(context).fillMetadata(item);
    }
#endif

#ifdef HAVE_FFMPEG
    if (contentType != CONTENT_TYPE_PLAYLIST && (itemCls == UPNP_CLASS_MUSIC_TRACK || itemCls == UPNP_CLASS_VIDEO_ITEM)) {
        FfmpegHandler(context).fillMetadata(item);
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
    if (itemCls == UPNP_CLASS_VIDEO_ITEM)
        FfmpegThumbnailerHandler(context, CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_VIDEO_ENABLED).fillMetadata(item);
    else if (itemCls == UPNP_CLASS_IMAGE_ITEM)
        FfmpegThumbnailerHandler(context, CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_IMAGE_ENABLED).fillMetadata(item);
#endif

    // Fanart for audio and video
    if (itemCls == UPNP_CLASS_MUSIC_TRACK || itemCls == UPNP_CLASS_VIDEO_ITEM)
        FanArtHandler(context).fillMetadata(item);

    // Subtitles for videos
    if (itemCls == UPNP_CLASS_VIDEO_ITEM)
        SubtitleHandler(context).fillMetadata(item);

    ResourceHandler(context).fillMetadata(item);
}

std::string MetadataHandler::getMetaFieldName(metadata_fields_t field)
{
    return getValueOrDefault(mt_keys, field, { "unknown" });
}

std::unique_ptr<MetadataHandler> MetadataHandler::createHandler(const std::shared_ptr<Context>& context, ContentHandler handlerType)
{
    switch (handlerType) {
    case ContentHandler::LIBEXIF:
#ifdef HAVE_LIBEXIF
        return std::make_unique<LibExifHandler>(context);
#else
        break;
#endif
    case ContentHandler::ID3:
#ifdef HAVE_TAGLIB
        return std::make_unique<TagLibHandler>(context);
#else
        break;
#endif
    case ContentHandler::MATROSKA:
#ifdef HAVE_MATROSKA
        return std::make_unique<MatroskaHandler>(context);
#else
        break;
#endif
    case ContentHandler::WAVPACK:
#ifdef HAVE_WAVPACK
        return std::make_unique<WavPackHandler>(context);
#else
        break;
#endif
    case ContentHandler::FFTH:
#ifdef HAVE_FFMPEGTHUMBNAILER
        return std::make_unique<FfmpegThumbnailerHandler>(context, CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_ENABLED);
#else
        break;
#endif
    case ContentHandler::FANART:
        return std::make_unique<FanArtHandler>(context);
    case ContentHandler::CONTAINERART:
        return std::make_unique<ContainerArtHandler>(context);
    case ContentHandler::SUBTITLE:
        return std::make_unique<SubtitleHandler>(context);
    case ContentHandler::RESOURCE:
        return std::make_unique<ResourceHandler>(context);
    case ContentHandler::DEFAULT:
    case ContentHandler::TRANSCODE:
    case ContentHandler::EXTURL:
    case ContentHandler::MP4:
    case ContentHandler::FLAC:
        break;
    }
    throw_std_runtime_error("Unknown content handler ID: {}", handlerType);
}

std::string MetadataHandler::getMimeType() const
{
    return MIMETYPE_DEFAULT;
}

static constexpr std::array chKeys = {
    std::pair(ContentHandler::DEFAULT, "Default"),
#ifdef HAVE_LIBEXIF
    std::pair(ContentHandler::LIBEXIF, "LibExif"),
#endif
#ifdef HAVE_TAGLIB
    std::pair(ContentHandler::ID3, "TagLib"),
#endif
    std::pair(ContentHandler::TRANSCODE, "Transcode"),
    std::pair(ContentHandler::EXTURL, "Exturl"),
    std::pair(ContentHandler::MP4, "MP4"),
#ifdef HAVE_FFMPEGTHUMBNAILER
    std::pair(ContentHandler::FFTH, "FFmpegThumbnailer"),
#endif
    std::pair(ContentHandler::FLAC, "Flac"),
    std::pair(ContentHandler::FANART, "Fanart"),
    std::pair(ContentHandler::CONTAINERART, "ContainerArt"),
#ifdef HAVE_MATROSKA
    std::pair(ContentHandler::MATROSKA, "Matroska"),
#endif
#ifdef HAVE_WAVPACK
    std::pair(ContentHandler::WAVPACK, "WavPack"),
#endif
    std::pair(ContentHandler::SUBTITLE, "Subtitle"),
    std::pair(ContentHandler::RESOURCE, "Resource"),
};

bool MetadataHandler::checkContentHandler(const std::string& contHandler)
{
    auto chEntry = std::find_if(chKeys.begin(), chKeys.end(), [contHandler](auto&& entry) { return contHandler == entry.second; });
    if (chEntry != chKeys.end()) {
        return true;
    }
    return false;
}

ContentHandler MetadataHandler::remapContentHandler(const std::string& contHandler)
{
    auto chEntry = std::find_if(chKeys.begin(), chKeys.end(), [contHandler](auto&& entry) { return contHandler == entry.second; });
    if (chEntry != chKeys.end()) {
        return chEntry->first;
    }
    throw_std_runtime_error("Invalid content handler value {}", contHandler);
}

ContentHandler MetadataHandler::remapContentHandler(int ch)
{
    return static_cast<ContentHandler>(ch);
}

std::string MetadataHandler::mapContentHandler2String(ContentHandler ch)
{
    auto chEntry = std::find_if(chKeys.begin(), chKeys.end(), [ch](auto&& entry) { return ch == entry.first; });
    if (chEntry != chKeys.end()) {
        return chEntry->second;
    }
    throw_std_runtime_error("Invalid content handler value {}", ch);
}

metadata_fields_t MetadataHandler::remapMetaDataField(std::string_view fieldName)
{
    for (auto&& [f, s] : mt_names) {
        if (s == fieldName) {
            return f;
        }
    }
    return M_MAX;
}
