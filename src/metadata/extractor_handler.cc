/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    extractor_handler.cc - this file is part of MediaTomb.
    
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

/// \file extractor_handler.cc
/// \brief Implementeation of the ExtractorHandler class.

#ifdef HAVE_CONFIG_H
    #include "autoconfig.h"
#endif

#ifdef HAVE_LIBEXTRACTOR

#include <extractor.h>

#include "config_manager.h"
#include "extractor_handler.h"
#include "string_converter.h"
#include "common.h"
#include "tools.h"
#include "rexp.h"
#include "mxml/mxml.h"


using namespace zmm;
using namespace mxml;


ExtractorHandler::ExtractorHandler() : MetadataHandler()
{
}

static EXTRACTOR_KeywordType getTagFromString(String tag)
{
    if (tag == "EXTRACTOR_UNKNOWN")
        return EXTRACTOR_UNKNOWN;
    if (tag == "EXTRACTOR_FILENAME")
        return EXTRACTOR_FILENAME;
    if (tag == "EXTRACTOR_MIMETYPE")
        return EXTRACTOR_MIMETYPE;
    if (tag == "EXTRACTOR_TITLE")
        return EXTRACTOR_TITLE;
    if (tag == "EXTRACTOR_AUTHOR")
        return EXTRACTOR_AUTHOR;
    if (tag == "EXTRACTOR_ARTIST")
        return EXTRACTOR_ARTIST;
    if (tag == "EXTRACTOR_DESCRIPTION")
        return EXTRACTOR_DESCRIPTION;
    if (tag == "EXTRACTOR_COMMENT")
        return EXTRACTOR_COMMENT;
    if (tag == "EXTRACTOR_DATE")
        return EXTRACTOR_DATE;
    if (tag == "EXTRACTOR_PUBLISHER")
        return EXTRACTOR_PUBLISHER;
    if (tag == "EXTRACTOR_LANGUAGE")
        return EXTRACTOR_LANGUAGE;
    if (tag == "EXTRACTOR_ALBUM")
        return EXTRACTOR_ALBUM;
    if (tag == "EXTRACTOR_GENRE")
        return EXTRACTOR_GENRE;
    if (tag == "EXTRACTOR_LOCATION")
        return EXTRACTOR_LOCATION;
    if (tag == "EXTRACTOR_VERSIONNUMBER")
        return EXTRACTOR_VERSIONNUMBER;
    if (tag == "EXTRACTOR_ORGANIZATION")
        return EXTRACTOR_ORGANIZATION;
    if (tag == "EXTRACTOR_COPYRIGHT")
        return EXTRACTOR_COPYRIGHT;
    if (tag == "EXTRACTOR_SUBJECT")
        return EXTRACTOR_SUBJECT;
    if (tag == "EXTRACTOR_KEYWORDS")
        return EXTRACTOR_KEYWORDS;
    if (tag == "EXTRACTOR_CONTRIBUTOR")
        return EXTRACTOR_CONTRIBUTOR;
    if (tag == "EXTRACTOR_RESOURCE_TYPE")
        return EXTRACTOR_RESOURCE_TYPE;
    if (tag == "EXTRACTOR_FORMAT")
        return EXTRACTOR_FORMAT;
    if (tag == "EXTRACTOR_RESOURCE_IDENTIFIER")
        return EXTRACTOR_RESOURCE_IDENTIFIER;
    if (tag == "EXTRACTOR_SOURCE")
        return EXTRACTOR_SOURCE;
    if (tag == "EXTRACTOR_RELATION")
        return EXTRACTOR_RELATION;
    if (tag == "EXTRACTOR_COVERAGE")
        return EXTRACTOR_COVERAGE;
    if (tag == "EXTRACTOR_SOFTWARE")
        return EXTRACTOR_SOFTWARE;
    if (tag == "EXTRACTOR_DISCLAIMER")
        return EXTRACTOR_DISCLAIMER;
    if (tag == "EXTRACTOR_WARNING")
        return EXTRACTOR_WARNING;
    if (tag == "EXTRACTOR_TRANSLATED")
        return EXTRACTOR_TRANSLATED;
    if (tag == "EXTRACTOR_CREATION_DATE")
        return EXTRACTOR_CREATION_DATE;
    if (tag == "EXTRACTOR_MODIFICATION_DATE")
        return EXTRACTOR_MODIFICATION_DATE;
    if (tag == "EXTRACTOR_CREATOR")
        return EXTRACTOR_CREATOR;
    if (tag == "EXTRACTOR_PRODUCER")
        return EXTRACTOR_PRODUCER;
    if (tag == "EXTRACTOR_PAGE_COUNT")
        return EXTRACTOR_PAGE_COUNT;
    if (tag == "EXTRACTOR_PAGE_ORIENTATION")
        return EXTRACTOR_PAGE_ORIENTATION;
    if (tag == "EXTRACTOR_PAPER_SIZE")
        return EXTRACTOR_PAPER_SIZE;
    if (tag == "EXTRACTOR_USED_FONTS")
        return EXTRACTOR_USED_FONTS;
    if (tag == "EXTRACTOR_PAGE_ORDER")
        return EXTRACTOR_PAGE_ORDER;
    if (tag == "EXTRACTOR_CREATED_FOR")
        return EXTRACTOR_CREATED_FOR;
    if (tag == "EXTRACTOR_MAGNIFICATION")
        return EXTRACTOR_MAGNIFICATION;
    if (tag == "EXTRACTOR_RELEASE")
        return EXTRACTOR_RELEASE;
    if (tag == "EXTRACTOR_GROUP")
        return EXTRACTOR_GROUP;
    if (tag == "EXTRACTOR_SIZE")
        return EXTRACTOR_SIZE;
    if (tag == "EXTRACTOR_SUMMARY")
        return EXTRACTOR_SUMMARY;
    if (tag == "EXTRACTOR_PACKAGER")
        return EXTRACTOR_PACKAGER;
    if (tag == "EXTRACTOR_VENDOR")
        return EXTRACTOR_VENDOR;
    if (tag == "EXTRACTOR_LICENSE")
        return EXTRACTOR_LICENSE;
    if (tag == "EXTRACTOR_DISTRIBUTION")
        return EXTRACTOR_DISTRIBUTION;
    if (tag == "EXTRACTOR_BUILDHOST")
        return EXTRACTOR_BUILDHOST;
    if (tag == "EXTRACTOR_OS")
        return EXTRACTOR_OS;
    if (tag == "EXTRACTOR_DEPENDENCY")
        return EXTRACTOR_DEPENDENCY;
    if (tag == "EXTRACTOR_HASH_MD4")
        return EXTRACTOR_HASH_MD4;
    if (tag == "EXTRACTOR_HASH_MD5")
        return EXTRACTOR_HASH_MD5;
    if (tag == "EXTRACTOR_HASH_SHA0")
        return EXTRACTOR_HASH_SHA0;
    if (tag == "EXTRACTOR_HASH_SHA1")
        return EXTRACTOR_HASH_SHA1;
    if (tag == "EXTRACTOR_HASH_RMD160")
        return EXTRACTOR_HASH_RMD160;
    if (tag == "EXTRACTOR_RESOLUTION")
        return EXTRACTOR_RESOLUTION;
    if (tag == "EXTRACTOR_CATEGORY")
        return EXTRACTOR_CATEGORY;
    if (tag == "EXTRACTOR_BOOKTITLE")
        return EXTRACTOR_BOOKTITLE;
    if (tag == "EXTRACTOR_PRIORITY")
        return EXTRACTOR_PRIORITY;
    if (tag == "EXTRACTOR_CONFLICTS")
        return EXTRACTOR_CONFLICTS;
    if (tag == "EXTRACTOR_REPLACES")
        return EXTRACTOR_REPLACES;
    if (tag == "EXTRACTOR_PROVIDES")
        return EXTRACTOR_PROVIDES;
    if (tag == "EXTRACTOR_CONDUCTOR")
        return EXTRACTOR_CONDUCTOR;
    if (tag == "EXTRACTOR_INTERPRET")
        return EXTRACTOR_INTERPRET;
    if (tag == "EXTRACTOR_OWNER")
        return EXTRACTOR_OWNER;
    if (tag == "EXTRACTOR_LYRICS")
        return EXTRACTOR_LYRICS;
    if (tag == "EXTRACTOR_MEDIA_TYPE")
        return EXTRACTOR_MEDIA_TYPE;
    if (tag == "EXTRACTOR_CONTACT")
        return EXTRACTOR_CONTACT;
    if (tag == "EXTRACTOR_THUMBNAIL_DATA")
        return EXTRACTOR_THUMBNAIL_DATA;

#ifdef EXTRACTOR_GE_0_5_2
    if (tag == "EXTRACTOR_PUBLICATION_DATE")
        return EXTRACTOR_PUBLICATION_DATE;
    if (tag == "EXTRACTOR_CAMERA_MAKE")
        return EXTRACTOR_CAMERA_MAKE;
    if (tag == "EXTRACTOR_CAMERA_MODEL")
        return EXTRACTOR_CAMERA_MODEL;
    if (tag == "EXTRACTOR_EXPOSURE")
        return EXTRACTOR_EXPOSURE;
    if (tag == "EXTRACTOR_APERTURE")
        return EXTRACTOR_APERTURE;
    if (tag == "EXTRACTOR_EXPOSURE_BIAS")
        return EXTRACTOR_EXPOSURE_BIAS;
    if (tag == "EXTRACTOR_FLASH")
        return EXTRACTOR_FLASH;
    if (tag == "EXTRACTOR_FLASH_BIAS")
        return EXTRACTOR_FLASH_BIAS;
    if (tag == "EXTRACTOR_FOCAL_LENGTH")
        return EXTRACTOR_FOCAL_LENGTH;
    if (tag == "EXTRACTOR_FOCAL_LENGTH_35MM")
        return EXTRACTOR_FOCAL_LENGTH_35MM;
    if (tag == "EXTRACTOR_ISO_SPEED")
        return EXTRACTOR_ISO_SPEED;
    if (tag == "EXTRACTOR_EXPOSURE_MODE")
        return EXTRACTOR_EXPOSURE_MODE;
    if (tag == "EXTRACTOR_METERING_MODE")
        return EXTRACTOR_METERING_MODE;
    if (tag == "EXTRACTOR_MACRO_MODE")
        return EXTRACTOR_MACRO_MODE;
    if (tag == "EXTRACTOR_IMAGE_QUALITY")
        return EXTRACTOR_IMAGE_QUALITY;
    if (tag == "EXTRACTOR_WHITE_BALANCE")
        return EXTRACTOR_WHITE_BALANCE;
    if (tag == "EXTRACTOR_ORIENTATION")
        return EXTRACTOR_ORIENTATION;
#endif // EXTRACTOR_GE_0_5_2

    log_warning("Ignoring unknown libextractor tag: %s\n", tag.c_str());
    return EXTRACTOR_UNKNOWN;
}

