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

/// \file metadata_handler.cc
/// \brief Implementeation of the MetadataHandler class.

#include "metadata_handler.h"
#include "string_converter.h"
#include "logger.h"

#ifdef HAVE_EXIV2
#include "metadata/exiv2_handler.h"
#endif

#ifdef HAVE_ID3
#include "metadata/id3_handler.h"
#endif

#ifdef HAVE_EXTRACTOR
#include "metadata/extractor_handler.h"
#endif

using namespace zmm;

mt_key MT_KEYS[] = {
    { "M_TITLE", "dc:title" },
    { "M_ARTIST", "upnp:artist" },
    { "M_ALBUM", "upnp:album" },
    { "M_DATE", "dc:date" },
    { "M_GENRE", "upnp:genre" },
    { "M_DESCRIPTION", "dc:description" }
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

    Ref<MetadataHandler> handler;
    do
    {
#ifdef HAVE_ID3
        if (mimetype == "audio/mpeg")
	{
            handler = Ref<MetadataHandler>(new Id3Handler());
	    break;
	}
#endif
#ifdef HAVE_EXIV2
        if (mimetype == "image/jpeg")
	{
            handler = Ref<MetadataHandler>(new Exiv2Handler());
	    break;
	}
#endif
#ifdef HAVE_EXTRACTOR
    {
        handler = Ref<MetadataHandler>(new ExtractorHandler());
        break;
    }
#endif

    }
    while (false);
    
    if (handler == nil)
        return;
    handler->fillMetadata(item);
}

String MetadataHandler::getFieldName(metadata_fields_t field)
{
    return String(MT_KEYS[field].upnp);
}

