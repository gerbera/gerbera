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

#ifdef HAVE_CONFIG_H
    #include "autoconfig.h"
#endif

#ifdef HAVE_FFMPEG

// ffmpeg needs the following sources
// INT64_C is not defined in ffmpeg/avformat.h but is needed
// macro defines included via autoconfig.h
#include <stdint.h>

//#ifdef FFMPEG_NEEDS_EXTERN_C
extern "C" 
{
//#endif

#include AVFORMAT_INCLUDE

//#ifdef FFMPEG_NEEDS_EXTERN_C
} // extern "C"
//#endif

#ifdef HAVE_FFMPEGTHUMBNAILER
    #include <libffmpegthumbnailer/videothumbnailerc.h>
#endif

#include "config_manager.h"
#include "ffmpeg_handler.h"
#include "string_converter.h"
#include "common.h"
#include "tools.h"
#include "rexp.h"
//#include "mxml/mxml.h"
#include "mem_io_handler.h"


using namespace zmm;
//using namespace mxml;

// Default constructor
FfmpegHandler::FfmpegHandler() : MetadataHandler()
{
}

static void addFfmpegMetadataFields(Ref<CdsItem> item, AVFormatContext *pFormatCtx) 
{

	Ref<StringConverter> sc = StringConverter::m2i();
    
	if (strlen(pFormatCtx->title) > 0) 
    {
	    log_debug("Added metadata title: %s\n", pFormatCtx->title);
        item->setMetadata(MT_KEYS[M_TITLE].upnp, 
                          sc->convert(pFormatCtx->title));
	}
	if (strlen(pFormatCtx->author) > 0) 
    {
	    log_debug("Added metadata author: %s\n", pFormatCtx->author);
        item->setMetadata(MT_KEYS[M_ARTIST].upnp, 
                          sc->convert(pFormatCtx->author));
	}
	if (strlen(pFormatCtx->album) > 0) 
    {
	    log_debug("Added metadata album: %s\n", pFormatCtx->album);
        item->setMetadata(MT_KEYS[M_ALBUM].upnp, 
                          sc->convert(pFormatCtx->album));
	}
	if (pFormatCtx->year > 0) 
    {
	    log_debug("Added metadata year: %d\n", pFormatCtx->year);
        item->setMetadata(MT_KEYS[M_DATE].upnp, 
                          sc->convert(String::from(pFormatCtx->year)));
	}
	if (strlen(pFormatCtx->genre) > 0) 
    {
	    log_debug("Added metadata genre: %s\n", pFormatCtx->genre);
        item->setMetadata(MT_KEYS[M_GENRE].upnp, 
                          sc->convert(pFormatCtx->genre));
	}
	if (strlen(pFormatCtx->comment) > 0) 
    {
	    log_debug("Added metadata comment: %s\n", pFormatCtx->comment);
        item->setMetadata(MT_KEYS[M_DESCRIPTION].upnp, 
                          sc->convert(pFormatCtx->comment));
	}
	if (pFormatCtx->track > 0) 
    {
	    log_debug("Added metadata track: %d\n", pFormatCtx->track);
        item->setMetadata(MT_KEYS[M_TRACKNUMBER].upnp, 
                          sc->convert(String::from(pFormatCtx->track)));
	}
}