static void addMetaField(metadata_fields_t field, EXTRACTOR_KeywordList *keywords, Ref<CdsItem> item, Ref<StringConverter> sc)
{
    String value;
    const char *temp = NULL;
 
    /// \todo check if UTF-8 conversion is needed, may already be in UTF-8
    
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
        value = temp;

    value = trim_string(value);
    
    if (string_ok(value))
    {
        item->setMetadata(MT_KEYS[field].upnp, sc->convert(value));
//        log_debug("Setting metadata on item: %d, %s\n", field, sc->convert(value).c_str());
    }
}

static void addResourceField(resource_attributes_t attr, EXTRACTOR_KeywordList *keywords, Ref<CdsItem> item, Ref<StringConverter> sc)
{
    String value;
    const char *temp = NULL;
    
    switch (attr)
    {
        case R_RESOLUTION:
            temp = EXTRACTOR_extractLast(EXTRACTOR_SIZE, keywords);
            break;
/*        case R_SIZE:
            temp = EXTRACTOR_extractLast(EXTRACTOR_SIZE, keywords);
            break;
*/
        default:
            return;
    }

    if (temp != NULL)
        value = temp;

    if (string_ok(value))
    {
          item->getResource(0)->addAttribute(MetadataHandler::getResAttrName(attr), value);
    }
}



