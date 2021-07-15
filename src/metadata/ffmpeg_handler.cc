/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    ffmpeg_handler.cc - this file is part of MediaTomb.
    
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
/*
    This code was contributed by
    Copyright (C) 2007 Ingo Preiml <ipreiml@edu.uni-klu.ac.at>
*/

/// \file ffmpeg_handler.cc

// Information about the stream are to be found in the structure
// AVFormatContext, defined in libavformat/avformat.h:335
// and in the structure
// AVCodecContext, defined in libavcodec/avcodec.h:722
// in the ffmpeg sources

#ifdef HAVE_FFMPEG
#include "ffmpeg_handler.h"

// ffmpeg needs the following sources
// INT64_C is not defined in ffmpeg/avformat.h but is needed
// macro defines included via autoconfig.h
#include <cerrno>
#include <cinttypes>
#include <cstdint>
#include <cstring>

#include <sys/stat.h>

extern "C" {

#include <libavformat/avformat.h>
#include <libavutil/avutil.h>

} // extern "C"

#ifdef HAVE_FFMPEGTHUMBNAILER
#include "iohandler/mem_io_handler.h"
#include <libffmpegthumbnailer/filmstripfilter.h>
#include <libffmpegthumbnailer/videothumbnailer.h>
#endif

#include "cds_objects.h"
#include "config/config_manager.h"
#include "util/string_converter.h"
#include "util/tools.h"

#ifdef HAVE_AVSTREAM_CODECPAR
#define as_codecpar(s) s->codecpar
#else
#define as_codecpar(s) s->codec
#endif

void FfmpegHandler::addFfmpegAuxdataFields(const std::shared_ptr<CdsItem>& item, AVFormatContext* pFormatCtx) const
{
    if (!pFormatCtx->metadata) {
        log_debug("no metadata");
        return;
    }

    auto sc = StringConverter::m2i(CFG_IMPORT_LIBOPTS_FFMPEG_CHARSET, item->getLocation(), config);
    std::vector<std::string> aux = config->getArrayOption(CFG_IMPORT_LIBOPTS_FFMPEG_AUXDATA_TAGS_LIST);
    for (auto&& desiredTag : aux) {
        if (!desiredTag.empty()) {
            AVDictionaryEntry* tag = nullptr;
            tag = av_dict_get(pFormatCtx->metadata, desiredTag.c_str(), nullptr, AV_DICT_IGNORE_SUFFIX);
            if (tag && tag->value && tag->value[0]) {
                log_debug("Added {}: {}", desiredTag.c_str(), tag->value);
                item->setAuxData(desiredTag, sc->convert(tag->value));
            }
        }
    }
} //addFfmpegAuxdataFields

