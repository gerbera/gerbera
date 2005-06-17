/*  metadata_reader.h - this file is part of MediaTomb.

    Copyright (C) 2005 Gena Batyan <bgeradz@deadlock.dhs.org>,
    Sergey Bostandzhyan <jin@deadlock.dhs.org>

    MediaTomb is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    MediaTomb is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with MediaTomb; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/// \file metadata_reader.h 
/// \brief Definition of the MetadataReader class.
#ifndef __METADATA_READER_H__
#define __METADATA_READER_H__

#include <id3/tag.h>
#include <id3/misc_support.h>

#include <exiv2/image.hpp>
#include <exiv2/exif.hpp>


#include "common.h"
#include "dictionary.h"
#include "cds_objects.h"

typedef enum
{
    M_TITLE = 0,
    M_ARTIST,
    M_ALBUM,
    M_DATE,
    M_GENRE,
    M_DESCRIPTION,
    M_MAX
} metadata_fields_t; 

typedef struct mt_key mt_key;
struct mt_key
{
    char *sym;
    char *upnp;
};

extern mt_key MT_KEYS[];

/// \brief This class is responsible for providing access to metadata information
/// of various media. Currently only id3 is supported.
class MetadataReader : public zmm::Object
{
public:
/// \brief Definition of the supported metadata fields.

    MetadataReader();
       
    void setMetadata(zmm::Ref<CdsItem> item);
    zmm::String getFieldName(metadata_fields_t field);

protected:
    void getID3(zmm::Ref<CdsItem> item);
    void addID3Field(metadata_fields_t field, ID3_Tag *tag, zmm::Ref<CdsItem> item);
    void getExiv2(zmm::Ref<CdsItem> item);
};

#endif

