/*  extractor_handler.cc - this file is part of MediaTomb.

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

/// \file extractor_handler.cc
/// \brief Implementeation of the Id3Handler class.

#define HAVE_EXTRACTOR 1

#ifdef HAVE_EXTRACTOR

#include <extractor.h>

#include "extractor_handler.h"
#include "string_converter.h"
#include "logger.h"

using namespace zmm;

ExtractorHandler::ExtractorHandler() : MetadataHandler()
{
}
       
static void addField(metadata_fields_t field, EXTRACTOR_KeywordList *keywords, Ref<CdsItem> item)
{
    String value;
    const char *temp = NULL;
  
    Ref<StringConverter> sc = StringConverter::m2i();
    int genre;
    
    switch (field)
    {
        case M_TITLE:
            temp = EXTRACTOR_extractLast(EXTRACTOR_TITLE, keywords);
            break;
        case M_ARTIST:
            temp = EXTRACTOR_extractLast(EXTRACTOR_ARTIST, keywords);
            break;
        case M_ALBUM:
            temp = EXTRACTOR_extractLast(EXTRACTOR_ALBUM, keywords);
            break;
        case M_DATE:
            temp = EXTRACTOR_extractLast(EXTRACTOR_DATE, keywords);
            break;
        case M_GENRE:
            temp = EXTRACTOR_extractLast(EXTRACTOR_GENRE, keywords);
            break;
        case M_DESCRIPTION:
            temp = EXTRACTOR_extractLast(EXTRACTOR_DESCRIPTION, keywords);

            if (temp == NULL)
                temp = EXTRACTOR_extractLast(EXTRACTOR_COMMENT, keywords);
            break;
        default:
            return;
    }

    if (temp != NULL)
        value = (char *)temp;

    if (string_ok(value))
    {
        item->setMetadata(String(MT_KEYS[field].upnp), sc->convert(value));
//        log_info(("Setting metadata on item: %d, %s\n", field, sc->convert(value).c_str()));
    }
}

void ExtractorHandler::fillMetadata(Ref<CdsItem> item)
{
    EXTRACTOR_ExtractorList *extractors = EXTRACTOR_loadDefaultLibraries();
    EXTRACTOR_KeywordList *keywords = EXTRACTOR_getKeywords(extractors, item->getLocation().c_str());
//    EXTRACTOR_printKeywords(stdout, keywords);

    for (int i = 0; i < M_MAX; i++)
        addField((metadata_fields_t) i, keywords, item);
    
    EXTRACTOR_freeKeywords(keywords);
    EXTRACTOR_removeAll(extractors);
}

Ref<IOHandler> ExtractorHandler::serveContent(Ref<CdsItem> item, int resNum)
{
    return nil;
}

#endif // HAVE_EXTRACTOR