void FfmpegHandler::addFfmpegMetadataFields(const std::shared_ptr<CdsItem>& item, AVFormatContext* pFormatCtx) const
{
    AVDictionaryEntry* e = nullptr;
    auto sc = StringConverter::m2i(CFG_IMPORT_LIBOPTS_FFMPEG_CHARSET, item->getLocation(), config);
    metadata_fields_t field;
    std::string value;

    while ((e = av_dict_get(pFormatCtx->metadata, "", e, AV_DICT_IGNORE_SUFFIX))) {
        value = e->value;

        if (strcmp(e->key, "title") == 0) {
            log_debug("Identified metadata title: {}", e->value);
            field = M_TITLE;
        } else if (strcmp(e->key, "artist") == 0) {
            log_debug("Identified metadata artist: {}", e->value);
            field = M_ARTIST;
        } else if (strcmp(e->key, "album") == 0) {
            log_debug("Identified metadata album: {}", e->value);
            field = M_ALBUM;
        } else if (strcmp(e->key, "date") == 0) {
            if ((value.length() == 4) && std::all_of(value.begin(), value.end(), ::isdigit) && (std::stoi(value) > 0)) {
                value.append("-01-01");
                log_debug("Identified metadata date: {}", value.c_str());
            }
            /// \toto parse possible ISO8601 timestamp
            field = M_DATE;
        } else if (strcmp(e->key, "creation_time") == 0) {
            log_debug("Identified metadata creation_time: {}", e->value);
            field = M_CREATION_DATE;
            struct tm tm_work;
            char m_date[] = "YYYY-mm-dd";
            if (strptime(e->value, "%Y-%m-%dT%T.000000%Z", &tm_work)) {
                time_t utc_time;
                // convert creation_time to local time
                utc_time = timegm(&tm_work);
                if (utc_time == time_t(-1)) {
                    continue;
                }
                localtime_r(&utc_time, &tm_work);
            } else if (!strptime(e->value, "%Y-%m-%d", &tm_work)) { // use creation_time as is
                continue;
            }
            strftime(m_date, sizeof(m_date), "%F", &tm_work);
            value = m_date;
        } else if (strcmp(e->key, "genre") == 0) {
            log_debug("Identified metadata genre: {}", e->value);
            field = M_GENRE;
        } else if (strcmp(e->key, "description") == 0) {
            log_debug("Identified metadata description: {}", e->value);
            field = M_DESCRIPTION;
        } else if (strcmp(e->key, "track") == 0) {
            log_debug("Identified metadata track: {}", e->value);
            field = M_TRACKNUMBER;
        } else if (strcmp(e->key, "discnumber") == 0) {
            log_debug("Identified metadata disk: {}", e->value);
            field = M_PARTNUMBER;
        } else if (strcmp(e->key, "album_artist") == 0) {
            log_debug("Identified metadata album_artist: {}", e->value);
            field = M_ALBUMARTIST;
        } else if (strcmp(e->key, "composer") == 0) {
            log_debug("Identified metadata composer: {}", e->value);
            field = M_COMPOSER;
        } else if (strcmp(e->key, "performer") == 0) {
            log_debug("Identified metadata performer: {}", e->value);
            field = M_CONDUCTOR;
        } else {
            continue;
        }

        // only use ffmpeg meta data if not found by other handler
        if (item->getMetadata(field).empty()) {
            item->setMetadata(field, sc->convert(trimString(value)));
            if (field == M_TRACKNUMBER) {
                item->setTrackNumber(stoiString(value));
            }
            if (field == M_PARTNUMBER) {
                item->setPartNumber(stoiString(value));
            }
        }
    }
}

