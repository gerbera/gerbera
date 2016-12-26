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

#ifdef HAVE_TAGLIB

#include <taglib/attachedpictureframe.h>
#include <taglib/aifffile.h>
#include <taglib/apefile.h>
#include <taglib/asffile.h>
#include <taglib/flacfile.h>
#include <taglib/id3v2tag.h>
#include <taglib/mp4file.h>
#include <taglib/mpegfile.h>
#include <taglib/textidentificationframe.h>
#include <taglib/vorbisfile.h>
#include <taglib/wavpackfile.h>

#include "config_manager.h"
#include "mem_io_handler.h"
#include "string_converter.h"
#include "taglib_handler.h"

#include "content_manager.h"

using namespace zmm;

TagLibHandler::TagLibHandler() : MetadataHandler()
{
}

static void addField(metadata_fields_t field, const TagLib::Tag* tag, Ref<CdsItem> item)
{
    if (tag == nullptr)
        return;

    if (tag->isEmpty())
        return;

    Ref<StringConverter> sc = StringConverter::i2i(); // sure is sure

    TagLib::String val;
    String value;
    unsigned int i;

    switch (field) {
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
        if (i > 0) {
            value = String::from(i);

            if (string_ok(value))
                value = value + _("-01-01");
        } else
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
        if (i > 0) {
            value = String::from(i);
            item->setTrackNumber((int)i);
        } else
            return;
        break;
    default:
        return;
    }

    if ((field != M_DATE) && (field != M_TRACKNUMBER))
        value = val.toCString(true);

    value = trim_string(value);

    if (string_ok(value)) {
        item->setMetadata(MT_KEYS[field].upnp, sc->convert(value));
        //        log_debug("Setting metadata on item: %d, %s\n", field, sc->convert(value).c_str());
    }
}

void TagLibHandler::populateGenericTags(Ref<CdsItem> item, const TagLib::File& file) const
{
    if (!file.tag())
        return;

    const TagLib::Tag* tag = file.tag();

    for (int i = 0; i < M_MAX; i++)
        addField((metadata_fields_t)i, tag, item);

    int temp;

    const TagLib::AudioProperties* audioProps = file.audioProperties();

    if (!audioProps)
        return;

    // note: UPnP requres bytes/second
    temp = audioProps->bitrate() * 1024 / 8;

    Ref<CdsResource> res = item->getResource(0);

    if (temp > 0) {
        res->addAttribute(MetadataHandler::getResAttrName(R_BITRATE),
            String::from(temp));
    }

    temp = audioProps->length();
    if (temp > 0) {
        res->addAttribute(MetadataHandler::getResAttrName(R_DURATION),
            secondsToHMS(temp));
    }

    temp = audioProps->sampleRate();
    if (temp > 0) {
        res->addAttribute(MetadataHandler::getResAttrName(R_SAMPLEFREQUENCY),
            String::from(temp));
    }

    temp = audioProps->channels();
    if (temp > 0) {
        res->addAttribute(MetadataHandler::getResAttrName(R_NRAUDIOCHANNELS),
            String::from(temp));
    }
}

void TagLibHandler::fillMetadata(Ref<CdsItem> item)
{
    Ref<Dictionary> mappings = ConfigManager::getInstance()->getDictionaryOption(CFG_IMPORT_MAPPINGS_MIMETYPE_TO_CONTENTTYPE_LIST);
    String content_type = mappings->get(item->getMimeType());

    if (content_type == CONTENT_TYPE_MP3) {
        extractMP3(item);
    } else if (content_type == CONTENT_TYPE_FLAC) {
       extractFLAC(item);
    } else if (content_type == CONTENT_TYPE_MP4) {
        extractMP4(item);
    } else if (content_type == CONTENT_TYPE_OGG) {
        extractOgg(item);
    } else if (content_type == CONTENT_TYPE_APE) {
        extractAPE(item);
    } else if (content_type == CONTENT_TYPE_WMA) {
        extractASF(item);
    } else if (content_type == CONTENT_TYPE_WAVPACK) {
        extractWavPack(item);
    } else if (content_type == CONTENT_TYPE_AIFF) {
        extractAiff(item);
    } else {
        log_warning("TagLibHandler does not handle the %s content type\n", content_type.c_str());
    }
    log_debug("TagLib handler done.\n");
}

bool TagLibHandler::isValidArtworkContentType(zmm::String art_mimetype)
{
    // saw that simply "PNG" was used with some mp3's, so mimetype setting
    // was probably invalid
    return (string_ok(art_mimetype) && (art_mimetype.index('/') != -1));
}

