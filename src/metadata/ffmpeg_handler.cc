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
/// \brief Implementeation of the FfmpegHandler class.

// Information about the stream are to be found in the structure
// AVFormatContext, defined in libavformat/avformat.h:335
// and in the structure
// AVCodecContext, defined in libavcodec/avcodec.h:722
// in the ffmpeg sources

#ifdef HAVE_FFMPEG

// ffmpeg needs the following sources
// INT64_C is not defined in ffmpeg/avformat.h but is needed
// macro defines included via autoconfig.h
#include <cinttypes>
#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>

extern "C" {

#include <libavformat/avformat.h>
#include <libavutil/avutil.h>

} // extern "C"

#ifdef HAVE_FFMPEGTHUMBNAILER
#include "iohandler/mem_io_handler.h"
#include <libffmpegthumbnailer/videothumbnailerc.h>
#endif

#include "config/config_manager.h"
#include "ffmpeg_handler.h"
#include "util/string_converter.h"

#ifdef HAVE_AVSTREAM_CODECPAR
#define as_codecpar(s) s->codecpar
#else
#define as_codecpar(s) s->codec
#endif

// Default constructor
FfmpegHandler::FfmpegHandler(std::shared_ptr<ConfigManager> config)
    : MetadataHandler(config)
{
}

void FfmpegHandler::addFfmpegAuxdataFields(std::shared_ptr<CdsItem> item, AVFormatContext* pFormatCtx) const
{
    if (!pFormatCtx->metadata) {
        log_debug("no metadata");
        return;
    }

    auto sc = StringConverter::m2i(config);
    std::vector<std::string> aux = config->getStringArrayOption(CFG_IMPORT_LIBOPTS_FFMPEG_AUXDATA_TAGS_LIST);
    for (size_t j = 0; j < aux.size(); j++) {
        std::string desiredTag(aux[j]);
        if (string_ok(desiredTag)) {
            AVDictionaryEntry* tag = NULL;
            tag = av_dict_get(pFormatCtx->metadata, desiredTag.c_str(), NULL, AV_DICT_IGNORE_SUFFIX);
            if (tag && tag->value && tag->value[0]) {
                log_debug("Added {}: {}", desiredTag.c_str(), tag->value);
                item->setAuxData(desiredTag, sc->convert(tag->value));
            }
        }
    }
} //addFfmpegAuxdataFields

void FfmpegHandler::addFfmpegMetadataFields(std::shared_ptr<CdsItem> item, AVFormatContext* pFormatCtx) const
{
    AVDictionaryEntry* e = NULL;
    auto sc = StringConverter::m2i(config);
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
            if ((value.length() == 4) && (std::stoi(value) > 0)) {
                value = value + "-01-01";
                log_debug("Identified metadata date: {}", value.c_str());
            }
            /// \toto parse possible ISO8601 timestamp
            field = M_DATE;
        } else if (strcmp(e->key, "genre") == 0) {
            log_debug("Identified metadata genre: {}", e->value);
            field = M_GENRE;
        } else if (strcmp(e->key, "description") == 0) {
            log_debug("Identified metadata description: {}", e->value);
            field = M_DESCRIPTION;
        } else if (strcmp(e->key, "track") == 0) {
            log_debug("Identified metadata track: {}", e->value);
            field = M_TRACKNUMBER;
        } else {
            continue;
        }

        item->setMetadata(MT_KEYS[field].upnp, sc->convert(trim_string(value)));
    }
}

