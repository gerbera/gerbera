/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    dvd_handler.cc - this file is part of MediaTomb.
    
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

/// \file dvd_handler.cc
/// \brief Implementeation of the DVDHandler class.

#ifdef HAVE_LIBDVDNAV

#include "dvd_handler.h"
#include "string_converter.h"
#include "common.h"
#include "tools.h"
#include "content_manager.h"
#include "config_manager.h"

#include "dvdnav_read.h"

using namespace zmm;

#define DVD                         "dvd"
#define DVD_TITLE                   DVD "t"
#define DVD_CHAPTER                 "ch"
#define DVD_AUDIO_TRACK             "at"
#define DVD_DURATION                "d"
#define DVD_REST_DURATION           "r"
#define DVD_FORMAT                  "f"
#define DVD_STREAM_ID               "i"
#define DVD_COUNT                   "c"
#define DVD_CHANNELS                "l"
#define DVD_SAMPLE_FREQUENCY        "q"
#define DVD_LANGUAGE                "g"

#define DVD_MIMETYPE                DVD "m"

DVDHandler::DVDHandler() : MetadataHandler()
{
}

String DVDHandler::renderKey(dvd_aux_key_names_t name, int title_idx, 
                             int chapter_idx, int audio_track_idx)
{
    String key;

    switch (name)
    {
        case DVD_Title:
            key = _(DVD_TITLE);
            break;
        case DVD_Chapter:
            key = _(DVD DVD_CHAPTER);
            break;
        case DVD_AudioTrack:
            key = _(DVD DVD_AUDIO_TRACK);
            break;
        case DVD_AudioStreamID:
            key = _(DVD DVD_STREAM_ID);
            break;
        case DVD_TitleCount:
            key = _(DVD_TITLE) + _(DVD_COUNT);
            break;
        case DVD_TitleDuration:
            key = _(DVD_TITLE) + title_idx + _(DVD_DURATION);
            break;
        case DVD_ChapterCount:
            key = _(DVD_TITLE) + title_idx + _(DVD_CHAPTER) + chapter_idx + 
                  _(DVD_COUNT);
            break;
        case DVD_ChapterDuration:
            key = _(DVD_TITLE) + title_idx + _(DVD_CHAPTER) + chapter_idx + 
                  _(DVD_DURATION);
        case DVD_ChapterRestDuration:
            key = _(DVD_TITLE) + title_idx + _(DVD_CHAPTER) + chapter_idx +
                  _(DVD_REST_DURATION);
            break;
        case DVD_AudioTrackCount:
            key = _(DVD_TITLE) + title_idx + _(DVD_AUDIO_TRACK) + _(DVD_COUNT);
            break;
        case DVD_AudioTrackFormat:
            key = _(DVD_TITLE) + title_idx + _(DVD_AUDIO_TRACK) + 
                   audio_track_idx + _(DVD_FORMAT);
            break;
        case DVD_AudioTrackStreamID:
            key = _(DVD_TITLE) + title_idx + _(DVD_AUDIO_TRACK) + 
                   audio_track_idx + _(DVD_STREAM_ID);
            break;
        case DVD_AudioTrackChannels:
            key = _(DVD_TITLE) + title_idx + _(DVD_AUDIO_TRACK) +
                  audio_track_idx + _(DVD_CHANNELS);
            break;
        case DVD_AudioTrackSampleFreq:
            key = _(DVD_TITLE) + title_idx + _(DVD_AUDIO_TRACK) +
                  audio_track_idx + _(DVD_SAMPLE_FREQUENCY);
            break;
        case DVD_AudioTrackLanguage:
            key = _(DVD_TITLE) + title_idx + _(DVD_AUDIO_TRACK) +
                  audio_track_idx + _(DVD_LANGUAGE);
            break;
        default:
            throw _Exception(_("Invalid dvd aux key name!"));
    }

    return key;
}

void DVDHandler::fillMetadata(Ref<CdsItem> item)
{
    try
    {
        Ref<DVDNavReader> dvd(new DVDNavReader(item->getLocation()));

        item->setFlag(OBJECT_FLAG_DVD_IMAGE);

        int titles = dvd->titleCount();
        item->setAuxData(renderKey(DVD_TitleCount), String::from(titles));

        for (int i = 0; i < titles; i++)
        {
            dvd->selectPGC(i, 0);
            item->setAuxData(renderKey(DVD_ChapterCount, i), 
                    String::from(dvd->chapterCount(i)));
//            if (dvd->titleDuration() > 0)
//                item->setAuxData(renderKey(DVD_TitleDuration, i), 
//                        secondsToHMS(dvd->titleDuration()));
            item->setAuxData(renderKey(DVD_AudioTrackCount, i), 
                    String::from(dvd->audioTrackCount()));

            for (int a = 0; a < dvd->audioTrackCount(); a++)
            {
                item->setAuxData(renderKey(DVD_AudioTrackFormat, i, 0, a),
                        dvd->audioFormat(a));
                item->setAuxData(renderKey(DVD_AudioTrackStreamID, i, 0, a),
                        String::from(dvd->audioStreamID(a)));
                item->setAuxData(renderKey(DVD_AudioTrackChannels, i, 0, a),
                        String::from(dvd->audioChannels(a)));
                item->setAuxData(renderKey(DVD_AudioTrackSampleFreq, i, 0, a),
                        String::from(dvd->audioSampleFrequency(a)));
                item->setAuxData(renderKey(DVD_AudioTrackLanguage, i, 0, a),
                        dvd->audioLanguage(a));
            }
#if 0
            int secs = 0;
            for (int c = dvd->chapterCount(i)-1; c >= 0; c--)
            {
                int chap_secs = dvd->chapterDuration(c);
                secs = secs + chap_secs;
                log_debug("Chapter seconds: %d rest seconds: %d\n", chap_secs,
                        secs);
                if (chap_secs > 0)
                    item->setAuxData(renderKey(DVD_ChapterDuration, i, c), 
                            secondsToHMS(chap_secs));
                if (secs > 0)
                    item->setAuxData(renderKey(DVD_ChapterRestDuration, i, c), 
                            secondsToHMS(secs));
            }
#endif
        } // for titles

        log_debug("DVD image %s has %d titles\n", item->getLocation().c_str(), 
                titles);
    }
    catch (Exception ex)
    {
        log_warning("Parsing ISO image failed (not a DVD?): %s\n", ex.getMessage().c_str());
    }
}



Ref<IOHandler> DVDHandler::serveContent(Ref<CdsItem> item, int resNum, off_t *data_size)
{
    *data_size = -1;
    return nil;
}

#endif // HAVE_DVD