// ffmpeg library calls
void FfmpegHandler::addFfmpegResourceFields(const std::shared_ptr<CdsItem>& item, AVFormatContext* pFormatCtx) const
{
    int audioch = 0, samplefreq = 0;
    bool audioset, videoset;
    auto resource = item->getResource(0);
    auto resource2 = startswith(item->getMimeType(), "audio") && item->getResourceCount() > 1 && item->getResource(1)->isMetaResource(ID3_ALBUM_ART) ? item->getResource(1) : item->getResource(0);

    // duration
    if (pFormatCtx->duration > 0) {
        auto duration = millisecondsToHMSF(pFormatCtx->duration / (AV_TIME_BASE / 1000));
        log_debug("Added duration: {}", duration);
        resource->addAttribute(R_DURATION, std::move(duration));
    }

    // bitrate
    if (pFormatCtx->bit_rate > 0) {
        // ffmpeg's bit_rate is in bits/sec, upnp wants it in bytes/sec
        // See http://www.upnp.org/schemas/av/didl-lite-v3.xsd
        auto bitrate = pFormatCtx->bit_rate / 8;
        log_debug("Added bitrate: {} kb/s", bitrate);
        resource->addAttribute(R_BITRATE, fmt::to_string(bitrate));
    }

    // video resolution, audio sampling rate, nr of audio channels
    audioset = false;
    videoset = false;
    for (size_t i = 0; i < pFormatCtx->nb_streams; i++) {
        AVStream* st = pFormatCtx->streams[i];

        if ((st) && (!videoset) && (as_codecpar(st)->codec_type == AVMEDIA_TYPE_VIDEO)) {
            auto codec_id = as_codecpar(st)->codec_id;
            resource->addAttribute(R_VIDEOCODEC, avcodec_get_name(codec_id));

            if (as_codecpar(st)->codec_tag > 0) {
                char fourcc[5];
                fourcc[0] = as_codecpar(st)->codec_tag;
                fourcc[1] = as_codecpar(st)->codec_tag >> 8;
                fourcc[2] = as_codecpar(st)->codec_tag >> 16;
                fourcc[3] = as_codecpar(st)->codec_tag >> 24;
                fourcc[4] = '\0';

                log_debug("FourCC: 0x{:x} = {} from stream {}", as_codecpar(st)->codec_tag, fourcc, i);
                std::string fcc = fourcc;
                if (!fcc.empty())
                    resource->addOption(RESOURCE_OPTION_FOURCC, fcc);
            }

            if ((as_codecpar(st)->width > 0) && (as_codecpar(st)->height > 0)) {
                auto resolution = fmt::format("{}x{}", as_codecpar(st)->width, as_codecpar(st)->height);

                log_debug("Added resolution: {} pixel from stream {}", resolution, i);
                resource2->addAttribute(R_RESOLUTION, std::move(resolution));
                videoset = true;
            }
        }
        if ((st) && (!audioset) && (as_codecpar(st)->codec_type == AVMEDIA_TYPE_AUDIO)) {
            auto codec_id = as_codecpar(st)->codec_id;
            resource->addAttribute(R_AUDIOCODEC, avcodec_get_name(codec_id));
            // find the first stream that has a valid sample rate
            if (as_codecpar(st)->sample_rate > 0) {
                samplefreq = as_codecpar(st)->sample_rate;
                log_debug("Added sample frequency: {} Hz from stream {}", samplefreq, i);
                resource->addAttribute(R_SAMPLEFREQUENCY, fmt::to_string(samplefreq));
                audioset = true;

                audioch = as_codecpar(st)->channels;
                if (audioch > 0) {
                    log_debug("Added number of audio channels: {} from stream {}", audioch, i);
                    resource->addAttribute(R_NRAUDIOCHANNELS, fmt::to_string(audioch));
                }
            }
        }
    }

#if defined(HAVE_FFMPEG) && defined(HAVE_FFMPEGTHUMBNAILER)
    if (config->getBoolOption(CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_ENABLED) && (startswith(item->getMimeType(), "video") || item->getFlag(OBJECT_FLAG_OGG_THEORA))) {
        std::string videoresolution = item->getResource(0)->getAttribute(R_RESOLUTION);
        int x;
        int y;

        if (!videoresolution.empty() && checkResolution(videoresolution, &x, &y)) {
            auto mappings = config->getDictionaryOption(CFG_IMPORT_MAPPINGS_MIMETYPE_TO_CONTENTTYPE_LIST);

            auto it = mappings.find(CONTENT_TYPE_JPG);
            std::string thumb_mimetype = it != mappings.end() && !it->second.empty() ? it->second : "image/jpeg";

            auto ffres = std::make_shared<CdsResource>(CH_FFTH);
            ffres->addParameter(RESOURCE_HANDLER, fmt::to_string(CH_FFTH));
            ffres->addOption(RESOURCE_CONTENT_TYPE, THUMBNAIL);
            ffres->addAttribute(R_PROTOCOLINFO, renderProtocolInfo(thumb_mimetype));

            y = config->getIntOption(CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_THUMBSIZE) * y / x;
            x = config->getIntOption(CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_THUMBSIZE);
            std::string resolution = fmt::format("{}x{}", x, y);
            ffres->addAttribute(R_RESOLUTION, resolution);
            item->addResource(move(ffres));
            log_debug("Adding resource for video thumbnail");
        }
    }
#endif // FFMPEGTHUMBNAILER
}

// Stub for suppressing ffmpeg error messages during matadata extraction
static void FfmpegNoOutputStub(void* ptr, int level, const char* fmt, va_list vl)
{
    // do nothing
}