String TagLibHandler::getContentTypeFromByteVector(const TagLib::ByteVector& data) const
{
    String art_mimetype = _(MIMETYPE_DEFAULT);
#ifdef HAVE_MAGIC
    art_mimetype = ContentManager::getInstance()->getMimeTypeFromBuffer(data.data(), data.size());
    if (!string_ok(art_mimetype))
        return _(MIMETYPE_DEFAULT);
#endif
    return art_mimetype;
}

void TagLibHandler::addArtworkResource(Ref<CdsItem> item, String art_mimetype)
{
    // if we could not determine the mimetype, then there is no
    // point to add the resource - it's probably garbage
    log_debug("Found artwork of type %s in file %s\n", art_mimetype.c_str(), item->getLocation().c_str());

    if (art_mimetype != _(MIMETYPE_DEFAULT)) {
        Ref<CdsResource> resource(new CdsResource(CH_ID3));
        resource->addAttribute(MetadataHandler::getResAttrName(
                                   R_PROTOCOLINFO),
            renderProtocolInfo(art_mimetype));
        resource->addParameter(_(RESOURCE_CONTENT_TYPE),
            _(ID3_ALBUM_ART));
        item->addResource(resource);
    }
}

Ref<IOHandler> TagLibHandler::serveContent(IN Ref<CdsItem> item, IN int resNum, OUT off_t* data_size)
{
    Ref<Dictionary> mappings = ConfigManager::getInstance()->getDictionaryOption(CFG_IMPORT_MAPPINGS_MIMETYPE_TO_CONTENTTYPE_LIST);
    String content_type = mappings->get(item->getMimeType());

    if (content_type == CONTENT_TYPE_MP3) {
        TagLib::MPEG::File f(item->getLocation().c_str());

        if (!f.isValid())
            throw _Exception(_("TagLibHandler: could not open file: ") + item->getLocation());

        if (!f.ID3v2Tag())
            throw _Exception(_("TagLibHandler: resource has no album information"));

        TagLib::ID3v2::FrameList list = f.ID3v2Tag()->frameList("APIC");
        if (list.isEmpty())
            throw _Exception(_("TagLibHandler: resource has no album information"));

        TagLib::ID3v2::AttachedPictureFrame* art = static_cast<TagLib::ID3v2::AttachedPictureFrame*>(list.front());

        Ref<IOHandler> h(new MemIOHandler((void*)art->picture().data(),
            art->picture().size()));

        *data_size = art->picture().size();
        return h;

    } else if (content_type == CONTENT_TYPE_FLAC) {
        TagLib::FLAC::File f(item->getLocation().c_str());

        if (!f.isValid())
            throw _Exception(_("TagLibHandler: could not open flac file: ") + item->getLocation());

        if (f.pictureList().isEmpty())
            throw _Exception(_("TagLibHandler: flac resource has no picture information"));

        TagLib::FLAC::Picture* pic = f.pictureList().front();
        const TagLib::ByteVector& data = pic->data();

        Ref<IOHandler> h(new MemIOHandler(data.data(), data.size()));

        *data_size = data.size();
        return h;

    } else if (content_type == CONTENT_TYPE_MP4) {
        TagLib::MP4::File f(item->getLocation().c_str());

        if (!f.isValid()) {
            throw _Exception(_("TagLibHandler: could not open mp4 file: ") + item->getLocation());
        }

        if (!f.hasMP4Tag()) {
            throw _Exception(_("TagLibHandler: mp4 resource has no tag information"));
        }

        String art_mimetype;

        const TagLib::MP4::ItemListMap& itemsListMap = f.tag()->itemListMap();
        const TagLib::MP4::Item& coverItem = itemsListMap["covr"];
        const TagLib::MP4::CoverArtList& coverArtList = coverItem.toCoverArtList();
        if (coverArtList.isEmpty()) {
            throw _Exception(_("TagLibHandler: mp4 resource has no picture information"));
        }

        const TagLib::MP4::CoverArt& coverArt = coverArtList.front();
        const TagLib::ByteVector& data = coverArt.data();

        Ref<IOHandler> h(new MemIOHandler(data.data(), data.size()));

        *data_size = data.size();
        return h;
    } else if (content_type == CONTENT_TYPE_WMA) {
        TagLib::ASF::File f(item->getLocation().c_str());

        if (!f.isValid())
            throw _Exception(_("TagLibHandler: could not open flac file: ") + item->getLocation());

        const TagLib::ASF::AttributeListMap& attrListMap = f.tag()->attributeListMap();
        if (!attrListMap.contains("WM/Picture"))
            throw _Exception(_("TagLibHandler: wma file has no picture information"));

        const TagLib::ASF::AttributeList& attrList = attrListMap["WM/Picture"];
        if (attrList.isEmpty())
            throw _Exception(_("TagLibHandler: wma list has no picture information"));

        const TagLib::ASF::Picture& wmpic = attrList[0].toPicture();
        if (!wmpic.isValid())
            throw _Exception(_("TagLibHandler: wma pic not valid"));

        const TagLib::ByteVector& data = wmpic.picture();

        Ref<IOHandler> h(new MemIOHandler(data.data(), data.size()));
        *data_size = data.size();
        return h;
    } else if (content_type == CONTENT_TYPE_OGG) {
        TagLib::Ogg::Vorbis::File f(item->getLocation().c_str());

        if (!f.isValid() || !f.tag())
            throw _Exception(_("TagLibHandler: could not open vorbis file: ") + item->getLocation());

        const TagLib::List<TagLib::FLAC::Picture *> picList = f.tag()->pictureList();
        if (picList.isEmpty())
            throw _Exception(_("TagLibHandler: vorbis file has no picture information"));

        const TagLib::FLAC::Picture *pic = picList.front();
        const TagLib::ByteVector& data = pic->data();

        Ref<IOHandler> h(new MemIOHandler(data.data(), data.size()));
        *data_size = data.size();
        return h;
    }

    throw _Exception(_("TagLibHandler: Unsupported content_type: ") + content_type);
}

