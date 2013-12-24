/*MT*

    MediaTomb - http://www.mediatomb.cc/

    flac_handler.cc - this file is part of MediaTomb.

    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>

    Copyright (C) 2006-2009 Gena Batyan <bgeradz@mediatomb.cc>,
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

    $Id: flac_handler.cc 2032 2009-12-21 22:58:57Z lww $
*/

/// \file flac_handler.cc
/// \brief Implementeation of the FlacHandler class.

#ifdef HAVE_CONFIG_H
    #include "autoconfig.h"
#endif

#ifdef HAVE_FLAC

#include <FLAC/all.h>

#include "flac_handler.h"
#include "string_converter.h"
#include "config_manager.h"
#include "common.h"
#include "tools.h"
#include "mem_io_handler.h"

#include "content_manager.h"

using namespace zmm;

FlacHandler::FlacHandler() : MetadataHandler()
{
}

static void addField(metadata_fields_t field, const FLAC__StreamMetadata* tags, Ref<CdsItem> item)
{
    String value;
    int i;

    Ref<StringConverter> sc = StringConverter::i2i(); // sure is sure

    switch (field)
    {
        case M_TITLE:
            i = FLAC__metadata_object_vorbiscomment_find_entry_from(tags, /*offset=*/0, "TITLE");
            break;
        case M_ARTIST:
            i = FLAC__metadata_object_vorbiscomment_find_entry_from(tags, /*offset=*/0, "ARTIST");
            break;
        case M_ALBUM:
            i = FLAC__metadata_object_vorbiscomment_find_entry_from(tags, /*offset=*/0, "ALBUM");
            break;
        case M_DATE:
            i = FLAC__metadata_object_vorbiscomment_find_entry_from(tags, /*offset=*/0, "DATE");
            break;
        case M_GENRE:
            i = FLAC__metadata_object_vorbiscomment_find_entry_from(tags, /*offset=*/0, "GENRE");
            break;
        case M_DESCRIPTION:
            i = FLAC__metadata_object_vorbiscomment_find_entry_from(tags, /*offset=*/0, "DESCRIPTION");
            break;
        case M_TRACKNUMBER:
            i = FLAC__metadata_object_vorbiscomment_find_entry_from(tags, /*offset=*/0, "TRACKNUMBER");
            break;
        default:
            return;
    }

    if( 0 <= i )
        value = strchr((const char *)tags->data.vorbis_comment.comments[i].entry, '=') + 1;
    else
        return;

    value = trim_string(value);

    if (string_ok(value))
    {
        item->setMetadata(MT_KEYS[field].upnp, sc->convert(value));
        log_debug("Setting metadata on item: %d, %s\n", field, sc->convert(value).c_str());
    }
}

void FlacHandler::fillMetadata(Ref<CdsItem> item)
{
    FLAC__StreamMetadata* tags = NULL;
    FLAC__StreamMetadata streaminfo;
    Ref<StringConverter> sc = StringConverter::i2i(); // sure is sure

    if (!FLAC__metadata_get_tags(item->getLocation().c_str(), &tags))
        return;

    if (FLAC__METADATA_TYPE_VORBIS_COMMENT == tags->type)
    {
        for (int i = 0; i < M_MAX; i++)
            addField((metadata_fields_t) i, tags, item);
    }

    FLAC__metadata_object_delete(tags);
    tags = NULL;

    if (!FLAC__metadata_get_streaminfo(item->getLocation().c_str(), &streaminfo))
        return;

    if (FLAC__METADATA_TYPE_STREAMINFO == streaminfo.type)
    {
        // note: UPnP requires bytes/second
        item->getResource(0)->addAttribute(MetadataHandler::getResAttrName(R_BITRATE), String::from((unsigned)((streaminfo.data.stream_info.bits_per_sample * streaminfo.data.stream_info.sample_rate) / 8)));
        // note: UPnP requires HMS
        item->getResource(0)->addAttribute(MetadataHandler::getResAttrName(R_DURATION), secondsToHMS((unsigned)(streaminfo.data.stream_info.total_samples / streaminfo.data.stream_info.sample_rate)));
        item->getResource(0)->addAttribute(MetadataHandler::getResAttrName(R_SAMPLEFREQUENCY), String::from(streaminfo.data.stream_info.sample_rate));
        item->getResource(0)->addAttribute(MetadataHandler::getResAttrName(R_NRAUDIOCHANNELS), String::from(streaminfo.data.stream_info.channels));
    }

    if (!FLAC__metadata_get_picture(item->getLocation().c_str(),
                                    &tags,
                                    FLAC__STREAM_METADATA_PICTURE_TYPE_FRONT_COVER,
                                    NULL,
                                    NULL,
                                    (unsigned)-1,
                                    (unsigned)-1,
                                    (unsigned)-1,
                                    (unsigned)-1 ))
        return;

    if (FLAC__METADATA_TYPE_PICTURE == tags->type)
    {
        String art_mimetype = tags->data.picture.mime_type;
        log_debug("Mime type : %s\n", sc->convert(art_mimetype).c_str());

        // saw that simply "PNG" was used with some mp3's, so mimetype setting
        // was probably invalid
        if (!string_ok(art_mimetype) || (art_mimetype.index('/') == -1))
        {
#ifdef HAVE_MAGIC
            art_mimetype =  ContentManager::getInstance()->getMimeTypeFromBuffer((void *)tags->data.picture.data, tags->data.picture.data_length);
            if (!string_ok(art_mimetype))
#endif
                art_mimetype = _(MIMETYPE_DEFAULT);

            log_debug("Mime type via magic: %s\n", sc->convert(art_mimetype).c_str());
        }

        // if we could not determine the mimetype, then there is no
        // point to add the resource - it's probably garbage
        if (art_mimetype != _(MIMETYPE_DEFAULT))
        {
            Ref<CdsResource> resource(new CdsResource(CH_FLAC));
            resource->addAttribute(MetadataHandler::getResAttrName(R_PROTOCOLINFO), renderProtocolInfo(art_mimetype));
            resource->addParameter(_(RESOURCE_CONTENT_TYPE), _(ID3_ALBUM_ART));
            item->addResource(resource);
        }
    }

    FLAC__metadata_object_delete(tags);
}

Ref<IOHandler> FlacHandler::serveContent(Ref<CdsItem> item, int resNum, off_t *data_size)
{
    FLAC__StreamMetadata* picture = NULL;

    if (!FLAC__metadata_get_picture(item->getLocation().c_str(),
                                    &picture,
                                    FLAC__STREAM_METADATA_PICTURE_TYPE_FRONT_COVER,
                                    NULL,
                                    NULL,
                                    (unsigned)-1,
                                    (unsigned)-1,
                                    (unsigned)-1,
                                    (unsigned)-1 ))
        throw _Exception(_("FlacHandler: could not exctract cover from: ") + item->getLocation());

    if ( FLAC__METADATA_TYPE_PICTURE != picture->type)
        throw _Exception(_("TagHandler: resource has no album information"));

    Ref<IOHandler> h(new MemIOHandler((void *)picture->data.picture.data, picture->data.picture.data_length));
    *data_size = picture->data.picture.data_length;

    FLAC__metadata_object_delete(picture);

    return h;
}

#endif // HAVE_FLAC
