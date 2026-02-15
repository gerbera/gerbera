/*MT*

    MediaTomb - http://www.mediatomb.cc/

    ffmpeg_handler.cc - this file is part of MediaTomb.

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
/*
    This code was contributed by
    Copyright (C) 2007 Ingo Preiml <ipreiml@edu.uni-klu.ac.at>
*/

/// @file metadata/ffmpeg_handler.cc

// Information about the stream are to be found in the structure
// AVFormatContext, defined in libavformat/avformat.h:335
// and in the structure
// AVCodecContext, defined in libavcodec/avcodec.h:722
// in the ffmpeg sources

#ifdef HAVE_FFMPEG
#define GRB_LOG_FAC GrbLogFacility::ffmpeg

#include "ffmpeg_handler.h"

#include "cds/cds_item.h"
#include "config/config.h"
#include "config/config_val.h"
#include "iohandler/io_handler.h"
#include "iohandler/mem_io_handler.h"
#include "metadata_enums.h"
#include "upnp/upnp_common.h"
#include "util/grb_time.h"
#include "util/mime.h"
#include "util/string_converter.h"
#include "util/tools.h"

#include <algorithm>
#include <cstdint>
#include <fmt/chrono.h>

extern "C" {

#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/display.h>
#include <libavutil/pixdesc.h>

} // extern "C"

#ifdef HAVE_AVSTREAM_CODECPAR
#define as_codecpar(s) s->codecpar
#else
#define as_codecpar(s) s->codec
#endif

#define STREAM_NUMBER_OPTION "streamNumber"

/// @brief Wrapper class to log FFMpeg messages
class FfmpegLogger {
public:
    FfmpegLogger()
    {
        av_log_set_level(logLevel);
        av_log_set_callback(&FfmpegLogger::LogFfmpegMessage);
    }
    ~FfmpegLogger()
    {
        av_log_set_callback(nullptr);
    }

    FfmpegLogger(const FfmpegLogger&) = delete;
    FfmpegLogger& operator=(const FfmpegLogger&) = delete;

private:
    static int printPrefix;
    static int logLevel;

    static void LogFfmpegMessage(void* ptr, int level, const char* fmt, va_list vargs)
    {
        int len = av_log_format_line2(ptr, level, fmt, vargs, nullptr, 0, &printPrefix);
        std::vector<char> buf(len + 1); // need space for NUL
        std::string message;
        if (len > 0) {
            av_log_format_line2(ptr, level, fmt, vargs, buf.data(), len, &printPrefix);
            message = buf.data();
        } else {
            message = "Failed to format message";
        }

        if (level <= logLevel) { // some ffmpeg functions use log_level_offset
            switch (level) {
            case AV_LOG_PANIC: // ffmpeg will crash now
            case AV_LOG_FATAL: // fatal as in can't decode, not crash
                log_error("FFMpeg {} {}", level, message);
                break;
            case AV_LOG_ERROR:
            case AV_LOG_WARNING:
                log_debug("FFMpeg {}: {}", level, message);
                break;
            case AV_LOG_INFO:
                log_debug("FFMpeg {}: {}", level, message);
                break;
            case AV_LOG_VERBOSE:
            case AV_LOG_DEBUG:
            case AV_LOG_TRACE:
                log_debug("FFMpeg {}: {}", level, message);
                break;
            case AV_LOG_QUIET:
                break;
            default:
                log_warning("FFMpeg unhandled {}: {}", level, message);
                break;
            }
        }
    }
};

static FfmpegLogger globalFfmpegLogger = FfmpegLogger();
int FfmpegLogger::printPrefix = 1;
int FfmpegLogger::logLevel = AV_LOG_INFO;

FfmpegHandler::FfmpegHandler(const std::shared_ptr<Context>& context)
    : MediaMetadataHandler(context,
          ConfigVal::IMPORT_LIBOPTS_FFMPEG_ENABLED,
          ConfigVal::IMPORT_LIBOPTS_FFMPEG_METADATA_TAGS_LIST,
          ConfigVal::IMPORT_LIBOPTS_FFMPEG_AUXDATA_TAGS_LIST,
          ConfigVal::IMPORT_LIBOPTS_FFMPEG_COMMENT_ENABLED,
          ConfigVal::IMPORT_LIBOPTS_FFMPEG_COMMENT_LIST)
    , artWorkEnabled(config->getBoolOption(ConfigVal::IMPORT_LIBOPTS_FFMPEG_ARTWORK_ENABLED))
    , subtitleSeekSize(config->getUIntOption(ConfigVal::IMPORT_LIBOPTS_FFMPEG_SUBTITLE_SEEK_SIZE))
{
#if (LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(58, 9, 100))
    // Register all formats and codecs
    av_register_all();
#endif
}

