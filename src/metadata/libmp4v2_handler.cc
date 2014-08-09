/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    libmp4v2_handler.cc - this file is part of MediaTomb.
    
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

/// \file libmp4v2_handler.cc
/// \brief Implementeation of the LibMP4V2Handler class.

#ifdef HAVE_CONFIG_H
    #include "autoconfig.h"
#endif

#ifdef HAVE_LIBMP4V2

#include "libmp4v2_handler.h"
#include "string_converter.h"
#include "common.h"
#include "tools.h"
#include "mem_io_handler.h"
#include "content_manager.h"
#include "config_manager.h"

// why does crap like that alsways happens with some Ubuntu package?
#undef PACKAGE
#undef PACKAGE_BUGREPORT
#undef PACKAGE_NAME
#undef PACKAGE_STRING
#undef PACKAGE_TARNAME
#undef VERSION
#undef TRUE
#undef FALSE

#include LIBMP4V2_INCLUDE

using namespace zmm;

LibMP4V2Handler::LibMP4V2Handler() : MetadataHandler()
{
}

static void addMetaField(metadata_fields_t field, MP4FileHandle mp4, Ref<CdsItem> item)
{
    String value;
    Ref<StringConverter> sc = StringConverter::i2i();
    
    const MP4Tags* new_tags = MP4TagsAlloc();

    if (!MP4TagsFetch(new_tags, mp4))
        return;

    switch (field)
    {
        case M_TITLE:
            value = new_tags->name;
            break;
        case M_ARTIST:
            value = new_tags->artist;
            break;
        case M_ALBUM:
            value = new_tags->album;
            break;
        case M_DATE:
            value = new_tags->releaseDate;
            if (value.length() > 0)
            {
                if (string_ok(value))
                    value = value + "-01-01";
                else
                    return;
            }
            break;
        case M_GENRE:
            value = new_tags->genre;
            break;
        case M_DESCRIPTION:
            value = new_tags->comments;
            break;
        case M_TRACKNUMBER:
            if (new_tags->track)
            {
                value = String::from(new_tags->track->index);
                item->setTrackNumber((int)new_tags->track->index);
            }
            else
			{
			    MP4TagsFree( new_tags );
                return;
            }
            break;
        default:
			MP4TagsFree( new_tags );
            return;
    }

	MP4TagsFree( new_tags );
    value = trim_string(value);

    if (string_ok(value))
    {
        item->setMetadata(MT_KEYS[field].upnp, sc->convert(value));
        log_debug("mp4 handler: setting metadata on item: %d, %s\n", field, sc->convert(value).c_str());
    }
}

