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
#include <libffmpegthumbnailer/videothumbnailerc.h>
#include "mem_io_handler.h"
#endif

#include "config_manager.h"
#include "ffmpeg_handler.h"
#include "string_converter.h"

#ifdef HAVE_AVSTREAM_CODECPAR
#define as_codecpar(s) s->codecpar
#else
#define as_codecpar(s) s->codec
#endif

using namespace zmm;

// Default constructor
FfmpegHandler::FfmpegHandler()
    : MetadataHandler()
{
}

static void addFfmpegMetadataFields(Ref<CdsItem> item, AVFormatContext* pFormatCtx)
{
    AVDictionaryEntry* e = NULL;
    Ref<StringConverter> sc = StringConverter::m2i();
    metadata_fields_t field;
    String value;

    while ((e = av_dict_get(pFormatCtx->metadata, "", e, AV_DICT_IGNORE_SUFFIX))) {
        value = e->value;

        if (strcmp(e->key, "title") == 0) {
            log_debug("Identified metadata title: %s\n", e->value);
            field = M_TITLE;
        } else if (strcmp(e->key, "artist") == 0) {
            log_debug("Identified metadata artist: %s\n", e->value);
            field = M_ARTIST;
        } else if (strcmp(e->key, "album") == 0) {
            log_debug("Identified metadata album: %s\n", e->value);
            field = M_ALBUM;
        } else if (strcmp(e->key, "date") == 0) {
            if ((value.length() == 4) && (value.toInt() > 0)) {
                value = value + _("-01-01");
                log_debug("Identified metadata date: %s\n", value.c_str());
            }
            /// \toto parse possible ISO8601 timestamp
            field = M_DATE;
        } else if (strcmp(e->key, "genre") == 0) {
            log_debug("Identified metadata genre: %s\n", e->value);
            field = M_GENRE;
        } else if (strcmp(e->key, "comment") == 0) {
            log_debug("Identified metadata comment: %s\n", e->value);
            field = M_DESCRIPTION;
        } else if (strcmp(e->key, "track") == 0) {
            log_debug("Identified metadata track: %d\n", e->value);
            field = M_TRACKNUMBER;
        } else {
            continue;
        }

        item->setMetadata(MT_KEYS[field].upnp, sc->convert(trim_string(value)));
    }
}