bool FfmpegHandler::isSupported(
    const std::string& contentType,
    bool isOggTheora,
    const std::string& mimeType,
    ObjectType mediaType)
{
    return mediaType == ObjectType::Audio || mediaType == ObjectType::Video;
}

static ObjectType getStreamType(const AVStream* st)
{
    switch (as_codecpar(st)->codec_type) {
    case AVMEDIA_TYPE_VIDEO:
        return ObjectType::Video;
    case AVMEDIA_TYPE_AUDIO:
        return ObjectType::Audio;
    case AVMEDIA_TYPE_SUBTITLE:
    case AVMEDIA_TYPE_DATA:
    case AVMEDIA_TYPE_ATTACHMENT:
    case AVMEDIA_TYPE_NB:
        return ObjectType::Unknown;
    case AVMEDIA_TYPE_UNKNOWN:
    default:
        return ObjectType::Unknown;
    }
}

/// @brief Wrapper class to interface with FFMpeg
class FfmpegObject {
public:
    std::string location;
    std::shared_ptr<StringConverter> sc;
    ObjectType objType;
    AVFormatContext* pFormatCtx = nullptr;

    FfmpegObject(const std::shared_ptr<ConverterManager>& converterManager, const std::shared_ptr<CdsItem>& item)
        : location(item->getLocation())
        , sc(converterManager->m2i(ConfigVal::IMPORT_LIBOPTS_FFMPEG_CHARSET, location))
        , objType(item->getMediaType())
    {
        // ffmpeg library context
        pFormatCtx = avformat_alloc_context();
        // Open media file
        if (avformat_open_input(&pFormatCtx, item->getLocation().c_str(), nullptr, nullptr) != 0) {
            log_debug("Could not open file");
            avformat_free_context(pFormatCtx);
            pFormatCtx = nullptr;
            return; // Couldn't open file
        }

        // Retrieve stream information
        if (avformat_find_stream_info(pFormatCtx, nullptr) < 0) {
            log_debug("Could not find stream information");
            avformat_close_input(&pFormatCtx);
            pFormatCtx = nullptr;
            return; // Couldn't find stream information
        }
    }
    ~FfmpegObject()
    {
        // Close the media file
        if (pFormatCtx)
            avformat_close_input(&pFormatCtx);
        pFormatCtx = nullptr;
    }

    FfmpegObject(const FfmpegObject&) = delete;
    FfmpegObject& operator=(const FfmpegObject&) = delete;

    /// @brief check if FFMpeg information is available
    operator bool() const
    {
        if (!pFormatCtx)
            return false;
        if (!pFormatCtx->metadata) {
            for (std::size_t stream_number = 0; stream_number < pFormatCtx->nb_streams; stream_number++) {
                if (pFormatCtx->streams[stream_number]->metadata)
                    return true;
            }
            return false;
        }
        return true;
    }

    /// @brief get key value from image
    std::string getKey(const std::string& desiredTag) const
    {
        log_debug("key: {} ", desiredTag);
        if (pFormatCtx->metadata) {
            auto tag = av_dict_get(pFormatCtx->metadata, desiredTag.c_str(), nullptr, AV_DICT_IGNORE_SUFFIX);
            if (tag && tag->value && tag->value[0]) {
                auto [val, err] = sc->convert(trimString(tag->value));
                if (!err.empty()) {
                    log_warning("{} {}: {}", location, desiredTag, err);
                }
                return val;
            }
        }
        for (std::size_t stream_number = 0; stream_number < pFormatCtx->nb_streams; stream_number++) {
            if (pFormatCtx->streams[stream_number]->metadata) {
                auto tag = av_dict_get(pFormatCtx->metadata, desiredTag.c_str(), nullptr, AV_DICT_IGNORE_SUFFIX);
                if (tag && tag->value && tag->value[0]) {
                    auto [val, err] = sc->convert(trimString(tag->value));
                    if (!err.empty()) {
                        log_warning("{} stream {} {}: {}", location, stream_number, desiredTag, err);
                    }
                    return val;
                }
            }
        }
        return "";
    }
};