void TagLibHandler::extractMP3(zmm::Ref<CdsItem> item) {
    TagLib::MPEG::File mp(item->getLocation().c_str());

    if (!mp.isValid() || !mp.hasID3v2Tag()) {
        log_debug("TagLibHandler: could not open mp3 file: %s\n",
                  item->getLocation().c_str());
        return;
    }
    populateGenericTags(item, mp);

    Ref<StringConverter> sc = StringConverter::i2i();

    Ref<Array<StringBase> > aux_tags_list = ConfigManager::getInstance()->getStringArrayOption(CFG_IMPORT_LIBOPTS_ID3_AUXDATA_TAGS_LIST);
    if (aux_tags_list != nullptr) {
        for (int j = 0; j < aux_tags_list->size(); j++) {

            String desiredFrame = aux_tags_list->get(j);
            if (!string_ok(desiredFrame)) {
                continue;
            }

            auto frameListMap = mp.ID3v2Tag()->frameListMap();

            if (frameListMap.contains(desiredFrame.c_str())) {
                const auto frameList = frameListMap[desiredFrame.c_str()];
                if (frameList.isEmpty())
                    continue;

                const TagLib::ID3v2::Frame *frame = frameList.front();
                const auto textFrame = static_cast<const TagLib::ID3v2::TextIdentificationFrame*>(frame);

                const TagLib::String frameContents = textFrame->toString();
                String value(frameContents.toCString(true), frameContents.size());
                value = sc->convert(value);
                log_debug(
                    "Adding frame: %s with value %s\n", desiredFrame.c_str(),
                    value.c_str());
                item->setAuxData(desiredFrame, value);
            }
        }
    }

    const TagLib::ID3v2::FrameList apicFrameList = mp.ID3v2Tag()->frameList("APIC");
    if (!apicFrameList.isEmpty()) {
        auto art = static_cast<const TagLib::ID3v2::AttachedPictureFrame*>(apicFrameList.front());

        const TagLib::ByteVector pic = art->picture();
        String art_mimetype = sc->convert(art->mimeType().toCString(true));
        if (!isValidArtworkContentType(art_mimetype)) {
            art_mimetype = getContentTypeFromByteVector(pic);
        }

        addArtworkResource(item, art_mimetype);
    }
}

void TagLibHandler::extractOgg(zmm::Ref<CdsItem> item) {
    TagLib::Ogg::Vorbis::File f(item->getLocation().c_str());

    if (!f.isValid()) {
        log_debug("TagLibHandler: could not open ogg file: %s\n",
                  item->getLocation().c_str());
        return;
    }
    populateGenericTags(item, f);

    if (!f.tag())
        return;

    // Vorbis uses the FLAC binary picture structure...
    // https://wiki.xiph.org/VorbisComment#Cover_art
    // The unofficial COVERART field is not supported.
    const TagLib::List<TagLib::FLAC::Picture *> picList = f.tag()->pictureList();
    if (picList.isEmpty())
        return;

    const TagLib::FLAC::Picture *pic = picList.front();
    const TagLib::ByteVector& data = pic->data();

    Ref<StringConverter> sc = StringConverter::i2i();
    String art_mimetype = sc->convert(pic->mimeType().toCString(true));
    if (!isValidArtworkContentType(art_mimetype)) {
        art_mimetype = getContentTypeFromByteVector(data);
    }
    addArtworkResource(item, art_mimetype);
}

