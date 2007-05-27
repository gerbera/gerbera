/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    metadata_handler.h - this file is part of MediaTomb.
    
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

/// \file metadata_handler.h
/// \brief Definition of the MetadataHandler class.
#ifndef __METADATA_HANDLER_H__
#define __METADATA_HANDLER_H__

#include "common.h"
#include "dictionary.h"
#include "cds_objects.h"
#include "io_handler.h"

// content handler Id's
#define CH_DEFAULT 0
#define CH_LIBEXIF 1

#define CONTENT_TYPE_MP3        "mp3"
#define CONTENT_TYPE_OGG        "ogg"
#define CONTENT_TYPE_FLAC       "flac"
#define CONTENT_TYPE_JPG        "jpg"
#define CONTENT_TYPE_PLAYLIST   "playlist"
#define CONTENT_TYPE_MPEG4VIDEO "mp4"

typedef enum
{
    M_TITLE = 0,
    M_ARTIST,
    M_ALBUM,
    M_DATE,
    M_GENRE,
    M_DESCRIPTION,
    M_TRACKNUMBER,
    M_MAX
} metadata_fields_t; 

typedef struct mt_key mt_key;
struct mt_key
{
    char *sym;
    char *upnp;
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
    char *sym;
    char *upnp;
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
};

#endif // __METADATA_HANDLER_H__