bool FfmpegHandler::addFfmpegAuxdataFields(
    const std::shared_ptr<CdsItem>& item,
    const FfmpegObject& ffmpegObject) const
{
    if (!ffmpegObject) {
        log_debug("no metadata");
        return false;
    }

    bool result = false;
    for (auto&& desiredTag : auxTags) {
        if (!desiredTag.empty()) {
            auto val = ffmpegObject.getKey(desiredTag);
            if (!val.empty()) {
                log_debug("Added {}: {}", desiredTag.c_str(), val);
                item->setAuxData(desiredTag, val);
                result = true;
            }
        }
    }
    return result;
}

bool FfmpegHandler::addFfmpegComment(
    const std::shared_ptr<CdsItem>& item,
    const FfmpegObject& ffmpegObject) const
{
    if (!ffmpegObject) {
        log_debug("no metadata");
        return false;
    }
    if (!isCommentEnabled)
        return false;

    std::vector<std::string> snippets;
    for (auto&& [label, desiredTag] : commentMap) {
        if (!desiredTag.empty()) {
            auto val = ffmpegObject.getKey(desiredTag);
            if (!val.empty()) {
                log_debug("Added {}: {}", label, val);
                snippets.push_back(fmt::format("{}: {}", label, val));
            }
        }
    }
    if (!snippets.empty()) {
        auto comment = fmt::format("{}", fmt::join(snippets, ", "));
        log_debug("Fabricated Comment: {}", comment);
        item->addMetaData(MetadataFields::M_DESCRIPTION, comment);
        return true;
    }
    return false;
}

bool FfmpegHandler::addFfmpegMetadataFields(
    const std::shared_ptr<CdsItem>& item,
    const FfmpegObject& ffmpegObject) const
{
    if (!ffmpegObject) {
        log_debug("no metadata");
        return false;
    }

    bool result = false;
    // only use ffmpeg meta data if not found by other handlers
    auto emptyProperties = std::map<MetadataFields, bool>();
    for (auto&& prop : propertyMap) {
        emptyProperties[prop.first] = item->getMetaData(prop.first).empty();
    }
    auto emptySpecProperties = std::map<std::string, bool>();
    for (auto&& prop : metaTags) {
        emptySpecProperties[prop.second] = item->getMetaData(prop.second).empty();
    }

    AVDictionaryEntry* avEntry = nullptr;
    if (ffmpegObject.pFormatCtx->metadata)
        while ((avEntry = av_dict_get(ffmpegObject.pFormatCtx->metadata, "", avEntry, AV_DICT_IGNORE_SUFFIX))) {
            result = getFfmpegMetadataField(item, ffmpegObject, avEntry, item->getMediaType(), emptyProperties, emptySpecProperties) || result;
        }
    for (std::size_t stream_number = 0; stream_number < ffmpegObject.pFormatCtx->nb_streams; stream_number++) {
        auto st = ffmpegObject.pFormatCtx->streams[stream_number];
        if (st->metadata) {
            auto streamType = getStreamType(st);
            while ((avEntry = av_dict_get(st->metadata, "", avEntry, AV_DICT_IGNORE_SUFFIX))) {
                result = getFfmpegMetadataField(item, ffmpegObject, avEntry, streamType, emptyProperties, emptySpecProperties) || result;
            }
        }
    }
    return result;
}

