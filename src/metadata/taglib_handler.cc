/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    taglib_handler.cc - this file is part of MediaTomb.
    
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

/// \file taglib_handler.cc
/// \brief Implementeation of the TagHandler class.

#ifdef HAVE_CONFIG_H
    #include "autoconfig.h"
#endif

#ifdef HAVE_TAGLIB

#include <taglib.h>
#include <fileref.h>
#include <tag.h>
#include <id3v2tag.h>
#include <mpegfile.h>
#include <audioproperties.h>
#include <attachedpictureframe.h>
#include <textidentificationframe.h>
#include <tstring.h>

#include "taglib_handler.h"
#include "string_converter.h"
#include "config_manager.h"
#include "common.h"
#include "tools.h"
#include "mem_io_handler.h"

#include "content_manager.h"

using namespace zmm;

TagHandler::TagHandler() : MetadataHandler()
{
}
       
static void addField(metadata_fields_t field, TagLib::Tag *tag, Ref<CdsItem> item)
{
    TagLib::String val;
    String value;
    unsigned int i;
    
    if (tag == NULL) 
        return;

    if (tag->isEmpty())
        return;

    Ref<StringConverter> sc = StringConverter::i2i(); // sure is sure
    
    switch (field)
    {
        case M_TITLE:
            val = tag->title();
            break;
        case M_ARTIST:
            val = tag->artist();
            break;
        case M_ALBUM:
            val = tag->album();
            break;
        case M_DATE:
            i = tag->year();
            if (i > 0)
            {
                value = String::from(i);

                if (string_ok(value))
                    value = value + _("-01-01");
            }
            else
                return;
            break;
        case M_GENRE:
            val = tag->genre();
            break;
        case M_DESCRIPTION:
            val = tag->comment();
            break;
        case M_TRACKNUMBER:
            i = tag->track();
            if (i > 0)
            {
                value = String::from(i);
                item->setTrackNumber((int)i);
            }
            else
                return;
            break;
        default:
            return;
    }

    if ((field != M_DATE) && (field != M_TRACKNUMBER))
        value = val.toCString(true);

    value = trim_string(value);
    
    if (string_ok(value))
    {
        item->setMetadata(MT_KEYS[field].upnp, sc->convert(value));
//        log_debug("Setting metadata on item: %d, %s\n", field, sc->convert(value).c_str());
    }
}

