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

/// \file id3_handler.cc
/// \brief Implementeation of the Id3Handler class.

#ifdef HAVE_ID3

#include <id3/tag.h>
#include <id3/misc_support.h>

#include "id3_handler.h"
#include "string_converter.h"
#include "common.h"
#include "tools.h"

using namespace zmm;

Id3Handler::Id3Handler() : MetadataHandler()
{
}
       
static void addID3Field(metadata_fields_t field, ID3_Tag *tag, Ref<CdsItem> item)
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
            value = String(ID3_retval);
            value = value + "-00-00";
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

    if ((field != M_GENRE) && (field != M_DATE))
        value = String(ID3_retval);
    
    if (ID3_retval)
        delete [] ID3_retval;

    value = trim_string(value);
    
    if (string_ok(value))
    {
        item->setMetadata(String(MT_KEYS[field].upnp), sc->convert(value));
//        log_info(("Setting metadata on item: %d, %s\n", field, sc->convert(value).c_str()));
    }
}

void Id3Handler::fillMetadata(Ref<CdsItem> item)
{
    ID3_Tag tag;
    const Mp3_Headerinfo* header;
    
    tag.Link(item->getLocation().c_str()); // the location has already been checked by the setMetadata function

    for (int i = 0; i < M_MAX; i++)
        addID3Field((metadata_fields_t) i, &tag, item);

    header = tag.GetMp3HeaderInfo();
/*    if (header) 
        log_debug(("Got mp3 header\n"));
    else
        log_debug(("Could not get mp3 header\n"));
*/    
    if (header)
    {
        int temp;

        // note: UPnP requres bytes/second
        if ((header->vbr_bitrate) > 0)
        {
            temp = (header->vbr_bitrate) / 8; 
        }
        else
        {
            temp = (header->bitrate) / 8;
        }

        if (temp > 0)
        {
            item->getResource(0)->addAttribute(MetadataHandler::getResAttrName(R_BITRATE),
                                               String::from(temp)); 
        }

        if ((header->time) > 0)
        {
            item->getResource(0)->addAttribute(MetadataHandler::getResAttrName(R_DURATION),
                                               secondsToHMS(header->time));
        }

        if ((header->frequency) > 0)
        {
            item->getResource(0)->addAttribute(MetadataHandler::getResAttrName(R_SAMPLEFREQUENCY), 
                                               String::from((int)(header->frequency)));
        }

        temp = 0;
        switch(header->channelmode)
        {
            case MP3CHANNELMODE_STEREO:
            case MP3CHANNELMODE_JOINT_STEREO:
            case MP3CHANNELMODE_DUAL_CHANNEL:
                temp = 2;
                break;
            case MP3CHANNELMODE_SINGLE_CHANNEL:
                temp = 1;
        }

        if (temp > 0)
        {
            item->getResource(0)->addAttribute(MetadataHandler::getResAttrName(R_NRAUDIOCHANNELS),
                                               String::from(temp));
        }
    }
    
    tag.Clear();
}

Ref<IOHandler> Id3Handler::serveContent(Ref<CdsItem> item, int resNum)
{
    return nil;
}

#endif // HAVE_ID3