bool FfmpegHandler::getFfmpegMetadataField(
    const std::shared_ptr<CdsItem>& item,
    const FfmpegObject& ffmpegObject,
    AVDictionaryEntry* avEntry,
    ObjectType streamType,
    std::map<MetadataFields, bool>& emptyProperties,
    std::map<std::string, bool>& emptySpecProperties) const
{
    bool result = false;
    std::string key = toLower(avEntry->key);
    std::string value = avEntry->value;
    log_debug("FFMpeg tag: {}: {}", key, value);
    {
        auto [val, err] = ffmpegObject.sc->convert(trimString(value));
        if (!err.empty()) {
            log_warning("{} {}: {}", ffmpegObject.location, key, err);
        }
        value = val;
    }
    auto it = metaTags.find(avEntry->key);
    if (it == metaTags.end())
        it = metaTags.find(key); // search lowercase string as well
    if (it != metaTags.end() && emptySpecProperties[it->second]) {
        log_debug("Identified special metadata '{}' as '{}': '{}'", it->first, it->second, value);
        item->addMetaData(it->second, value);
        emptySpecProperties[it->second] = false;
        return result; // iterate while loop
    }
    auto pIt = std::find_if(propertyMap.begin(), propertyMap.end(),
        [&](auto&& p) {
            return p.second == key && item->getMetaData(p.first).empty();
        });
    if (pIt != propertyMap.end()) {
        log_debug("Identified default metadata '{}' '{}': {}", pIt->first, pIt->second, value);
        const auto field = pIt->first;
        if (field == MetadataFields::M_TITLE && streamType != item->getMediaType())
            return result;
        if (emptyProperties[field]) {
            if (field == MetadataFields::M_DATE || field == MetadataFields::M_UPNP_DATE || field == MetadataFields::M_CREATION_DATE) {
                std::tm tmWork {};
                if (!parseDate(avEntry->value, tmWork))
                    return result;
                auto mDate = fmt::format("{:%Y-%m-%d}", tmWork);
                item->addMetaData(field, mDate);
            } else {
                item->addMetaData(field, value);
            }
            emptyProperties[field] = false;
        }
        if (field == MetadataFields::M_TRACKNUMBER) {
            item->setTrackNumber(stoiString(value));
        } else if (field == MetadataFields::M_PARTNUMBER) {
            item->setPartNumber(stoiString(value));
        }
        result = true;
    } else {
        log_debug("Skipped metadata {} = '{}'", key, value);
    }
    return result;
}

/// @brief Convert Rotation Information to 360 degree value
static double get_rotation(const std::int32_t* displaymatrix)
{
    double rot = 0;
    if (displaymatrix)
        rot = -round(av_display_rotation_get(displaymatrix));
    rot -= 360 * floor(rot / 360 + 0.9 / 360);
    if (rot < -180) {
        rot += 360;
    } else if (rot >= 180.0) {
        rot -= 360;
    }
    return rot;
}

/// @brief Extract Artwork from media file
static std::vector<std::uint8_t> extractArtImage(
    const FfmpegObject& ffmpegObject,
    std::size_t imageStreamIndex)
{
    log_debug("start {}", imageStreamIndex);
    if (!ffmpegObject) {
        return {};
    }
    if (imageStreamIndex >= ffmpegObject.pFormatCtx->nb_streams) {
        return {};
    }

    AVStream* audioStream = nullptr;
    // Find Audio Stream with attaced picture
    for (std::size_t stream_number = 0; stream_number < ffmpegObject.pFormatCtx->nb_streams; stream_number++) {
        if ((ffmpegObject.pFormatCtx->streams[stream_number]->disposition & AV_DISPOSITION_ATTACHED_PIC) && stream_number == imageStreamIndex) { // implies codec_type == AVMEDIA_TYPE_AUDIO
            audioStream = ffmpegObject.pFormatCtx->streams[stream_number];
            break;
        }
    }

    if (!audioStream) {
        log_debug("No audio stream found in the file");
        return {};
    }

    // Thanks to https://stackoverflow.com/questions/13592709/retrieve-album-art-using-ffmpeg
    AVPacket pkt = audioStream->attached_pic;
    if (pkt.size > 0) {
        log_debug("end {}", pkt.size);
        return std::vector<std::uint8_t>(pkt.data, pkt.data + pkt.size);
    }

    AVDictionaryEntry* avEntry = nullptr;
    avEntry = av_dict_get(ffmpegObject.pFormatCtx->metadata, "APIC", avEntry, 0);
    if (!avEntry) {
        log_debug("No embedded image found in the file");
        return {};
    }

    log_debug("end APIC {}", strlen(avEntry->value));
    return std::vector<std::uint8_t>(avEntry->value, avEntry->value + strlen(avEntry->value));
}

/// @brief Extract Subtitle from media file
static std::vector<std::uint8_t> extractSubtitle(
    const FfmpegObject& ffmpegObject,
    long long subtitleStreamIndex,
    std::size_t maxBytes = 0)
{
#if (LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(60, 0, 0))
    log_debug("extractSubtitle disabled - ffmpeg seems broken");
    return {}; // crashes when reading some subtitles
#else
    log_debug("start");
    if (!ffmpegObject) {
        return {};
    }

    AVPacket* packet = av_packet_alloc();
    if (!packet) {
        log_debug("Could not allocate packet");
        return {};
    }

    av_seek_frame(ffmpegObject.pFormatCtx, subtitleStreamIndex, 0, 0);

    std::vector<std::uint8_t> result;
    // Store subtitle packets
    while (av_read_frame(ffmpegObject.pFormatCtx, packet) >= 0 && (maxBytes == 0 || result.size() < maxBytes)) {
        log_vdebug("checking {} vs {}", packet->stream_index, subtitleStreamIndex);
        if (packet->stream_index == subtitleStreamIndex) {
            auto chunk = std::vector<uint8_t>(packet->data, packet->data + packet->size);
            result.reserve(result.size() + chunk.size());
            result.insert(result.end(), chunk.begin(), chunk.end());
        }
        av_packet_unref(packet);
    }
    av_packet_free(&packet);
    log_debug("end {}", result.size());
    return result;
#endif
}

