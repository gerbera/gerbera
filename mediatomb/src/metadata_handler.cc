/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    metadata_handler.cc - this file is part of MediaTomb.
    
    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>
    
    Copyright (C) 2006-2007 Gena Batyan <bgeradz@mediatomb.cc>,
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
#ifdef HAVE_ID3 // we only want ID3 if taglib was not found
#include "metadata/id3_handler.h"
#endif // HAVE_ID3
#endif // HAVE_TAGLIB


#ifdef HAVE_FFMPEG
#include "metadata/ffmpeg_handler.h"
#endif

#ifdef HAVE_EXTRACTOR
#include "metadata/extractor_handler.h"
#endif

#ifdef HAVE_EXIF
#include "metadata/libexif_handler.h"
#endif

using namespace zmm;

mt_key MT_KEYS[] = {
    { "M_TITLE", "dc:title" },
    { "M_ARTIST", "upnp:artist" },
    { "M_ALBUM", "upnp:album" },
    { "M_DATE", "dc:date" },
    { "M_GENRE", "upnp:genre" },
    { "M_DESCRIPTION", "dc:description" },
    { "M_TRACKNUMBER", "upnp:originalTrackNumber"},
    { "M_ALBUMARTURI", "upnp:albumArtURI"}

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

    string_ok_ex(location);
    check_path_ex(location);

    String mimetype = item->getMimeType();

    Ref<CdsResource> resource(new CdsResource(CH_DEFAULT));
    resource->addAttribute(String(RES_KEYS[R_PROTOCOLINFO].upnp),
                           renderProtocolInfo(mimetype));
    item->addResource(resource);

    Ref<MetadataHandler> handler;
    Ref<Dictionary> mappings = ConfigManager::getInstance()->getDictionaryOption(CFG_IMPORT_MAPPINGS_MIMETYPE_TO_CONTENTTYPE_LIST);

    String content_type = mappings->get(mimetype);
    do
    {
#ifdef HAVE_TAGLIB
        if ((content_type == CONTENT_TYPE_MP3) || 
            (content_type == CONTENT_TYPE_OGG) || 
            (content_type == CONTENT_TYPE_FLAC))
        {
            handler = Ref<MetadataHandler>(new TagHandler());
            break;
        }
#else
#ifdef HAVE_ID3
        if (content_type == CONTENT_TYPE_MP3)
        {
            handler = Ref<MetadataHandler>(new Id3Handler());
            break;
        }
#endif // HAVE_ID3
#endif // HAVE_TAGLIB



#ifdef HAVE_EXIV2
/*        
          if (content_type == CONTENT_TYPE_JPG)
          {
          handler = Ref<MetadataHandler>(new Exiv2Handler());
          break;
          } 
*/
#endif

#ifdef HAVE_EXIF
        if (content_type == CONTENT_TYPE_JPG)
        {
            handler = Ref<MetadataHandler>(new LibExifHandler());
            break;
        }
#else
        
#ifndef HAVE_EXTRACTOR
        if (content_type == CONTENT_TYPE_JPG)
        {
            set_jpeg_resolution_resource(item, 0);
        }
#endif // EXTRACTOR
        
#endif // HAVE_EXIF

#ifdef HAVE_FFMPEG
        {
            handler = Ref<MetadataHandler>(new FfmpegHandler());
            break;
        }
#endif // HAVE_FFMPEG
 
#ifdef HAVE_EXTRACTOR
        {
            handler = Ref<MetadataHandler>(new ExtractorHandler());
            break;
        }
#endif // HAVE_EXTRACTOR

    }
    while (false);

    if (handler == nil)
        return;
    handler->fillMetadata(item);
}

String MetadataHandler::getMetaFieldName(metadata_fields_t field)
{
    return String(MT_KEYS[field].upnp);
}

String MetadataHandler::getResAttrName(resource_attributes_t attr)
{
    return String(RES_KEYS[attr].upnp);
}

Ref<MetadataHandler> MetadataHandler::createHandler(int handlerType)
{
    switch(handlerType)
    {
#ifdef HAVE_EXIF
        case CH_LIBEXIF:
            return Ref<MetadataHandler>(new LibExifHandler());
#endif
#ifdef HAVE_ID3
        case CH_ID3:
            return Ref<MetadataHandler>(new Id3Handler());
#elif HAVE_TAGLIB
        case CH_ID3:
            return Ref<MetadataHandler>(new TagHandler());
#endif
        default:
            throw _Exception(_("unknown content handler ID: ") + handlerType);
    }
}
