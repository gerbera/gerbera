/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    libmp4v2_handler.cc - this file is part of MediaTomb.
    
    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>
    
    Copyright (C) 2006-2008 Gena Batyan <bgeradz@mediatomb.cc>,
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
    
    $ $
*/

/// \file libmp4v2_handler.cc
/// \brief Implementeation of the LibMP4V2Handler class.

#ifdef HAVE_CONFIG_H
    #include "autoconfig.h"
#endif

#ifdef HAVE_LIBMP4V2

#include <mp4.h>

#include "libmp4v2_handler.h"
#include "string_converter.h"
#include "common.h"
#include "tools.h"
#include "mem_io_handler.h"
#include "content_manager.h"
#include "config_manager.h"

using namespace zmm;

LibMP4V2Handler::LibMP4V2Handler() : MetadataHandler()
{
}

static void addMetaField(metadata_fields_t field, MP4FileHandle *mp4, Ref<CdsItem> item)
{
    String value;
    char*  mp4_retval = NULL;
    u_int16_t track;
    u_int16_t total_tracks;
 
    Ref<StringConverter> sc = StringConverter::i2i();
    
    switch (field)
    {
        case M_TITLE:
            MP4GetMetadataName(mp4, &mp4_retval);
            break;
        case M_ARTIST:
            MP4GetMetadataArtist(mp4, &mp4_retval);
            break;
        case M_ALBUM:
            MP4GetMetadataAlbum(mp4, &mp4_retval);
            break;
        case M_DATE:
            MP4GetMetadataYear(mp4, &mp4_retval);
            if (mp4_retval)
            {
                value = String(mp4_retval);
                free(mp4_retval);
                if (string_ok(value))
                    value = value + "-01-01";
                else
                    return;
            }
            break;
        case M_GENRE:
            MP4GetMetadataGenre(mp4, &mp4_retval);
            break;
        case M_DESCRIPTION:
            MP4GetMetadataComment(mp4, &mp4_retval);
            break;
        case M_TRACKNUMBER:
            MP4GetMetadataTrack(mp4, &track, &total_tracks);
            if (track > 0)
            {
                value = String::from(track);
                item->setTrackNumber((int)track);
            }
            else
                return;
            break;
        default:
            return;
    }

    if ((field != M_DATE) && (field != M_TRACKNUMBER) && 
        (mp4_retval))
    {
        value = String(mp4_retval);
        free(mp4_retval);
    }
    
    value = trim_string(value);
    
    if (string_ok(value))
    {
        item->setMetadata(String(MT_KEYS[field].upnp), sc->convert(value));
        log_debug("mp4 handler: setting metadata on item: %d, %s\n", field, sc->convert(value).c_str());
    }
}