/// @brief extract orientation from stream
static int getOrientation(AVStream* st)
{
    AVDictionaryEntry* entry = nullptr;
    entry = av_dict_get(st->metadata, "rotate", entry, AV_DICT_MATCH_CASE);
    double rot = 0;
    if (entry) {
        rot = stoiString(entry->value);
        log_debug("{} = {}", "rotate", rot);
    } else {
#if (LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(60, 0, 0))
        auto displaymatrix = av_stream_get_side_data(st, AV_PKT_DATA_DISPLAYMATRIX, nullptr);
        if (displaymatrix) {
            rot = get_rotation(reinterpret_cast<std::int32_t*>(displaymatrix));
            log_debug("{} = {}", "displaymatrix", rot);
        }
#else
        int32_t* displayMatrix = nullptr;
        auto psd = av_packet_side_data_get(as_codecpar(st)->coded_side_data,
            as_codecpar(st)->nb_coded_side_data, AV_PKT_DATA_DISPLAYMATRIX);
        if (psd)
            displayMatrix = reinterpret_cast<std::int32_t*>(psd->data);
        if (displayMatrix) {
            rot = get_rotation(displayMatrix);
            log_debug("{} = {}", "displayMatrix", rot);
        }
#endif
    }
    int orientation = 0;
    if (rot == 0) {
        orientation = 1;
    } else if (rot == 90) {
        orientation = 6;
    } else if (rot == -90) {
        orientation = 8;
    } else if (rot == -180 || rot == 180) {
        orientation = 3;
    }
    log_debug("Orientation = {}", orientation);
    return orientation;
}

std::string FfmpegHandler::getContentTypeFromByteVector(const std::vector<std::uint8_t>& data) const
{
#ifdef HAVE_MAGIC
    auto artMimetype = mime->bufferToMimeType(data.data(), data.size());
    log_debug("found mime {}", artMimetype);
    return artMimetype.empty() ? UPNP_DESC_ICON_JPG_MIMETYPE : artMimetype;
#else
    return UPNP_DESC_ICON_JPG_MIMETYPE;
#endif
}

// duration
static void setDuration(const std::shared_ptr<CdsResource>& res, int64_t value)
{
    if (value != AV_NOPTS_VALUE) {
        auto duration = millisecondsToHMSF(value / (AV_TIME_BASE / 1000));
        log_debug("Added duration: {}", duration);
        res->addAttribute(ResourceAttribute::DURATION, duration);
    }
}

static void setBitRate(const std::shared_ptr<CdsResource>& res, int64_t value)
{
    if (value > 0) {
        // ffmpeg's bit_rate is in bits/sec, upnp wants it in bytes/sec
        // See http://www.upnp.org/schemas/av/didl-lite-v3.xsd
        auto bitrate = value / 8;
        log_debug("Added bitrate: {} kb/s", bitrate);
        res->addAttribute(ResourceAttribute::BITRATE, fmt::to_string(bitrate));
    }
}

void FfmpegHandler::setResourceAttributes(
    const std::shared_ptr<CdsResource>& res,
    AVStream* st,
    long long stream_number)
{
    for (auto&& [attr, tag] : resourceMap) {
        AVDictionaryEntry* md = (st->metadata) ? av_dict_get(st->metadata, tag, nullptr, 0) : nullptr;
        if (md && md->value) {
            log_debug("Resource {}: {}", tag, md->value);
            res->addAttribute(attr, md->value);
        } else if (stream_number > -1) {
            res->addAttribute(attr, fmt::to_string(stream_number));
        }
    }
}