Ref<RExp> ReAudioFormat;
EXTRACTOR_ExtractorList *extractors = NULL;
bool load_libraries_failed = false;

void ExtractorHandler::fillMetadata(Ref<CdsItem> item)
{
    if (load_libraries_failed)
        return;
    Ref<Array<StringBase> > aux;
    Ref<StringConverter> sc = StringConverter::i2i();
        
    if (! extractors)
    {
        extractors = EXTRACTOR_loadDefaultLibraries();
        if (! extractors)
            load_libraries_failed = true;
    }
    EXTRACTOR_KeywordList *keywords = EXTRACTOR_getKeywords(extractors, item->getLocation().c_str());

    //EXTRACTOR_printKeywords(stdout, keywords);

    for (int i = 0; i < M_MAX; i++)
        addMetaField((metadata_fields_t)i, keywords, item, sc);
    
    for (int i = 0; i < R_MAX; i++)
        addResourceField((resource_attributes_t)i, keywords, item, sc);

    Ref<ConfigManager> cm = ConfigManager::getInstance();
    aux = cm->getStringArrayOption(CFG_IMPORT_LIBOPTS_EXTRACTOR_AUXDATA_TAGS_LIST);
    if (aux != nil)
    {
        String value;
        String tmp;
        const char *temp = NULL;
        
        for (int j = 0; j < aux->size(); j++)
        {

            tmp = aux->get(j);
            if (string_ok(tmp))
            {
                temp = EXTRACTOR_extractLast(getTagFromString(tmp), keywords);
                if (temp != NULL)
                {
                    value = temp; 
                    if (string_ok(value))
                    {
                        value = sc->convert(value);
                        item->setAuxData(tmp, value);
//                      log_debug(("Adding tag: %s with value %s\n", tmp.c_str(), value.c_str()));
                    }
                }
            }
        }
    }

    if (ReAudioFormat == nil)
    {
        ReAudioFormat = Ref<RExp>(new RExp());
        // 64 kbps, 44100 hz, 8m30 stereo
        ReAudioFormat->compile(_("([0-9]+)\\s+kbps,\\s*([0-9]+)\\s+hz,\\s*"
                                 "(([0-9]+)h)?([0-9]+)m([0-9]+)\\s(\\S+)"), "i");
    }
   
    /*
    temp = EXTRACTOR_extractLast(EXTRACTOR_FORMAT, keywords);
    log_debug("EXTRACTOR_FORMAT: %s\n", temp);

    if (temp)
    {
        Ref<Matcher> matcher = ReAudioFormat->match((char *)temp);
        if (matcher != nil)
        {
            log_debug(("BR:%s FR:%s H:%s M:%s S:%s TYPE:%s\n",
                       matcher->group(1).c_str(),
                       matcher->group(2).c_str(),
                       matcher->group(4).c_str(),
                       matcher->group(5).c_str(),
                       matcher->group(6).c_str(),
                       matcher->group(7).c_str()));
        }
        else
        {
            log_debug(("format pattern unmatched!"));
        }
    }

    */
    EXTRACTOR_freeKeywords(keywords);

    // commented out for the sake of efficiency
    // EXTRACTOR_removeAll(extractors);
}

Ref<IOHandler> ExtractorHandler::serveContent(Ref<CdsItem> item, int resNum, off_t *data_size)
{
    *data_size = -1;
    return nil;
}

#endif // HAVE_EXTRACTOR