void LibMP4V2Handler::fillMetadata(Ref<CdsItem> item)
{
    MP4FileHandle mp4;
    Ref<StringConverter> sc = StringConverter::i2i();
    int temp;
    
    // the location has already been checked by the setMetadata function
    mp4 = MP4Read(item->getLocation().c_str()); 

    for (int i = 0; i < M_MAX; i++)
        addMetaField((metadata_fields_t) i, &mp4, item);

    Ref<ConfigManager> cm = ConfigManager::getInstance();

/*
        int temp;

        // note: UPnP requres bytes/second
        if ((header->vbr_bitrate) > 0)
        {
            temp = (header->vbr_bitrate) / 8; 
        }
        else
        {
            temp = (header->bitrate) / 8;
        }

        if (temp > 0)
        {
            item->getResource(0)->addAttribute(MetadataHandler::getResAttrName(R_BITRATE),
                                               String::from(temp)); 
        }
*/
//  MP4GetTimeScale returns the time scale in units of ticks per second for
//  the mp4 file. Caveat: tracks may use the same time scale as  the  movie
//  or may use their own time scale.
    u_int32_t timescale = MP4GetTimeScale(mp4);
//  MP4GetDuration  returns  the  maximum duration of all the tracks in the
//  specified mp4 file.
//
//  Caveat: the duration is the movie (file) time scale units.
    MP4Duration duration = MP4GetDuration(mp4);
    
    duration = duration / timescale;
    if (duration > 0)
        item->getResource(0)->addAttribute(MetadataHandler::getResAttrName(R_DURATION),
                                           secondsToHMS(duration));
/*
        if ((header->frequency) > 0)
        {
            item->getResource(0)->addAttribute(MetadataHandler::getResAttrName(R_SAMPLEFREQUENCY), 
                                               String::from((int)(header->frequency)));
        }
        */


    MP4TrackId tid = MP4FindTrackId(mp4, 0, MP4_AUDIO_TRACK_TYPE);
    if (tid != MP4_INVALID_TRACK_ID)
    {
        temp = MP4GetTrackAudioChannels(mp4, tid);
        if (temp > 0)
        {
            item->getResource(0)->addAttribute(MetadataHandler::getResAttrName(R_NRAUDIOCHANNELS),
                                               String::from(temp));
        }
    }

#if 0    
//#ifdef HAVE_ID3_ALBUMART
    // get album art
    // we have a bit of design problem here - the album art is actually not
    // a resource, but our architecture is built that way that we can only
    // serve resources, so we are going to bend this a little:
    // the album art will be saved as a resource, but the CdsResourceManager
    // will handle this special case and render the XML correctly...
    // \todo discuss the album art approach with Leo
    if (ID3_HasPicture(&tag))
    {
        ID3_Frame* frame = NULL;
        frame = tag.Find(ID3FID_PICTURE);
        if (frame != NULL)
        {
            ID3_Field* art = frame->GetField(ID3FN_DATA);
            if (art != NULL)
            {
                Ref<StringConverter> sc = StringConverter::m2i(); 
                String art_mimetype = sc->convert(String(ID3_GetPictureMimeType(&tag)));
                if (!string_ok(art_mimetype) || (art_mimetype.index('/') == -1))
                {
#ifdef HAVE_MAGIC
                    art_mimetype = ContentManager::getInstance()->getMimeTypeFromBuffer((void *)art->GetRawBinary(), art->Size());
                    if (!string_ok(art_mimetype))
#endif
                       art_mimetype = _(MIMETYPE_DEFAULT);
                }

                // if we could not determine the mimetype, then there is no
                // point to add the resource - it's probably garbage
                if (art_mimetype != _(MIMETYPE_DEFAULT))
                {
                    Ref<CdsResource> resource(new CdsResource(CH_ID3));
                    resource->addAttribute(MetadataHandler::getResAttrName(R_PROTOCOLINFO), renderProtocolInfo(art_mimetype));
                    resource->addParameter(_(RESOURCE_CONTENT_TYPE), _(ID3_ALBUM_ART));
                    item->addResource(resource);
                }

            }
        }
    }
#endif
//    tag.Clear();
    MP4Close(mp4);
}

Ref<IOHandler> LibMP4V2Handler::serveContent(Ref<CdsItem> item, int resNum, off_t *data_size)
{
    /*
    ID3_Tag tag;
    if (tag.Link(item->getLocation().c_str()) == 0)
    {
        throw _Exception(_("LibMP4V2Handler: could not open file: ") + item->getLocation());
    }

    Ref<CdsResource> res = item->getResource(resNum);

    String ctype = res->getParameters()->get(_(RESOURCE_CONTENT_TYPE));

    if (ctype != ID3_ALBUM_ART)
        throw _Exception(_("LibMP4V2Handler: got unknown content type: ") + ctype);

    if (!ID3_HasPicture(&tag))
        throw _Exception(_("LibMP4V2Handler: resource has no album art information"));

    ID3_Frame* frame = NULL;
    frame = tag.Find(ID3FID_PICTURE);

    if (frame == NULL)
        throw _Exception(_("LibMP4V2Handler: could not server album art - empty frame"));

    ID3_Field* art = frame->GetField(ID3FN_DATA);
    if (art == NULL)
        throw _Exception(_("LibMP4V2Handler: could not server album art - empty field"));

    Ref<IOHandler> h(new MemIOHandler((void *)art->GetRawBinary(), art->Size()));
    *data_size = art->Size();
    tag.Clear();

    return h;
    */
    *data_size = -1;
    return nil;
}

#endif // HAVE_LIBMP4V2