bool FfmpegHandler::addFfmpegResourceFields(
    const std::shared_ptr<CdsItem>& item,
    const FfmpegObject& ffmpegObject)
{
    if (!ffmpegObject.pFormatCtx) {
        log_debug("no resource data");
        return false;
    }

    auto resource = item->getResource(ContentHandler::DEFAULT);
    bool isAudioFile = item->isSubClass(UPNP_CLASS_AUDIO_ITEM);
    bool hasThumb = !!item->getResource(ResourcePurpose::Thumbnail);
    auto resource2 = isAudioFile ? item->getResource(ResourcePurpose::Thumbnail) : resource;

    // duration
    setDuration(resource, ffmpegObject.pFormatCtx->duration);
    // bitrate
    setBitRate(resource, ffmpegObject.pFormatCtx->bit_rate);

    // video resolution, audio sampling rate, nr of audio channels
    int audioSet = 0;
    int videoSet = artWorkEnabled && isAudioFile && hasThumb ? 1 : 0;
    bool result = false;
    for (std::size_t stream_number = 0; stream_number < ffmpegObject.pFormatCtx->nb_streams; stream_number++) {
        auto st = ffmpegObject.pFormatCtx->streams[stream_number];

        if (!st)
            continue;

        switch (as_codecpar(st)->codec_type) {
        case AVMEDIA_TYPE_VIDEO: {
            if (videoSet > 0 || !resource2) {
                resource2 = std::make_shared<CdsResource>(ContentHandler::FFMPEG, isAudioFile ? ResourcePurpose::Thumbnail : ResourcePurpose::Content);
                item->addResource(resource2);
                resource2->addOption(STREAM_NUMBER_OPTION, fmt::to_string(stream_number));
            }

            if (((videoSet >= 1 && artWorkEnabled) || !hasThumb) && isAudioFile) {
                // use ffmpeg to get artwork
                auto image = extractArtImage(ffmpegObject, stream_number);
                if (!image.empty()) {
                    auto artMimetype = getContentTypeFromByteVector(image);
                    hasThumb = true;
                    log_debug("art {} {}", image.size(), artMimetype);
                    if (artMimetype != MIMETYPE_DEFAULT) {
                        resource2->addAttribute(ResourceAttribute::PROTOCOLINFO, renderProtocolInfo(artMimetype));
                    }
                }
                resource2->addAttribute(ResourceAttribute::SIZE, fmt::to_string(image.size()));
            }
            // orientation
            resource2->addAttribute(ResourceAttribute::ORIENTATION, fmt::format("{}", getOrientation(st)));

            // duration of stream
            if (st->duration * av_q2d(st->time_base) > ffmpegObject.pFormatCtx->duration || resource2->getAttribute(ResourceAttribute::DURATION).empty())
                setDuration(resource2, st->duration * av_q2d(st->time_base));
            // bitrate of stream
            setBitRate(resource2, as_codecpar(st)->bit_rate);
            setResourceAttributes(resource2, st, -1);

            // video codec
            auto codecId = as_codecpar(st)->codec_id;
            resource2->addAttribute(ResourceAttribute::VIDEOCODEC, avcodec_get_name(codecId));

            // fourcc
            if (as_codecpar(st)->codec_tag > 0) {
                char fourcc[5];
                fourcc[0] = as_codecpar(st)->codec_tag;
                fourcc[1] = as_codecpar(st)->codec_tag >> 8;
                fourcc[2] = as_codecpar(st)->codec_tag >> 16;
                fourcc[3] = as_codecpar(st)->codec_tag >> 24;
                fourcc[4] = '\0';

                log_debug("FourCC: 0x{:x} = {} from stream {}", as_codecpar(st)->codec_tag, fourcc, stream_number);
                std::string fcc = fourcc;
                if (!fcc.empty())
                    resource->addOption(RESOURCE_OPTION_FOURCC, fcc);
            }

            // resolution
            if (as_codecpar(st)->width > 0 && as_codecpar(st)->height > 0) {
                auto resolution = fmt::format("{}x{}", as_codecpar(st)->width, as_codecpar(st)->height);

                log_debug("Added resolution: {} pixel from stream {}", resolution, stream_number);
                resource2->addAttribute(ResourceAttribute::RESOLUTION, resolution);
                videoSet++;
            }

            // pixelformat
            {
                auto pix_fmt = static_cast<AVPixelFormat>(as_codecpar(st)->format);

                // Get pixel format name
                auto pix_fmt_name = av_get_pix_fmt_name(pix_fmt);
                if (pix_fmt_name) {
                    resource2->addAttribute(ResourceAttribute::PIXELFORMAT, pix_fmt_name);
                    log_debug("Pixel Format: {}", pix_fmt_name);
                } else {
                    log_debug("Unknown Pixel Format");
                }
            }
            result = true;
            break;
        }
        case AVMEDIA_TYPE_SUBTITLE: {
            auto stResource = std::make_shared<CdsResource>(ContentHandler::FFMPEG, ResourcePurpose::Subtitle);
            item->addResource(stResource);
            // subtitle codec
            auto codecId = as_codecpar(st)->codec_id;
            stResource->addAttribute(ResourceAttribute::TYPE, avcodec_get_name(codecId));
            stResource->addOption(STREAM_NUMBER_OPTION, fmt::to_string(stream_number));

            auto subtitle = extractSubtitle(ffmpegObject, stream_number, subtitleSeekSize);
            if (!subtitle.empty()) {
                auto subMimetype = getContentTypeFromByteVector(subtitle);
                log_debug("subtitle {} {}", subtitle.size(), subMimetype);
                if (subMimetype != MIMETYPE_DEFAULT) {
                    stResource->addAttribute(ResourceAttribute::PROTOCOLINFO, renderProtocolInfo(subMimetype));
                }
            }

            setResourceAttributes(stResource, st, stream_number);

            if (as_codecpar(st)->extradata && as_codecpar(st)->extradata_size > 0) {
                log_debug("Subtitle Size: {}", as_codecpar(st)->extradata_size);
                stResource->addAttribute(ResourceAttribute::SIZE, fmt::format("{}", as_codecpar(st)->extradata_size));
            } else
                stResource->addAttribute(ResourceAttribute::SIZE, fmt::to_string(subtitle.size()));
            result = true;
            break;
        }
        case AVMEDIA_TYPE_DATA: // Opaque data information usually continuous.
        case AVMEDIA_TYPE_ATTACHMENT: // Opaque data information usually sparse.
        case AVMEDIA_TYPE_NB:
            log_debug("media type {}: {}", stream_number, as_codecpar(st)->codec_type);
            break;
        case AVMEDIA_TYPE_AUDIO: {
            if (audioSet > 0) {
                resource = std::make_shared<CdsResource>(ContentHandler::FFMPEG, ResourcePurpose::Content);
                item->addResource(resource);
                resource->addOption(STREAM_NUMBER_OPTION, fmt::to_string(stream_number));
            }

            setResourceAttributes(resource, st, -1);
            // audio codec
            auto codecId = as_codecpar(st)->codec_id;
            resource->addAttribute(ResourceAttribute::AUDIOCODEC, avcodec_get_name(codecId));
            // find the first stream that has a valid sample rate

            // duration of stream
            if (st->duration * av_q2d(st->time_base) > ffmpegObject.pFormatCtx->duration || resource->getAttribute(ResourceAttribute::DURATION).empty())
                setDuration(resource, st->duration * av_q2d(st->time_base));
            // bitrate of stream
            setBitRate(resource, as_codecpar(st)->bit_rate);

            if (as_codecpar(st)->sample_rate > 0) {
                int sampleFreq = as_codecpar(st)->sample_rate;

                log_debug("Bits per raw sample: {}", as_codecpar(st)->bits_per_raw_sample);
                int bitsPerSample = as_codecpar(st)->bits_per_coded_sample;
                log_debug("Bits per coded sample: {}", bitsPerSample);

                if (bitsPerSample > 0) {
                    resource->addAttribute(ResourceAttribute::BITS_PER_SAMPLE, fmt::to_string(bitsPerSample));

                    // Fix up Sample rate reporting
                    // FFMpeg will tell us DSD is 16bit PCM, but its actually 1bit, so we have to multiply this out
                    if (codecId == AV_CODEC_ID_DSD_LSBF || codecId == AV_CODEC_ID_DSD_MSBF || codecId == AV_CODEC_ID_DSD_LSBF_PLANAR || codecId == AV_CODEC_ID_DSD_MSBF_PLANAR) {
                        sampleFreq *= bitsPerSample;
                    }
                }

                log_debug("Added sample frequency: {} Hz from stream {}", sampleFreq, stream_number);
                resource->addAttribute(ResourceAttribute::SAMPLEFREQUENCY, fmt::to_string(sampleFreq));
                audioSet++;
            }
// FF_API_OLD_CHANNEL_LAYOUT
#if LIBAVUTIL_VERSION_MAJOR < 57
            int audioCh = as_codecpar(st)->channels;
#else
            int audioCh = as_codecpar(st)->ch_layout.nb_channels;
#endif
            if (audioCh > 0) {
                log_debug("Added number of audio channels: {} from stream {}", audioCh, stream_number);
                resource->addAttribute(ResourceAttribute::NRAUDIOCHANNELS, fmt::to_string(audioCh));
            }
            result = true;
            break;
        }
        case AVMEDIA_TYPE_UNKNOWN:
            break;
        }
    }

    return result;
}

