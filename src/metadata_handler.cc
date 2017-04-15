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

#include "metadata_handler.h"
#include "tools.h"
#include "config_manager.h"

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
    { "M_ALBUMARTIST", "upnp:artist@role[AlbumArtist]"},
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
    resource->addAttribute(getResAttrName(R_PROTOCOLINFO), renderProtocolInfo(mimetype));
    resource->addAttribute(getResAttrName(R_SIZE), String::from(filesize));
    
    item->addResource(resource);

    Ref<Dictionary> mappings = ConfigManager::getInstance()->getDictionaryOption(CFG_IMPORT_MAPPINGS_MIMETYPE_TO_CONTENTTYPE_LIST);
    String content_type = mappings->get(mimetype);
   
    if ((content_type == CONTENT_TYPE_OGG) && (isTheora(item->getLocation())))
            item->setFlag(OBJECT_FLAG_OGG_THEORA);

#ifdef HAVE_TAGLIB
    if ((content_type == CONTENT_TYPE_MP3) || 
        ((content_type == CONTENT_TYPE_OGG) && (!item->getFlag(OBJECT_FLAG_OGG_THEORA))) ||
        (content_type == CONTENT_TYPE_WMA) ||
        (content_type == CONTENT_TYPE_WAVPACK) ||
        (content_type == CONTENT_TYPE_FLAC) ||
        (content_type == CONTENT_TYPE_PCM) ||
        (content_type == CONTENT_TYPE_AIFF) ||
        (content_type == CONTENT_TYPE_APE) ||
        (content_type == CONTENT_TYPE_MP4))
    {
        TagLibHandler().fillMetadata(item);
    }
#endif // HAVE_TAGLIB

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
        LibExifHandler().fillMetadata(item);
    }
#endif // HAVE_LIBEXIF

#ifdef HAVE_FFMPEG
    if (content_type != CONTENT_TYPE_PLAYLIST &&
        ((content_type == CONTENT_TYPE_OGG &&
         item->getFlag(OBJECT_FLAG_OGG_THEORA)) ||
        item->getMimeType().startsWith(_("video")) ||
        item->getMimeType().startsWith(_("audio"))))
    {
        FfmpegHandler().fillMetadata(item);
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

    // Fanart for all things!
    FanArtHandler().fillMetadata(item);
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
#ifdef HAVE_TAGLIB
        case CH_ID3:
            return Ref<MetadataHandler>(new TagLibHandler());
#endif
#if defined(HAVE_FFMPEG) && defined(HAVE_FFMPEGTHUMBNAILER)
        case CH_FFTH:
            return Ref<MetadataHandler>(new FfmpegHandler());
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
