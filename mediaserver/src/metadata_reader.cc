/*  metadata_reader.cc - this file is part of MediaTomb.

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

/// \file metadata_reader.cc
/// \brief Implementeation of the MetadataReader class.

#include "metadata_reader.h"

using namespace zmm;

MetadataReader::MetadataReader() : Object()
{
}
       
void MetadataReader::getMetadata(Ref<CdsItem> item)
{
    String location = item->getLocation();

    string_ok_ex(location);
    check_path_ex(location);

    String mimetype = item->getMimeType();

    if (mimetype == "audio/mpeg")
        getID3(item);
}

void MetadataReader::getID3(Ref<CdsItem> item)
{
    ID3_Tag tag;

    tag.Link(item->getLocation().c_str()); // the location has already been checked by the getMetadata function

    for (int i = 0; i <= M_COMMENT; i++)
        addID3Field((metadata_fields_t) i, &tag, item);
}

void MetadataReader::addID3Field(metadata_fields_t field, ID3_Tag *tag, Ref<CdsItem> item)
{
    String value;
   
    switch (field)
    {
        case M_TITLE:
            value = String(ID3_GetTitle(tag));
            if (string_ok(value))
                item->setTitle(value);
            break;
        case M_ARTIST:
            value = String(ID3_GetArtist(tag));
            break;
        case M_ALBUM:
            value = String(ID3_GetAlbum(tag));
            break;
        case M_YEAR:
            value = String(ID3_GetYear(tag));
            break;
        case M_GENRE:
            value = String(ID3_GetGenre(tag));
            break;
        case M_COMMENT:
            value = String(ID3_GetComment(tag));
            if (string_ok(value))
                item->setDescription(value);
            break;
        default:
            return;
    }
}