// ffmpeg library calls
static void addFfmpegResourceFields(Ref<CdsItem> item, AVFormatContext *pFormatCtx, int *x, int *y) 
{
    unsigned int i;
    int hours, mins, secs, us;
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
    if ((hours + mins + secs) > 0) 
    { 
    	sprintf(duration, "%02d:%02d:%02d.%01d", hours, mins,
                secs, (10 * us) / AV_TIME_BASE);
        log_debug("Added duration: %s\n", duration);
        item->getResource(0)->addAttribute(MetadataHandler::getResAttrName(R_DURATION), duration);
    }

	// bitrate
    if (pFormatCtx->bit_rate > 0)  
    {
        log_debug("Added overall bitrate: %d kb/s\n", 
                  pFormatCtx->bit_rate/1000);
        item->getResource(0)->addAttribute(MetadataHandler::getResAttrName(R_BITRATE), String::from(pFormatCtx->bit_rate/1000));
    }

	// video resolution, audio sampling rate, nr of audio channels
	audioset = false;
	videoset = false;
	for(i=0; i<pFormatCtx->nb_streams; i++) 
    {
		AVStream *st = pFormatCtx->streams[i];
		if((st != NULL) && (videoset == false) && (st->codec->codec_type == CODEC_TYPE_VIDEO))
        {
            if (st->codec->codec_tag > 0)
            {
                char fourcc[5];
                fourcc[0] = st->codec->codec_tag;
                fourcc[1] = st->codec->codec_tag >> 8;
                fourcc[2] = st->codec->codec_tag >> 16;
                fourcc[3] = st->codec->codec_tag >> 24;
                fourcc[4] = '\0';

                log_debug("FourCC: %x = %s\n", 
                                        st->codec->codec_tag, fourcc);
                String fcc = fourcc;
                if (string_ok(fcc))
                    item->getResource(0)->addOption(_(RESOURCE_OPTION_FOURCC), 
                                                    fcc);
            }

			if ((st->codec->width > 0) && (st->codec->height > 0)) 
            {
                resolution = String::from(st->codec->width) + "x" + 
                                    String::from(st->codec->height);

				log_debug("Added resolution: %s pixel\n", resolution.c_str());
		        item->getResource(0)->addAttribute(MetadataHandler::getResAttrName(R_RESOLUTION), resolution);
				videoset = true;
                *x = st->codec->width;
                *y = st->codec->height;
			}
		} 
		if(st->codec->codec_type == CODEC_TYPE_AUDIO) 
        {
			// Increase number of audiochannels
			audioch++;
			// Get the sample rate
			if ((audioset == false) && (st->codec->sample_rate > 0)) 
            {
				samplefreq = st->codec->sample_rate;
			    if (samplefreq > 0) 
                {
		    	    log_debug("Added sample frequency: %d Hz\n", samplefreq);
		        	item->getResource(0)->addAttribute(MetadataHandler::getResAttrName(R_SAMPLEFREQUENCY), String::from(samplefreq));
					audioset = true;
    			}
			}
		}
	}

    if (audioch > 0) 
    {
        log_debug("Added number of audio channels: %d\n", audioch);
        item->getResource(0)->addAttribute(MetadataHandler::getResAttrName(R_NRAUDIOCHANNELS), String::from(audioch));
    }
} // addFfmpegResourceFields

/*double time_to_double(struct timeval time) {
        return time.tv_sec + (time.tv_usec / 1000000.0);
}*/

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

	AVFormatContext *pFormatCtx;
	
	// Suppress all log messages
	av_log_set_callback(FfmpegNoOutputStub);
	
	// Register all formats and codecs
    av_register_all();

    // Open video file
    if (av_open_input_file(&pFormatCtx, 
                          item->getLocation().c_str(), NULL, 0, NULL) != 0)
        return; // Couldn't open file

    // Retrieve stream information
    if (av_find_stream_info(pFormatCtx) < 0)
    {
        av_close_input_file(pFormatCtx);
        return; // Couldn't find stream information
    }   
	// Add metadata using ffmpeg library calls
	addFfmpegMetadataFields(item, pFormatCtx);
	// Add resources using ffmpeg library calls
	addFfmpegResourceFields(item, pFormatCtx, &x, &y);
	
    // Close the video file
    av_close_input_file(pFormatCtx);
}

Ref<IOHandler> FfmpegHandler::serveContent(Ref<CdsItem> item, int resNum, off_t *data_size)
{
    *data_size = -1;
#ifdef HAVE_FFMPEGTHUMBNAILER
    Ref<ConfigManager> cfg = ConfigManager::getInstance();

    if (!cfg->getBoolOption(CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_ENABLED))
        return nil;

#ifdef FFMPEGTHUMBNAILER_OLD_API
    video_thumbnailer *th = create_thumbnailer();
    image_data *img = create_image_data();
#else
    video_thumbnailer *th = video_thumbnailer_create();
    image_data *img = video_thumbnailer_create_image_data();
#endif // old api


    th->seek_percentage        = cfg->getIntOption(CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_SEEK_PERCENTAGE);

    if (cfg->getBoolOption(CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_FILMSTRIP_OVERLAY))
        th->overlay_film_strip = 1;
    else
        th->overlay_film_strip = 0;

    th->thumbnail_size = cfg->getIntOption(CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_THUMBSIZE);
    th->thumbnail_image_quality = cfg->getIntOption(CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_IMAGE_QUALITY);
    th->thumbnail_image_type   = Jpeg;

    log_debug("Generating thumbnail for file: %s\n", item->getLocation().c_str());

#ifdef FFMPEGTHUMBNAILER_OLD_API
    if (generate_thumbnail_to_buffer(th, item->getLocation().c_str(), img) != 0)
#else
    if (video_thumbnailer_generate_thumbnail_to_buffer(th, 
                                         item->getLocation().c_str(), img) != 0)
#endif // old api
        throw _Exception(_("Could not generate thumbnail for ") + 
                item->getLocation());

    *data_size = (off_t)img->image_data_size;
    Ref<IOHandler> h(new MemIOHandler((void *)img->image_data_ptr, 
                                              img->image_data_size));
#ifdef FFMPEGTHUMBNAILER_OLD_API
    destroy_image_data(img);
    destroy_thumbnailer(th);
#else
    video_thumbnailer_destroy_image_data(img);
    video_thumbnailer_destroy(th);
#endif// old api
    return h;
#else
    return nil;
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