// ffmpeg library calls
static void addFfmpegResourceFields(Ref<CdsItem> item, AVFormatContext* pFormatCtx, int* x, int* y)
{
    int64_t hours, mins, secs, us;
    int audioch = 0, samplefreq = 0;
    bool audioset, videoset;
    String resolution;
    char duration[15];

    // Initialize the buffers
    duration[0] = 0;

    *x = 0;
    *y = 0;

    // duration
    secs = pFormatCtx->duration / AV_TIME_BASE;
    us = pFormatCtx->duration % AV_TIME_BASE;
    mins = secs / 60;
    secs %= 60;
    hours = mins / 60;
    mins %= 60;
    if ((hours + mins + secs) > 0) {
        sprintf(duration, "%02" PRId64 ":%02" PRId64 ":%02" PRId64 ".%01" PRId64, hours, mins, secs, (10 * us) / AV_TIME_BASE);
        log_debug("Added duration: %s\n", duration);
        item->getResource(0)->addAttribute(MetadataHandler::getResAttrName(R_DURATION), duration);
    }

    // bitrate
    if (pFormatCtx->bit_rate > 0) {
        // ffmpeg's bit_rate is in bits/sec, upnp wants it in bytes/sec
        // See http://www.upnp.org/schemas/av/didl-lite-v3.xsd
        log_debug("Added overall bitrate: %d kb/s\n", pFormatCtx->bit_rate / 8);
        item->getResource(0)->addAttribute(MetadataHandler::getResAttrName(R_BITRATE), String::from(pFormatCtx->bit_rate / 8));
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

                log_debug("FourCC: %x = %s\n",
                    as_codecpar(st)->codec_tag, fourcc);
                String fcc = fourcc;
                if (string_ok(fcc))
                    item->getResource(0)->addOption(_(RESOURCE_OPTION_FOURCC),
                        fcc);
            }

            if ((as_codecpar(st)->width > 0) && (as_codecpar(st)->height > 0)) {
                resolution = String::from(as_codecpar(st)->width) + "x" + String::from(as_codecpar(st)->height);

                log_debug("Added resolution: %s pixel\n", resolution.c_str());
                item->getResource(0)->addAttribute(MetadataHandler::getResAttrName(R_RESOLUTION), resolution);
                videoset = true;
                *x = as_codecpar(st)->width;
                *y = as_codecpar(st)->height;
            }
        }
        if ((st != NULL) && (audioset == false) && (as_codecpar(st)->codec_type == AVMEDIA_TYPE_AUDIO)) {
            // find the first stream that has a valid sample rate
            if (as_codecpar(st)->sample_rate > 0) {
                samplefreq = as_codecpar(st)->sample_rate;
                log_debug("Added sample frequency: %d Hz\n", samplefreq);
                item->getResource(0)->addAttribute(MetadataHandler::getResAttrName(R_SAMPLEFREQUENCY), String::from(samplefreq));
                audioset = true;

                audioch = as_codecpar(st)->channels;
                if (audioch > 0) {
                    log_debug("Added number of audio channels: %d\n", audioch);
                    item->getResource(0)->addAttribute(MetadataHandler::getResAttrName(R_NRAUDIOCHANNELS), String::from(audioch));
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

void FfmpegHandler::fillMetadata(Ref<CdsItem> item)
{
    log_debug("Running ffmpeg handler on %s\n", item->getLocation().c_str());

    int x = 0;
    int y = 0;

    AVFormatContext* pFormatCtx = NULL;

    // Suppress all log messages
    av_log_set_callback(FfmpegNoOutputStub);

    // Register all formats and codecs
    av_register_all();

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
    // Add resources using ffmpeg library calls
    addFfmpegResourceFields(item, pFormatCtx, &x, &y);

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
            log_warning("could not stat(%s): %s\n", path, strerror(errno));
            return -1;
        }
        mode_t xbits = S_IXUSR | S_IXGRP | S_IXOTH;
        if (!(st.st_mode & xbits)) {
            if (chmod(path, st.st_mode | xbits)) {
                log_warning("could not chmod(%s, +x): %s\n", path, strerror(errno));
                return -1;
            }
        }
    }

    return ret;
}

static bool makeThumbnailCacheDir(String& path)
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

static String getThumbnailCacheFilePath(String& movie_filename, bool create)
{
    Ref<ConfigManager> cfg = ConfigManager::getInstance();
    String cache_dir = cfg->getOption(CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_CACHE_DIR);

    if (cache_dir.length() == 0) {
        String home_dir = cfg->getOption(CFG_SERVER_HOME);
        cache_dir = home_dir + "/cache-dir";
    }

    cache_dir = cache_dir + movie_filename + "-thumb.jpg";
    if (create && !makeThumbnailCacheDir(cache_dir))
        cache_dir = "";
    return cache_dir;
}

static bool readThumbnailCacheFile(String movie_filename, uint8_t** ptr_img, size_t* size_img)
{
    String path = getThumbnailCacheFilePath(movie_filename, false);
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

static void writeThumbnailCacheFile(String movie_filename, uint8_t* ptr_img, int size_img)
{
    String path = getThumbnailCacheFilePath(movie_filename, true);
    FILE* fp = fopen(path.c_str(), "wb");
    if (!fp)
        return;
    fwrite(ptr_img, sizeof(uint8_t), size_img, fp);
    fclose(fp);
}

#endif

Ref<IOHandler> FfmpegHandler::serveContent(Ref<CdsItem> item, int resNum, off_t* data_size)
{
    *data_size = -1;
#ifdef HAVE_FFMPEGTHUMBNAILER
    Ref<ConfigManager> cfg = ConfigManager::getInstance();

    if (!cfg->getBoolOption(CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_ENABLED))
        return nullptr;

    if (cfg->getBoolOption(CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_CACHE_DIR_ENABLED)) {
        uint8_t* ptr_image;
        size_t size_image;
        if (readThumbnailCacheFile(item->getLocation(),
                &ptr_image, &size_image)) {
            *data_size = (off_t)size_image;
            Ref<IOHandler> h(new MemIOHandler(ptr_image, size_image));
            free(ptr_image);
            log_debug("Returning cached thumbnail for file: %s\n", item->getLocation().c_str());
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

    th->seek_percentage = cfg->getIntOption(CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_SEEK_PERCENTAGE);

    if (cfg->getBoolOption(CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_FILMSTRIP_OVERLAY))
        th->overlay_film_strip = 1;
    else
        th->overlay_film_strip = 0;

    th->thumbnail_size = cfg->getIntOption(CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_THUMBSIZE);
    th->thumbnail_image_quality = cfg->getIntOption(CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_IMAGE_QUALITY);
    th->thumbnail_image_type = Jpeg;

    log_debug("Generating thumbnail for file: %s\n", item->getLocation().c_str());

#ifdef FFMPEGTHUMBNAILER_OLD_API
    if (generate_thumbnail_to_buffer(th, item->getLocation().c_str(), img) != 0)
#else
    if (video_thumbnailer_generate_thumbnail_to_buffer(th,
            item->getLocation().c_str(), img)
        != 0)
#endif // old api
    {
        pthread_mutex_unlock(&thumb_lock);
        throw _Exception(_("Could not generate thumbnail for ") + item->getLocation());
    }
    if (cfg->getBoolOption(CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_CACHE_DIR_ENABLED)) {
        writeThumbnailCacheFile(item->getLocation(),
            img->image_data_ptr, img->image_data_size);
    }

    *data_size = (off_t)img->image_data_size;
    Ref<IOHandler> h(new MemIOHandler((void*)img->image_data_ptr,
        img->image_data_size));
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

String FfmpegHandler::getMimeType()
{
    Ref<ConfigManager> cfg = ConfigManager::getInstance();

    Ref<Dictionary> mappings = cfg->getDictionaryOption(CFG_IMPORT_MAPPINGS_MIMETYPE_TO_CONTENTTYPE_LIST);
    String thumb_mimetype = mappings->get(_(CONTENT_TYPE_JPG));
    if (!string_ok(thumb_mimetype))
        thumb_mimetype = _("image/jpeg");

    return thumb_mimetype;
}
#endif // HAVE_FFMPEG
