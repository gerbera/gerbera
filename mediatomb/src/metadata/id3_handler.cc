/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    id3_handler.cc - this file is part of MediaTomb.
    
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

/// \file id3_handler.cc
/// \brief Implementeation of the Id3Handler class.

#ifdef HAVE_CONFIG_H
    #include "autoconfig.h"
#endif

#ifdef HAVE_ID3LIB

#ifdef HAVE_CONFIG_H
    #undef HAVE_CONFIG_H // else utils.h from the id3 library tries to import "config.h"

    #include <id3/tag.h>
    #include <id3/misc_support.h>
    #define HAVE_CONFIG_H
#else
    #include <id3/tag.h>
    #include <id3/misc_support.h>
#endif

#include "id3_handler.h"
#include "string_converter.h"
#include "common.h"
#include "tools.h"
#include "mem_io_handler.h"
#include "content_manager.h"
#include "config_manager.h"

using namespace zmm;

Id3Handler::Id3Handler() : MetadataHandler()
{
}

char* ID3_GetFrameContent(const ID3_Tag *tag, ID3_FrameID frameID)
{
    char *content = NULL;
    ID3_Frame *frame = NULL;
    if (tag != NULL && (frame = tag->Find(frameID)) != NULL) 
    {
	    content = ID3_GetString(frame, ID3FN_TEXT);
    }
    return content;
}
       
static void addID3Field(metadata_fields_t field, ID3_Tag *tag, Ref<CdsItem> item)
{
    String value;
    char*  ID3_retval = NULL;
  
    Ref<StringConverter> sc = StringConverter::m2i();
    size_t genre;
#if SIZEOF_SIZE_T > 4
    long track;
#else
    int track;
#endif
    
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
            value = ID3_retval;
            if (string_ok(value))
                value = value + "-01-01";
            else
                return;
            break;
        case M_GENRE:
            genre = ID3_GetGenreNum(tag);
            if (ID3_V1GENRE2DESCRIPTION(genre))
                value = (char *)(ID3_V1GENRE2DESCRIPTION(genre));

            if (!string_ok(value))
            {
                ID3_retval = ID3_GetGenre(tag); 
                value = ID3_retval;
            }
            break;
        case M_DESCRIPTION:
            ID3_retval = ID3_GetComment(tag);
            break;
        case M_TRACKNUMBER:
            track = ID3_GetTrackNum(tag);
            if (track > 0)
            {
                value = String::from(track);
                item->setTrackNumber((int)track);
            }
            else
                return;
            break;
        case M_AUTHOR:
            ID3_retval = ID3_GetFrameContent(tag, ID3FID_COMPOSER);
            break;
        case M_DIRECTOR:
            ID3_retval = ID3_GetFrameContent(tag, ID3FID_CONDUCTOR);
            break;
//        case M_OPUS:
//            ID3_retval = ID3_GetFrameContent(tag, ID3FID_CONTENTGROUP);
//            break;
        default:
            return;
    }

    if ((field != M_GENRE) && (field != M_DATE) && (field != M_TRACKNUMBER))
        value = ID3_retval;
    
    if (ID3_retval)
        delete [] ID3_retval;

    value = trim_string(value);
    
    if (string_ok(value))
    {
        item->setMetadata(MT_KEYS[field].upnp, sc->convert(value));
//        log_debug("Setting metadata on item: %d, %s\n", field, sc->convert(value).c_str());
    }
}

