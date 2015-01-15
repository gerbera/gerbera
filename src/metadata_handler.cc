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
/// \brief Implementeation of the MetadataHandler class.

#ifdef HAVE_CONFIG_H
    #include "autoconfig.h"
#endif

#include "metadata_handler.h"
#include "string_converter.h"
#include "tools.h"
#include "config_manager.h"

#ifdef HAVE_EXIV2
#include "metadata/exiv2_handler.h"
#endif

#ifdef HAVE_TAGLIB
#include "metadata/taglib_handler.h"
#else
#ifdef HAVE_ID3LIB // we only want ID3 if taglib was not found
#include "metadata/id3_handler.h"
#endif // HAVE_ID3LIB
#endif // HAVE_TAGLIB

#ifdef HAVE_FLAC
#include "metadata/flac_handler.h"
#endif

#ifdef HAVE_LIBMP4V2
#include "metadata/libmp4v2_handler.h"
#endif

#ifdef HAVE_FFMPEG
#include "metadata/ffmpeg_handler.h"
#endif

#ifdef HAVE_LIBEXIF
#include "metadata/libexif_handler.h"
#endif

#ifdef HAVE_LIBDVDNAV
#include "metadata/dvd_handler.h"
#endif

#include "metadata/fanart_handler.h"

using namespace zmm;

mt_key MT_KEYS[] = {
    { "M_TITLE", "dc:title" },
    { "M_ARTIST", "upnp:artist" },
    { "M_ALBUM", "upnp:album" },
    { "M_DATE", "dc:date" },
    { "M_GENRE", "upnp:genre" },
    { "M_DESCRIPTION", "dc:description" },
    { "M_LONGDESCRIPTION", "upnp:longDescription" },
    { "M_TRACKNUMBER", "upnp:originalTrackNumber"},
    { "M_ALBUMARTURI", "upnp:albumArtURI"},
    { "M_REGION", "upnp:region"},
    { "M_AUTHOR", "upnp:author"},
    { "M_DIRECTOR", "upnp:director"},
    { "M_PUBLISHER", "dc:publisher"},
    { "M_RATING", "upnp:rating"},
    { "M_ACTOR", "upnp:actor"},
    { "M_PRODUCER", "upnp:producer"},
};

res_key RES_KEYS[] = {
    { "R_SIZE", "size" },
    { "R_DURATION", "duration" },
    { "R_BITRATE", "bitrate" },
    { "R_SAMPLEFREQUENCY", "sampleFrequency" },
    { "R_NRAUDIOCHANNELS", "nrAudioChannels" },
    { "R_RESOLUTION", "resolution" },
    { "R_COLORDEPTH", "colorDepth" },
    { "R_PROTOCOLINFO", "protocolInfo" }
};


MetadataHandler::MetadataHandler() : Object()
{
}
       