void LibMP4V2Handler::fillMetadata(Ref<CdsItem> item)
{
    MP4FileHandle mp4;

    // the location has already been checked by the setMetadata function
    mp4 = MP4Read(item->getLocation().c_str()); 
    if (mp4 == MP4_INVALID_FILE_HANDLE)
    {
        log_error("Skipping metadata extraction for file %s\n", 
                  item->getLocation().c_str());
        return;
    }

    try
    {
        for (int i = 0; i < M_MAX; i++)
            addMetaField((metadata_fields_t) i, mp4, item);

        Ref<ConfigManager> cm = ConfigManager::getInstance();

        //  MP4GetTimeScale returns the time scale in units of ticks per 
        //  second for the mp4 file. Caveat: tracks may use the same time 
        //  scale as  the  movie or may use their own time scale.
        u_int32_t timescale = MP4GetTimeScale(mp4);
        //  MP4GetDuration  returns the maximum duration of all the tracks in 
        //  the specified mp4 file.
        //
        //  Caveat: the duration is the movie (file) time scale units.
        MP4Duration duration = MP4GetDuration(mp4);

        duration = duration / timescale;
        if (duration > 0)
            item->getResource(0)->addAttribute(MetadataHandler::getResAttrName(R_DURATION),
                    secondsToHMS(duration));
 

        MP4TrackId tid = MP4FindTrackId(mp4, 0, MP4_AUDIO_TRACK_TYPE);
        if (tid != MP4_INVALID_TRACK_ID)
        {
#ifdef HAVE_MP4_GET_TRACK_AUDIO_CHANNELS
            int temp = MP4GetTrackAudioChannels(mp4, tid);
            if (temp > 0)
            {
                item->getResource(0)->addAttribute(MetadataHandler::getResAttrName(R_NRAUDIOCHANNELS), String::from(temp));

                timescale =  MP4GetTrackTimeScale(mp4, tid);
                if (timescale > 0)
                    item->getResource(0)->addAttribute(MetadataHandler::getResAttrName(R_SAMPLEFREQUENCY), String::from((unsigned int)timescale));
            }
#endif
            // note: UPnP requres bytes/second
            timescale = MP4GetTrackBitRate(mp4, tid);
            if (timescale > 0)
            {
                timescale = timescale / 8;
                item->getResource(0)->addAttribute(MetadataHandler::getResAttrName(R_BITRATE), String::from(timescale));
            }
        }

#if defined(HAVE_MAGIC)
        void *art_data = 0;
        u_int32_t art_data_len = 0;
        String art_mimetype;

        const MP4Tags* new_tags = MP4TagsAlloc();
        MP4TagsFetch(new_tags, mp4);
        if (new_tags->artworkCount)
        {
            art_data = new_tags->artwork->data;
            art_data_len = new_tags->artwork->size;
        }
#ifdef HAVE_MP4_GET_METADATA_COVER_ART_COUNT
        if (new_tags->artworkCount && art_data_len > 0) 
#endif
        {
            if (art_data)
            {
                try
                {
                    art_mimetype = ContentManager::getInstance()->getMimeTypeFromBuffer((void *)art_data, art_data_len);
                    if (!string_ok(art_mimetype))
                        art_mimetype = _(MIMETYPE_DEFAULT);

                }
                catch (Exception ex)
                {
                    MP4TagsFree(new_tags);
                    throw ex;
                }

                if (art_mimetype != _(MIMETYPE_DEFAULT))
                {
                    Ref<CdsResource> resource(new CdsResource(CH_MP4));
                    resource->addAttribute(MetadataHandler::getResAttrName(R_PROTOCOLINFO), renderProtocolInfo(art_mimetype));
                    resource->addParameter(_(RESOURCE_CONTENT_TYPE), _(ID3_ALBUM_ART));
                    item->addResource(resource);
                }
            }
        }
        MP4TagsFree(new_tags);
#endif
        MP4Close(mp4);
    }
    catch (Exception ex)
    {
        MP4Close(mp4);
        throw ex;
    }
}

Ref<IOHandler> LibMP4V2Handler::serveContent(Ref<CdsItem> item, int resNum, off_t *data_size)
{
    MP4FileHandle mp4 = MP4Read(item->getLocation().c_str());
    if (mp4 == MP4_INVALID_FILE_HANDLE)
    {
        throw _Exception(_("LibMP4V2Handler: could not open file: ") + item->getLocation());
    }

    Ref<CdsResource> res = item->getResource(resNum);

    String ctype = res->getParameters()->get(_(RESOURCE_CONTENT_TYPE));

    if (ctype != ID3_ALBUM_ART)
        throw _Exception(_("LibMP4V2Handler: got unknown content type: ") + ctype);

    const MP4Tags* new_tags = MP4TagsAlloc();
    if (MP4TagsFetch(new_tags, mp4))
    {
#ifdef HAVE_MP4_GET_METADATA_COVER_ART_COUNT
        if (!new_tags->artworkCount)
            throw _Exception(_("LibMP4V2Handler: resource has no album art information"));
#endif
        void *art_data = 0;
        u_int32_t art_data_len;

        const MP4TagArtwork* art = new_tags->artwork;
        art_data = art->data;
        art_data_len = art->size;
        if (art)
        {
            if (art_data)
            {
                *data_size = (off_t)art_data_len;
                Ref<IOHandler> h(new MemIOHandler(art_data, art_data_len));
                MP4TagsFree(new_tags);
                return h;
            }
        }
        MP4TagsFree(new_tags);
    }
    throw _Exception(_("LibMP4V2Handler: could not serve album art "
            "for file") + item->getLocation() + 
        " - embedded image not found");
}

#endif // HAVE_LIBMP4V2