void Id3Handler::fillMetadata(Ref<CdsItem> item)
{
    ID3_Tag tag;
    const Mp3_Headerinfo* header;
    Ref<Array<StringBase> > aux;
    Ref<StringConverter> sc = StringConverter::m2i();
    
    // the location has already been checked by the setMetadata function
    tag.Link(item->getLocation().c_str()); 

    for (int i = 0; i < M_MAX; i++)
        addID3Field((metadata_fields_t) i, &tag, item);

    Ref<ConfigManager> cm = ConfigManager::getInstance();
    aux = cm->getStringArrayOption(CFG_IMPORT_LIBOPTS_ID3_AUXDATA_TAGS_LIST);
    if (aux != nil)
    {
        const char *temp = NULL;
        
        for (int j = 0; j < aux->size(); j++)
        {

            String desiredFrame(aux->get(j));
            if (string_ok(desiredFrame))
            {
                ID3_Tag::Iterator* frameIter = tag.CreateIterator();
                ID3_Frame* id3Frame = NULL;
                while (NULL != (id3Frame = frameIter->GetNext())) 
                {
                    String frameName(id3Frame->GetTextID());
                    if (string_ok(frameName) && (frameName == desiredFrame)) 
                    {
                        ID3_Frame::Iterator* fieldIter = id3Frame->CreateIterator();
                        ID3_Field* id3Field = NULL;
                        while (NULL != (id3Field = fieldIter->GetNext())) 
                        {
                            if (id3Field->GetType() == ID3FTY_TEXTSTRING) 
                            {
                                temp = id3Field->GetRawText();

                                if (temp != NULL)
                                {
                                    String value(temp);
                                    if (string_ok(value))
                                    {
                                        value = sc->convert(value);
                                        log_debug("Adding frame: %s with value %s\n", desiredFrame.c_str(), value.c_str());
                                        item->setAuxData(desiredFrame, value);
                                    }
                                }
                            }
                        }
                        delete fieldIter;
                    }
                }
                delete frameIter;
            }
        }
    }


    header = tag.GetMp3HeaderInfo();
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
                break;
            case MP3CHANNELMODE_FALSE:
                break;
        }

        if (temp > 0)
        {
            item->getResource(0)->addAttribute(MetadataHandler::getResAttrName(R_NRAUDIOCHANNELS),
                                               String::from(temp));
        }
    }

#ifdef HAVE_ID3LIB_ALBUMART
    // get album art
    // we have a bit of design problem here - the album art is actually not
    // a resource, but our architecture is built that way that we can only
    // serve resources, so we are going to bend this a little:
    // the album art will be saved as a resource, but the CdsResourceManager
    // will handle this special case and render the XML correctly...
    // \todo discuss the album art approach with Leo
    if (ID3_HasPicture(&tag))
    {
        ID3_Frame* frame = NULL;
        frame = tag.Find(ID3FID_PICTURE);
        if (frame != NULL)
        {
            ID3_Field* art = frame->GetField(ID3FN_DATA);
            if (art != NULL)
            {
                Ref<StringConverter> sc = StringConverter::m2i(); 
                String art_mimetype = sc->convert(ID3_GetPictureMimeType(&tag));
                if (!string_ok(art_mimetype) || (art_mimetype.index('/') == -1))
                {
#ifdef HAVE_MAGIC
                    art_mimetype = ContentManager::getInstance()->getMimeTypeFromBuffer((void *)art->GetRawBinary(), art->Size());
                    if (!string_ok(art_mimetype))
#endif
                       art_mimetype = _(MIMETYPE_DEFAULT);
                }

                // if we could not determine the mimetype, then there is no
                // point to add the resource - it's probably garbage
                if (art_mimetype != _(MIMETYPE_DEFAULT))
                {
                    Ref<CdsResource> resource(new CdsResource(CH_ID3));
                    resource->addAttribute(MetadataHandler::getResAttrName(R_PROTOCOLINFO), renderProtocolInfo(art_mimetype));
                    resource->addParameter(_(RESOURCE_CONTENT_TYPE), _(ID3_ALBUM_ART));
                    item->addResource(resource);
                }

            }
        }
    }
#endif
    tag.Clear();
}

Ref<IOHandler> Id3Handler::serveContent(Ref<CdsItem> item, int resNum, off_t *data_size)
{
    ID3_Tag tag;
    if (tag.Link(item->getLocation().c_str()) == 0)
    {
        throw _Exception(_("Id3Handler: could not open file: ") + item->getLocation());
    }

    Ref<CdsResource> res = item->getResource(resNum);

    String ctype = res->getParameters()->get(_(RESOURCE_CONTENT_TYPE));

    if (ctype != ID3_ALBUM_ART)
        throw _Exception(_("Id3Handler: got unknown content type: ") + ctype);

    if (!ID3_HasPicture(&tag))
        throw _Exception(_("Id3Handler: resource has no album art information"));

    ID3_Frame* frame = NULL;
    frame = tag.Find(ID3FID_PICTURE);

    if (frame == NULL)
        throw _Exception(_("Id3Handler: could not server album art - empty frame"));

    ID3_Field* art = frame->GetField(ID3FN_DATA);
    if (art == NULL)
        throw _Exception(_("Id3Handler: could not server album art - empty field"));

    Ref<IOHandler> h(new MemIOHandler((void *)art->GetRawBinary(), art->Size()));
    *data_size = art->Size();
    tag.Clear();

    return h;
}

#endif // HAVE_ID3LIB