void TagLibHandler::extractASF(zmm::Ref<CdsItem> item) {
    TagLib::ASF::File f(item->getLocation().c_str());

    if (!f.isValid()) {
        log_debug("TagLibHandler: could not open asf/wma file: %s\n",
                  item->getLocation().c_str());
        return;
    }
    populateGenericTags(item, f);

    const TagLib::ASF::AttributeListMap& attrListMap = f.tag()->attributeListMap();
    if (attrListMap.contains("WM/Picture")) {
        const TagLib::ASF::AttributeList& attrList = attrListMap["WM/Picture"];
        if (attrList.isEmpty())
            return;

        const TagLib::ASF::Picture& wmpic = attrList[0].toPicture();
        if (!wmpic.isValid())
            return;

        Ref<StringConverter> sc = StringConverter::i2i();
        String art_mimetype = sc->convert(wmpic.mimeType().toCString(true));
        if (!isValidArtworkContentType(art_mimetype)) {
            art_mimetype = getContentTypeFromByteVector(wmpic.picture());
        }
        addArtworkResource(item, art_mimetype);
    }
}

void TagLibHandler::extractFLAC(zmm::Ref<CdsItem> item) {
    TagLib::FLAC::File f(item->getLocation().c_str());

    if (!f.isValid()) {
        log_debug("TagLibHandler: could not open flac file: %s\n",
                  item->getLocation().c_str());
        return;
    }
    populateGenericTags(item, f);

    if (f.pictureList().isEmpty()) {
        log_debug("TagLibHandler: flac resource has no picture information\n");
        return;
    }
    const TagLib::FLAC::Picture *pic = f.pictureList().front();
    const TagLib::ByteVector& data = pic->data();

    Ref<StringConverter> sc = StringConverter::i2i();
    String art_mimetype = sc->convert(pic->mimeType().toCString(true));
    if (!isValidArtworkContentType(art_mimetype)) {
        art_mimetype = getContentTypeFromByteVector(data);
    }
    addArtworkResource(item, art_mimetype);
}

void TagLibHandler::extractAPE(zmm::Ref<CdsItem> item) {
    TagLib::APE::File f(item->getLocation().c_str());

    if (!f.isValid()) {
        log_debug("TagLibHandler: could not open APE file: %s\n",
                  item->getLocation().c_str());
        return;
    }
    populateGenericTags(item, f);
}

void TagLibHandler::extractWavPack(zmm::Ref<CdsItem> item) {
    TagLib::WavPack::File f(item->getLocation().c_str());

    if (!f.isValid()) {
        log_debug("TagLibHandler: could not open WavPack file: %s\n",
                  item->getLocation().c_str());
        return;
    }
    populateGenericTags(item, f);
}

void TagLibHandler::extractMP4(zmm::Ref<CdsItem> item) {
    TagLib::MP4::File f(item->getLocation().c_str());
    populateGenericTags(item, f);

    if (!f.isValid()) {
        log_debug("TagLibHandler: could not open mp4 file: %s\n",
                  item->getLocation().c_str());
        return;
    }

    if (!f.hasMP4Tag()) {
        log_debug("TagLibHandler: mp4 file has no tag information\n");
        return;
    }

    String art_mimetype;

    TagLib::MP4::ItemListMap itemsListMap = f.tag()->itemListMap();
    TagLib::MP4::Item coverItem = itemsListMap["covr"];
    TagLib::MP4::CoverArtList coverArtList = coverItem.toCoverArtList();
    if (coverArtList.isEmpty()) {
        log_debug("TagLibHandler: mp4 file has no coverart");
        return;
    }

    TagLib::MP4::CoverArt coverArt = coverArtList.front();
    TagLib::ByteVector data = coverArt.data();
    art_mimetype = getContentTypeFromByteVector(data);

    if (string_ok(art_mimetype))
        addArtworkResource(item, art_mimetype);
}

void TagLibHandler::extractAiff(zmm::Ref<CdsItem> item) {
    TagLib::RIFF::AIFF::File f(item->getLocation().c_str());

    if (!f.isValid()) {
        log_debug("TagLibHandler: could not open AIFF file: %s\n",
                  item->getLocation().c_str());
        return;
    }
    populateGenericTags(item, f);
}

#endif // HAVE_TAGLIB