void FfmpegHandler::fillMetadata(std::shared_ptr<CdsObject> obj)
{
    auto item = std::dynamic_pointer_cast<CdsItem>(obj);
    if (!item)
        return;

    log_debug("Running ffmpeg handler on {}", item->getLocation().c_str());

    AVFormatContext* pFormatCtx = nullptr;

    // Suppress all log messages
    av_log_set_callback(FfmpegNoOutputStub);

#if (LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(58, 9, 100))
    // Register all formats and codecs
    av_register_all();
#endif
    // Open video file
    if (avformat_open_input(&pFormatCtx,
            item->getLocation().c_str(), nullptr, nullptr)
        != 0)
        return; // Couldn't open file

    // Retrieve stream information
    if (avformat_find_stream_info(pFormatCtx, nullptr) < 0) {
        avformat_close_input(&pFormatCtx);
        return; // Couldn't find stream information
    }
    // Add metadata using ffmpeg library calls
    addFfmpegMetadataFields(item, pFormatCtx);
    // Add auxdata
    addFfmpegAuxdataFields(item, pFormatCtx);
    // Add resources using ffmpeg library calls
    addFfmpegResourceFields(item, pFormatCtx);

    // Close the video file
    avformat_close_input(&pFormatCtx);
}

#ifdef HAVE_FFMPEGTHUMBNAILER

fs::path getThumbnailCacheBasePath(const Config& config)
{
    if (auto configuredDir = config.getOption(CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_CACHE_DIR);
        !configuredDir.empty()) {
        return fs::path(std::move(configuredDir));
    }
    auto home = config.getOption(CFG_SERVER_HOME);
    return fs::path(std::move(home)) / "cache-dir";
}

fs::path getThumbnailCachePath(const fs::path& base, const fs::path& movie)
{
    assert(movie.is_absolute());

    auto path = base / movie.relative_path();
    path += "-thumb.jpg";
    return path;
}

std::optional<std::vector<std::byte>> FfmpegHandler::readThumbnailCacheFile(const fs::path& movie_filename) const
{
    auto path = getThumbnailCachePath(getThumbnailCacheBasePath(*config), movie_filename);
    return readBinaryFile(path);
}

void FfmpegHandler::writeThumbnailCacheFile(const fs::path& movie_filename, const std::byte* data, std::size_t size) const
{
    try {
        auto path = getThumbnailCachePath(getThumbnailCacheBasePath(*config), movie_filename);
        fs::create_directories(path.parent_path());
        writeBinaryFile(path, data, size);
    } catch (const std::runtime_error& e) {
        log_error("Failed to write thumbnail cache: {}", e.what());
    }
}
#endif

std::unique_ptr<IOHandler> FfmpegHandler::serveContent(std::shared_ptr<CdsObject> obj, int resNum)
{
#ifdef HAVE_FFMPEGTHUMBNAILER
    auto item = std::dynamic_pointer_cast<CdsItem>(obj);
    if (!item)
        return nullptr;

    if (!config->getBoolOption(CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_ENABLED))
        return nullptr;

    const auto cache_enabled = config->getBoolOption(CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_CACHE_DIR_ENABLED);

    if (cache_enabled) {
        if (auto data = readThumbnailCacheFile(item->getLocation())) {
            log_debug("Returning cached thumbnail for file: {}", item->getLocation().c_str());
            return std::make_unique<MemIOHandler>(data->data(), data->size());
        }
    }

    auto thumb_lock = std::scoped_lock<std::mutex>(thumb_mutex);

    auto th = ffmpegthumbnailer::VideoThumbnailer(config->getIntOption(CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_THUMBSIZE), false, true, config->getIntOption(CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_IMAGE_QUALITY), false);
    std::vector<uint8_t> img;

    th.setSeekPercentage(config->getIntOption(CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_SEEK_PERCENTAGE));
    if (config->getBoolOption(CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_FILMSTRIP_OVERLAY))
        th.addFilter(new ffmpegthumbnailer::FilmStripFilter());

    log_debug("Generating thumbnail for file: {}", item->getLocation().c_str());

    th.generateThumbnail(item->getLocation().c_str(), Jpeg, img);
    if (cache_enabled) {
        writeThumbnailCacheFile(item->getLocation(),
            reinterpret_cast<std::byte*>(img.data()), img.size());
    }

    return std::make_unique<MemIOHandler>(img.data(), img.size());
#else
    return nullptr;
#endif
}

std::string FfmpegHandler::getMimeType()
{
    auto mappings = config->getDictionaryOption(CFG_IMPORT_MAPPINGS_MIMETYPE_TO_CONTENTTYPE_LIST);
    return getValueOrDefault(mappings, CONTENT_TYPE_JPG, "image/jpeg");
}
#endif // HAVE_FFMPEG