// ffmpeg library calls
static void addFfmpegResourceFields(std::shared_ptr<CdsItem> item, AVFormatContext* pFormatCtx)
{
    int64_t hours, mins, secs, us;
    int audioch = 0, samplefreq = 0;
    bool audioset, videoset;
    std::string resolution;
    char duration[15];

    // Initialize the buffers
    duration[0] = 0;

    // duration
    secs = pFormatCtx->duration / AV_TIME_BASE;
    us = pFormatCtx->duration % AV_TIME_BASE;
    mins = secs / 60;
    secs %= 60;
    hours = mins / 60;
    mins %= 60;
    if ((hours + mins + secs) > 0) {
        sprintf(duration, "%02" PRId64 ":%02" PRId64 ":%02" PRId64 ".%01" PRId64, hours, mins, secs, (10 * us) / AV_TIME_BASE);
        log_debug("Added duration: {}", duration);
        item->getResource(0)->addAttribute(MetadataHandler::getResAttrName(R_DURATION), duration);
    }

    // bitrate
    if (pFormatCtx->bit_rate > 0) {
        // ffmpeg's bit_rate is in bits/sec, upnp wants it in bytes/sec
        // See http://www.upnp.org/schemas/av/didl-lite-v3.xsd
        log_debug("Added overall bitrate: {} kb/s", pFormatCtx->bit_rate / 8);
        item->getResource(0)->addAttribute(MetadataHandler::getResAttrName(R_BITRATE), std::to_string(pFormatCtx->bit_rate / 8));
    }

    // video resolution, audio sampling rate, nr of audio channels
    audioset = false;
    videoset = false;
    for (unsigned int i = 0; i < pFormatCtx->nb_streams; i++) {
        AVStream* st = pFormatCtx->streams[i];
        if ((st != NULL) && (videoset == false) && (as_codecpar(st)->codec_type == AVMEDIA_TYPE_VIDEO)) {
            if (as_codecpar(st)->codec_tag > 0) {
                char fourcc[5];
                fourcc[0] = as_codecpar(st)->codec_tag;
                fourcc[1] = as_codecpar(st)->codec_tag >> 8;
                fourcc[2] = as_codecpar(st)->codec_tag >> 16;
                fourcc[3] = as_codecpar(st)->codec_tag >> 24;
                fourcc[4] = '\0';

                log_debug("FourCC: %x = {}",
                    as_codecpar(st)->codec_tag, fourcc);
                std::string fcc = fourcc;
                if (string_ok(fcc))
                    item->getResource(0)->addOption(RESOURCE_OPTION_FOURCC,
                        fcc);
            }

            if ((as_codecpar(st)->width > 0) && (as_codecpar(st)->height > 0)) {
                resolution = std::to_string(as_codecpar(st)->width) + "x" + std::to_string(as_codecpar(st)->height);

                log_debug("Added resolution: {} pixel", resolution.c_str());
                item->getResource(0)->addAttribute(MetadataHandler::getResAttrName(R_RESOLUTION), resolution);
                videoset = true;
            }
        }
        if ((st != NULL) && (audioset == false) && (as_codecpar(st)->codec_type == AVMEDIA_TYPE_AUDIO)) {
            // find the first stream that has a valid sample rate
            if (as_codecpar(st)->sample_rate > 0) {
                samplefreq = as_codecpar(st)->sample_rate;
                log_debug("Added sample frequency: {} Hz", samplefreq);
                item->getResource(0)->addAttribute(MetadataHandler::getResAttrName(R_SAMPLEFREQUENCY), std::to_string(samplefreq));
                audioset = true;

                audioch = as_codecpar(st)->channels;
                if (audioch > 0) {
                    log_debug("Added number of audio channels: {}", audioch);
                    item->getResource(0)->addAttribute(MetadataHandler::getResAttrName(R_NRAUDIOCHANNELS), std::to_string(audioch));
                }
            }
        }
    }
}

// Stub for suppressing ffmpeg error messages during matadata extraction
void FfmpegNoOutputStub(void* ptr, int level, const char* fmt, va_list vl)
{
    // do nothing
}