void MetadataHandler::setMetadata(Ref<CdsItem> item)
{
    String location = item->getLocation();
    off_t filesize;

    string_ok_ex(location);
    check_path_ex(location, false, false, &filesize);

    String mimetype = item->getMimeType();

    Ref<CdsResource> resource(new CdsResource(CH_DEFAULT));
    resource->addAttribute(getResAttrName(R_PROTOCOLINFO),
                           renderProtocolInfo(mimetype));
    resource->addAttribute(getResAttrName(R_SIZE),
            String::from(filesize));
    
    item->addResource(resource);

    Ref<MetadataHandler> handler;
    Ref<Dictionary> mappings = ConfigManager::getInstance()->getDictionaryOption(CFG_IMPORT_MAPPINGS_MIMETYPE_TO_CONTENTTYPE_LIST);

    String content_type = mappings->get(mimetype);
   
    if ((content_type == CONTENT_TYPE_OGG) && (isTheora(item->getLocation())))
            item->setFlag(OBJECT_FLAG_OGG_THEORA);


    Ref<Array<MetadataHandler> > handlers = Ref<Array<MetadataHandler> >(new Array<MetadataHandler>());

#ifdef HAVE_TAGLIB
    if ((content_type == CONTENT_TYPE_MP3) || 
       ((content_type == CONTENT_TYPE_OGG) && 
        (!item->getFlag(OBJECT_FLAG_OGG_THEORA))) ||
        (content_type == CONTENT_TYPE_WMA) ||
        (content_type == CONTENT_TYPE_WAVPACK))
    {
        handlers->append(Ref<MetadataHandler>(new TagHandler()));
    }
#else
#ifdef HAVE_ID3LIB
    if (content_type == CONTENT_TYPE_MP3)
    {
        handlers->append(Ref<MetadataHandler>(new Id3Handler()));
    }
#endif // HAVE_ID3LIB
#endif // HAVE_TAGLIB

#ifdef HAVE_FLAC
    if (content_type == CONTENT_TYPE_FLAC)
    {
        handlers->append(Ref<MetadataHandler>(new FlacHandler()));
    }

#endif

#ifdef HAVE_EXIV2
/*        
    if (content_type == CONTENT_TYPE_JPG)
    {
        handlers->append(Ref<MetadataHandler>(new Exiv2Handler()));
    } 
*/
#endif

#ifdef HAVE_LIBEXIF
    if (content_type == CONTENT_TYPE_JPG)
    {
        handlers->append(Ref<MetadataHandler>(new LibExifHandler()));
    }
#endif // HAVE_LIBEXIF

#if defined(HAVE_LIBMP4V2)
#if !defined(HAVE_FFMPEG)
    if (content_type == CONTENT_TYPE_MP4)
#else // FFMPEG available 
    if ((content_type == CONTENT_TYPE_MP4) && 
        (!item->getMimeType().startsWith(_("video"))))
#endif // !defined(HAVE_FFMPEG)            
    {
        handlers->append(Ref<MetadataHandler>(new LibMP4V2Handler()));
    }
#endif // defined(HAVE_LIBMP4V2)

#ifdef HAVE_FFMPEG
    if (content_type != CONTENT_TYPE_PLAYLIST &&
        ((content_type == CONTENT_TYPE_OGG &&
         item->getFlag(OBJECT_FLAG_OGG_THEORA)) ||
        item->getMimeType().startsWith(_("video")) ||
        item->getMimeType().startsWith(_("audio"))))
    {
        handlers->append(Ref<MetadataHandler>(new FfmpegHandler()));
    }
#else
    if (content_type == CONTENT_TYPE_AVI)
    {
        String fourcc = getAVIFourCC(item->getLocation());
        if (string_ok(fourcc))
        {
            item->getResource(0)->addOption(_(RESOURCE_OPTION_FOURCC),
                                            fourcc);
        }
    }

#endif // HAVE_FFMPEG

#ifdef HAVE_LIBDVDNAV
    if (content_type == CONTENT_TYPE_DVD)
    {
        handlers->append(Ref<MetadataHandler>(new DVDHandler()));
    }
#endif

    // Fanart for all things!
    handlers->append(Ref<MetadataHandler>(new FanArtHandler()));

    int size = handlers->size();
    for (int i = 0; i < size; i++)
    {
        Ref<MetadataHandler> mh = handlers->get(i);
        mh->fillMetadata(item);
    }

}

String MetadataHandler::getMetaFieldName(metadata_fields_t field)
{
    return MT_KEYS[field].upnp;
}

String MetadataHandler::getResAttrName(resource_attributes_t attr)
{
    return RES_KEYS[attr].upnp;
}

Ref<MetadataHandler> MetadataHandler::createHandler(int handlerType)
{
    switch(handlerType)
    {
#ifdef HAVE_LIBEXIF
        case CH_LIBEXIF:
            return Ref<MetadataHandler>(new LibExifHandler());
#endif
#ifdef HAVE_ID3LIB
        case CH_ID3:
            return Ref<MetadataHandler>(new Id3Handler());
#elif HAVE_TAGLIB
        case CH_ID3:
            return Ref<MetadataHandler>(new TagHandler());
#endif
#ifdef HAVE_LIBMP4V2
        case CH_MP4:
            return Ref<MetadataHandler>(new LibMP4V2Handler());
#endif
#if defined(HAVE_FFMPEG) && defined(HAVE_FFMPEGTHUMBNAILER)
        case CH_FFTH:
            return Ref<MetadataHandler>(new FfmpegHandler());
#endif
#ifdef HAVE_FLAC
        case CH_FLAC:
            return Ref<MetadataHandler>(new FlacHandler());
#endif
        case CH_FANART:
            return Ref<MetadataHandler>(new FanArtHandler());
        default:
            throw _Exception(_("unknown content handler ID: ") + handlerType);
    }
}

String MetadataHandler::getMimeType()
{
    return _(MIMETYPE_DEFAULT);
}
