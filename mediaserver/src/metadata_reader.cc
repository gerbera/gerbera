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
#include "string_converter.h"

using namespace zmm;

mt_key MT_KEYS[] = {
    { "M_TITLE", "dc:title" },
    { "M_ARTIST", "upnp:artist" },
    { "M_ALBUM", "upnp:album" },
    { "M_DATE", "dc:date" },
    { "M_GENRE", "upnp:genre" },
    { "M_DESCRIPTION", "dc:description" }
};


MetadataReader::MetadataReader() : Object()
{
}
       
void MetadataReader::setMetadata(Ref<CdsItem> item)
{
    String location = item->getLocation();

    string_ok_ex(location);
    check_path_ex(location);

    String mimetype = item->getMimeType();

    if (mimetype == "audio/mpeg")
        getID3(item);
}

String MetadataReader::getFieldName(metadata_fields_t field)
{
    return String(MT_KEYS[field].upnp);
}

void MetadataReader::getID3(Ref<CdsItem> item)
{
    ID3_Tag tag;

    tag.Link(item->getLocation().c_str()); // the location has already been checked by the setMetadata function

    for (int i = 0; i < M_MAX; i++)
        addID3Field((metadata_fields_t) i, &tag, item);

    tag.Clear();
}

void MetadataReader::addID3Field(metadata_fields_t field, ID3_Tag *tag, Ref<CdsItem> item)
{
    String value;
    char*  ID3_retval = NULL;
  
    Ref<StringConverter> sc = StringConverter::m2i();
    int genre;
    
    switch (field)
    {
        case M_TITLE:
            ID3_retval = ID3_GetTitle(tag);
            break;
        case M_ARTIST:
            ID3_retval = ID3_GetArtist(tag);          
            break;
        case M_ALBUM:
            ID3_retval = ID3_GetAlbum(tag);
            break;
        case M_DATE:
            ID3_retval = ID3_GetYear(tag);
            break;
        case M_GENRE:
            genre = ID3_GetGenreNum(tag);
            value = String((char *)(ID3_V1GENRE2DESCRIPTION(genre)));
            
            break;
        case M_DESCRIPTION:
            ID3_retval = ID3_GetComment(tag);
            break;
        default:
            return;
    }

    if (field != M_GENRE)
        value = String(ID3_retval);
    
    if (ID3_retval)
        delete [] ID3_retval;

    if (string_ok(value))
    {
        item->setMetadata(String(MT_KEYS[field].upnp), sc->convert(value));
//        printf("Setting metadata on item: %d, %s\n", field, sc->convert(value).c_str());
    }
}