void FfmpegHandler::fillMetadata(std::shared_ptr<CdsItem> item)
{
    log_debug("Running ffmpeg handler on {}", item->getLocation().c_str());

    AVFormatContext* pFormatCtx = NULL;

    // Suppress all log messages
    av_log_set_callback(FfmpegNoOutputStub);

#if (LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(58, 9, 100))
    // Register all formats and codecs
    av_register_all();
#endif
    // Open video file
    if (avformat_open_input(&pFormatCtx,
            item->getLocation().c_str(), NULL, NULL)
        != 0)
        return; // Couldn't open file

    // Retrieve stream information
    if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
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

// The ffmpegthumbnailer code (ffmpeg?) is not threading safe.
// Add a lock around the usage to avoid crashing randomly.
static pthread_mutex_t thumb_lock;

static int _mkdir(const char* path)
{
    int ret = mkdir(path, 0777);

    if (ret == 0) {
        // Make sure we are +x in case of restrictive umask that strips +x.
        struct stat st;
        if (stat(path, &st)) {
            log_warning("could not stat({}): {}", path, strerror(errno));
            return -1;
        }
        mode_t xbits = S_IXUSR | S_IXGRP | S_IXOTH;
        if (!(st.st_mode & xbits)) {
            if (chmod(path, st.st_mode | xbits)) {
                log_warning("could not chmod({}, +x): {}", path, strerror(errno));
                return -1;
            }
        }
    }

    return ret;
}

static bool makeThumbnailCacheDir(fs::path& path)
{
    char* path_temp = strdup(path.c_str());
    char* last_slash = strrchr(path_temp, '/');
    char* slash = last_slash;
    bool ret = false;

    if (!last_slash)
        return ret;

    // Assume most dirs exist, so scan backwards first.
    // Avoid stat/access checks due to TOCTOU races.
    errno = 0;
    for (slash = last_slash; slash > path_temp; --slash) {
        if (*slash != '/')
            continue;
        *slash = '\0';
        if (_mkdir(path_temp) == 0) {
            // Now we can forward scan.
            while (slash < last_slash) {
                *slash = DIR_SEPARATOR;
                if (_mkdir(path_temp) < 0)
                    // Allow EEXIST in case of someone else doing `mkdir`.
                    if (errno != EEXIST)
                        goto done;
                slash += strlen(slash);
            }
            if (slash == last_slash)
                ret = true;
            break;
        } else if (errno == EEXIST) {
            ret = true;
            break;
        } else if (errno != ENOENT) {
            break;
        }
    }

done:
    free(path_temp);
    return ret;
}

fs::path FfmpegHandler::getThumbnailCacheFilePath(const fs::path& movie_filename, bool create) const
{
    fs::path cache_dir = config->getOption(CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_CACHE_DIR);

    if (cache_dir.empty()) {
        fs::path home_dir = config->getOption(CFG_SERVER_HOME);
        cache_dir = home_dir / "cache-dir";
    }

    cache_dir = cache_dir / (movie_filename.string() + "-thumb.jpg");
    if (create && !makeThumbnailCacheDir(cache_dir))
        cache_dir = "";
    return cache_dir;
}

bool FfmpegHandler::readThumbnailCacheFile(const fs::path& movie_filename, uint8_t** ptr_img, size_t* size_img) const
{
    fs::path path = getThumbnailCacheFilePath(movie_filename, false);
    FILE* fp = fopen(path.c_str(), "rb");
    if (!fp)
        return false;

    size_t bytesRead;
    uint8_t buffer[1024];
    *ptr_img = NULL;
    *size_img = 0;
    while ((bytesRead = fread(buffer, 1, sizeof(buffer), fp)) > 0) {
        *ptr_img = (uint8_t*)realloc(*ptr_img, *size_img + bytesRead);
        memcpy(*ptr_img + *size_img, buffer, bytesRead);
        *size_img += bytesRead;
    }
    fclose(fp);
    return true;
}

void FfmpegHandler::writeThumbnailCacheFile(const fs::path& movie_filename, uint8_t* ptr_img, int size_img) const
{
    fs::path path = getThumbnailCacheFilePath(movie_filename, true);
    FILE* fp = fopen(path.c_str(), "wb");
    if (!fp)
        return;
    fwrite(ptr_img, sizeof(uint8_t), size_img, fp);
    fclose(fp);
}
#endif

std::unique_ptr<IOHandler> FfmpegHandler::serveContent(std::shared_ptr<CdsItem> item, int resNum)
{
#ifdef HAVE_FFMPEGTHUMBNAILER
    if (!config->getBoolOption(CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_ENABLED))
        return nullptr;

    if (config->getBoolOption(CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_CACHE_DIR_ENABLED)) {
        uint8_t* ptr_image;
        size_t size_image;
        if (readThumbnailCacheFile(item->getLocation(), &ptr_image, &size_image)) {
            auto h = std::make_unique<MemIOHandler>(ptr_image, size_image);
            free(ptr_image);
            log_debug("Returning cached thumbnail for file: {}", item->getLocation().c_str());
            return h;
        }
    }

    pthread_mutex_lock(&thumb_lock);

#ifdef FFMPEGTHUMBNAILER_OLD_API
    video_thumbnailer* th = create_thumbnailer();
    image_data* img = create_image_data();
#else
    video_thumbnailer* th = video_thumbnailer_create();
    image_data* img = video_thumbnailer_create_image_data();
#endif // old api

    th->seek_percentage = config->getIntOption(CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_SEEK_PERCENTAGE);

    if (config->getBoolOption(CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_FILMSTRIP_OVERLAY))
        th->overlay_film_strip = 1;
    else
        th->overlay_film_strip = 0;

    th->thumbnail_size = config->getIntOption(CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_THUMBSIZE);
    th->thumbnail_image_quality = config->getIntOption(CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_IMAGE_QUALITY);
    th->thumbnail_image_type = Jpeg;

    log_debug("Generating thumbnail for file: {}", item->getLocation().c_str());

#ifdef FFMPEGTHUMBNAILER_OLD_API
    if (generate_thumbnail_to_buffer(th, item->getLocation().c_str(), img) != 0)
#else
    if (video_thumbnailer_generate_thumbnail_to_buffer(th,
            item->getLocation().c_str(), img)
        != 0)
#endif // old api
    {
        pthread_mutex_unlock(&thumb_lock);
        throw std::runtime_error("Could not generate thumbnail for " + item->getLocation().string());
    }
    if (config->getBoolOption(CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_CACHE_DIR_ENABLED)) {
        writeThumbnailCacheFile(item->getLocation(),
            img->image_data_ptr, img->image_data_size);
    }

    auto h = std::make_unique<MemIOHandler>((void*)img->image_data_ptr, img->image_data_size);
#ifdef FFMPEGTHUMBNAILER_OLD_API
    destroy_image_data(img);
    destroy_thumbnailer(th);
#else
    video_thumbnailer_destroy_image_data(img);
    video_thumbnailer_destroy(th);
#endif // old api
    pthread_mutex_unlock(&thumb_lock);
    return h;
#else
    return nullptr;
#endif
}

std::string FfmpegHandler::getMimeType()
{
    auto mappings = config->getDictionaryOption(CFG_IMPORT_MAPPINGS_MIMETYPE_TO_CONTENTTYPE_LIST);
    std::string thumb_mimetype = getValueOrDefault(mappings, CONTENT_TYPE_JPG);
    if (!string_ok(thumb_mimetype))
        thumb_mimetype = "image/jpeg";

    return thumb_mimetype;
}
#endif // HAVE_FFMPEG