void TagHandler::fillMetadata(Ref<CdsItem> item)
{
    Ref<Array<StringBase> > aux;
    Ref<StringConverter> sc = StringConverter::i2i();

    TagLib::FileRef f(item->getLocation().c_str());

    if (f.isNull() || (!f.tag()))
        return;


    TagLib::Tag *tag = f.tag();

    for (int i = 0; i < M_MAX; i++)
        addField((metadata_fields_t) i, tag, item);

    int temp;
    
    TagLib::AudioProperties *audioProps = f.audioProperties();
    
    if (audioProps == NULL) 
        return;
    
    // note: UPnP requres bytes/second
    temp = audioProps->bitrate() * 1024 / 8;

    if (temp > 0)
    {
        item->getResource(0)->addAttribute(MetadataHandler::getResAttrName(R_BITRATE),
                                           String::from(temp)); 
    }

    temp = audioProps->length();
    if (temp > 0)
    {
        item->getResource(0)->addAttribute(MetadataHandler::getResAttrName(R_DURATION),
                                           secondsToHMS(temp));
    }

    temp = audioProps->sampleRate();
    if (temp > 0)
    {
        item->getResource(0)->addAttribute(MetadataHandler::getResAttrName(R_SAMPLEFREQUENCY), 
                                           String::from(temp));
    }

    temp = audioProps->channels();

    if (temp > 0)
    {
        item->getResource(0)->addAttribute(MetadataHandler::getResAttrName(R_NRAUDIOCHANNELS),
                                           String::from(temp));
    }

    Ref<Dictionary> mappings = ConfigManager::getInstance()->getDictionaryOption(CFG_IMPORT_MAPPINGS_MIMETYPE_TO_CONTENTTYPE_LIST);
    String content_type = mappings->get(item->getMimeType());
    // we are done here, album art can only be retrieved from id3v2
    if (content_type != CONTENT_TYPE_MP3)
        return;

    // did not yet found a way on how to get to the picture from the file
    // reference that we already have
    TagLib::MPEG::File mp(item->getLocation().c_str());

    if (!mp.isValid() || !mp.ID3v2Tag())
        return;

    //log_debug("Tag contains %i frame(s)\n", mp.ID3v2Tag()->frameListMap().size());
    Ref<ConfigManager> cm = ConfigManager::getInstance();
    aux = cm->getStringArrayOption(CFG_IMPORT_LIBOPTS_ID3_AUXDATA_TAGS_LIST);
    if (aux != nil) 
    {
        for (int j = 0; j < aux->size(); j++) 
        {
            String desiredFrame = aux->get(j);
            if (string_ok(desiredFrame)) 
            {
                const TagLib::ID3v2::FrameList& allFrames = mp.ID3v2Tag()->frameList();
                if (!allFrames.isEmpty()) 
                {
                    TagLib::ID3v2::FrameList::ConstIterator it = allFrames.begin();
                    for (; it != allFrames.end(); it++) 
                    {
                        TagLib::String frameID((*it)->frameID(), TagLib::String::Latin1);
                        //log_debug("Found frame: %s\n", frameID.toCString(true));

                        TagLib::ID3v2::TextIdentificationFrame *textFrame =
                            dynamic_cast<TagLib::ID3v2::TextIdentificationFrame *>(*it);

                        if (textFrame) 
                        {
                            //log_debug("We have a TextIdentificationFrame\n");

                            if (frameID == desiredFrame.c_str()) 
                            {
                                TagLib::String frameContents = textFrame->toString();
                                //log_debug("We have a match!!!!: %s\n", frameContents.toCString(true));
                                String value(frameContents.toCString(true), frameContents.size());
                                value = sc->convert(value);
                                log_debug("Adding frame: %s with value %s\n", desiredFrame.c_str(), value.c_str());
                                item->setAuxData(desiredFrame, value);
                            }
                        }
                    }
                }
            }
        }
    }

    TagLib::ID3v2::FrameList list = mp.ID3v2Tag()->frameList("APIC");
    if (!list.isEmpty())
    {
        TagLib::ID3v2::AttachedPictureFrame *art =
               static_cast<TagLib::ID3v2::AttachedPictureFrame *>(list.front());

        if (art->picture().size() < 1)
            return;

        String art_mimetype = sc->convert(art->mimeType().toCString(true));
        // saw that simply "PNG" was used with some mp3's, so mimetype setting
        // was probably invalid
        if (!string_ok(art_mimetype) || (art_mimetype.index('/') == -1))
        {
#ifdef HAVE_MAGIC
            art_mimetype =  ContentManager::getInstance()->getMimeTypeFromBuffer((void *)art->picture().data(), art->picture().size());
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

Ref<IOHandler> TagHandler::serveContent(Ref<CdsItem> item, int resNum, off_t *data_size)
{
    // we should only get mp3 files here
    TagLib::MPEG::File f(item->getLocation().c_str());

    if (!f.isValid())
        throw _Exception(_("TagHandler: could not open file: ") + 
                item->getLocation());


    if (!f.ID3v2Tag())
        throw _Exception(_("TagHandler: resource has no album information"));

    TagLib::ID3v2::FrameList list = f.ID3v2Tag()->frameList("APIC");
    if (list.isEmpty())
        throw _Exception(_("TagHandler: resource has no album information"));

    TagLib::ID3v2::AttachedPictureFrame *art =
        static_cast<TagLib::ID3v2::AttachedPictureFrame *>(list.front());

    Ref<IOHandler> h(new MemIOHandler((void *)art->picture().data(), 
                art->picture().size()));

    *data_size = art->picture().size();
    return h;
}

#endif // HAVE_TAGLIB