bool FfmpegHandler::fillMetadata(const std::shared_ptr<CdsObject>& obj)
{
    auto item = std::dynamic_pointer_cast<CdsItem>(obj);
    if (!item || !isEnabled)
        return false;

    log_debug("Running ffmpeg handler on {}", item->getLocation().c_str());

    FfmpegObject ffmpegObject(converterManager, item);

    bool result = false;
    // Add metadata for unset values
    result = addFfmpegMetadataFields(item, ffmpegObject) || result;
    // Add auxdata
    result = addFfmpegAuxdataFields(item, ffmpegObject) || result;
    // Add special resource properties
    result = addFfmpegResourceFields(item, ffmpegObject) || result;
    // Fabricate comment
    if (item->getMetaData(MetadataFields::M_DESCRIPTION).empty())
        result = addFfmpegComment(item, ffmpegObject) || result;
    return result;
}

std::unique_ptr<IOHandler> FfmpegHandler::serveContent(
    const std::shared_ptr<CdsObject>& obj,
    const std::shared_ptr<CdsResource>& resource)
{
    auto item = std::dynamic_pointer_cast<CdsItem>(obj);
    if (!item || !isEnabled || !resource)
        return nullptr;
    auto streamIndex = stoulString(resource->getOption(STREAM_NUMBER_OPTION));
    if (streamIndex < 1)
        return nullptr;

    log_debug("Running ffmpeg handler on {}", item->getLocation().c_str());

    if (resource->getPurpose() == ResourcePurpose::Thumbnail) {
        FfmpegObject ffmpegObject(converterManager, item);
        auto resolution = resource->getAttribute(ResourceAttribute::RESOLUTION);
        auto st = ffmpegObject.pFormatCtx->streams[streamIndex];

        if (!st) {
            log_warning("resource {} pointing to wrong stream", streamIndex);
            return nullptr;
        }

        if (as_codecpar(st)->width > 0 && as_codecpar(st)->height > 0) {
            auto res = fmt::format("{}x{}", as_codecpar(st)->width, as_codecpar(st)->height);
            if (res != resolution) {
                log_warning("resource {} pointing to wrong index {}, resolution mismatch {} - {}", streamIndex, res, resolution);
            }
            auto image = extractArtImage(ffmpegObject, streamIndex);
            if (!image.empty()) {
                return std::make_unique<MemIOHandler>(image.data(), image.size());
            }
        }
    }
    if (resource->getPurpose() == ResourcePurpose::Subtitle) {
        FfmpegObject ffmpegObject(converterManager, item);
        auto language = resource->getAttribute(ResourceAttribute::LANGUAGE);
        auto st = ffmpegObject.pFormatCtx->streams[streamIndex];

        if (!st) {
            log_warning("resource {} pointing to wrong stream", streamIndex);
            return nullptr;
        }

        AVDictionaryEntry* langEntry = av_dict_get(st->metadata, "language", nullptr, 0);
        std::string lang = (langEntry && langEntry->value) ? langEntry->value : fmt::to_string(streamIndex);

        if (language == lang) {
            auto subtitle = extractSubtitle(ffmpegObject, streamIndex);
            if (!subtitle.empty()) {
                auto subString = std::string(reinterpret_cast<const char*>(subtitle.data()), subtitle.size());
                auto [val, err] = ffmpegObject.sc->convert(subString);
                if (!err.empty()) {
                    log_warning("{} {}: {}", ffmpegObject.location, subString, err);
                }
                return std::make_unique<MemIOHandler>(val);
            }
        }
    }
    return nullptr;
}

std::string FfmpegHandler::getMimeType() const
{
    return getValueOrDefault(mimeContentTypeMappings, CONTENT_TYPE_JPG, UPNP_DESC_ICON_JPG_MIMETYPE);
}

#endif // HAVE_FFMPEG
