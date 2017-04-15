/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    metadata_handler.h - this file is part of MediaTomb.
    
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

/// \file metadata_handler.h
/// \brief Definition of the MetadataHandler class.
#ifndef __METADATA_HANDLER_H__
#define __METADATA_HANDLER_H__

#include "common.h"
#include "dictionary.h"
#include "cds_objects.h"
#include "io_handler.h"

// content handler Id's
#define CH_DEFAULT   0
#define CH_LIBEXIF   1
#define CH_ID3       2
#define CH_TRANSCODE 3
#define CH_EXTURL    4
#define CH_MP4       5
#define CH_FFTH      6
#define CH_FLAC      7
#define CH_FANART    8

#define CONTENT_TYPE_MP3        "mp3"
#define CONTENT_TYPE_OGG        "ogg"
#define CONTENT_TYPE_FLAC       "flac"
#define CONTENT_TYPE_WMA        "wma"
#define CONTENT_TYPE_WAVPACK    "wv"
#define CONTENT_TYPE_APE        "ape"
#define CONTENT_TYPE_JPG        "jpg"
#define CONTENT_TYPE_PLAYLIST   "playlist"
#define CONTENT_TYPE_MP4        "mp4"
#define CONTENT_TYPE_PCM        "pcm"
#define CONTENT_TYPE_AVI        "avi"
#define CONTENT_TYPE_MPEG       "mpeg"
#define CONTENT_TYPE_QUICKTIME  "quicktime"
#define CONTENT_TYPE_MKV        "mkv"
#define CONTENT_TYPE_MKA        "mka"
#define CONTENT_TYPE_AIFF       "aiff"

#define OGG_THEORA              "t"

#define RESOURCE_CONTENT_TYPE   "rct"
#define RESOURCE_HANDLER        "rh"

#define ID3_ALBUM_ART           "aa"
#define EXIF_THUMBNAIL          "EX_TH"
#define THUMBNAIL               "th" // thumbnail without need for special handling

typedef enum
{
    M_TITLE = 0,
    M_ARTIST,
    M_ALBUM,
    M_DATE,
    M_GENRE,
    M_DESCRIPTION,
    M_LONGDESCRIPTION,
    M_TRACKNUMBER,
    M_ALBUMARTURI,
    M_REGION,
    /// \todo make sure that those are only used with appropriate upnp classes
    M_AUTHOR,
    M_DIRECTOR,
    M_PUBLISHER,
    M_RATING,
    M_ACTOR,
    M_PRODUCER,
    M_ALBUMARTIST,

    M_MAX
} metadata_fields_t; 

typedef struct mt_key mt_key;
struct mt_key
{
    const char *sym;
    const char *upnp;
};

extern mt_key MT_KEYS[];

// res tag attributes
typedef enum
{
    R_SIZE = 0,
    R_DURATION,
    R_BITRATE,
    R_SAMPLEFREQUENCY,
    R_NRAUDIOCHANNELS,
    R_RESOLUTION,
    R_COLORDEPTH,
    R_PROTOCOLINFO,
    R_MAX
} resource_attributes_t;

typedef struct res_key res_key;
struct res_key
{   
    const char *sym;
    const char *upnp;
};

extern res_key RES_KEYS[];


/// \brief This class is responsible for providing access to metadata information
/// of various media. 
class MetadataHandler : public zmm::Object
{
public:
/// \brief Definition of the supported metadata fields.

    MetadataHandler();
       
    static void setMetadata(zmm::Ref<CdsItem> item);
    static zmm::String getMetaFieldName(metadata_fields_t field);
    static zmm::String getResAttrName(resource_attributes_t attr);

    static zmm::Ref<MetadataHandler> createHandler(int handlerType);
    
    virtual void fillMetadata(zmm::Ref<CdsItem> item) = 0;
    virtual zmm::Ref<IOHandler> serveContent(zmm::Ref<CdsItem> item, int resNum, off_t *data_size) = 0;
    virtual zmm::String getMimeType();
};

#endif // __METADATA_HANDLER_H__
