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

#ifdef HAVE_TAGLIB
#include "taglib_handler.h" // API

#include <taglib/aifffile.h>
#include <taglib/apefile.h>
#include <taglib/asffile.h>
#include <taglib/attachedpictureframe.h>
#include <taglib/flacfile.h>
#include <taglib/id3v2tag.h>
#include <taglib/mp4file.h>
#include <taglib/mpegfile.h>
#include <taglib/textidentificationframe.h>
#include <taglib/tfilestream.h>
#include <taglib/tiostream.h>
#include <taglib/tpropertymap.h>
#include <taglib/vorbisfile.h>
#include <taglib/wavpackfile.h>

#include "cds_objects.h"
#include "config/config_manager.h"
#include "iohandler/mem_io_handler.h"
#include "util/mime.h"
#include "util/tools.h"

TagLibHandler::TagLibHandler(const std::shared_ptr<Context>& context)
    : MetadataHandler(context)
{
    entrySeparator = this->config->getOption(CFG_IMPORT_LIBOPTS_ENTRY_SEP);
    legacyEntrySeparator = this->config->getOption(CFG_IMPORT_LIBOPTS_ENTRY_LEGACY_SEP);
}

void TagLibHandler::addField(metadata_fields_t field, const TagLib::File& file, const TagLib::Tag* tag, const std::shared_ptr<CdsItem>& item) const
{
    if (tag == nullptr)
        return;

    if (tag->isEmpty())
        return;

    auto sc = StringConverter::i2i(config); // sure is sure

    TagLib::String val;
    TagLib::StringList list;
    std::string value;
    unsigned int i;
    bool checkLegacy = true;

    switch (field) {
    case M_TITLE:
        val = tag->title();
        checkLegacy = false;
        break;
    case M_ARTIST:
        val = tag->artist();
        break;
    case M_ALBUM:
        val = tag->album();
        checkLegacy = false;
        break;
    case M_DATE:
    case M_UPNP_DATE:
        i = tag->year();
        if (i == 0)
            return;
        value = fmt::format("{}-01-01", i);
        break;
    case M_GENRE:
        val = tag->genre();
        checkLegacy = false;
        break;
    case M_DESCRIPTION:
        val = tag->comment();
        checkLegacy = false;
        break;
    case M_TRACKNUMBER:
        i = tag->track();
        if (i == 0)
            return;
        value = fmt::to_string(i);
        item->setTrackNumber(int(i));
        break;
    case M_PARTNUMBER:
        list = file.properties()["DISCNUMBER"];
        if (!list.isEmpty()) {
            value = list[0].toCString(true);
            item->setPartNumber(stoiString(value));
        } else {
            list = file.properties()["TPOS"];
            if (list.isEmpty())
                return;
            value = list[0].toCString(true);
            item->setPartNumber(stoiString(value));
        }
        break;
    case M_ALBUMARTIST:
        // we have to use file.properties() instead of tag->properties()
        // because the latter returns incomplete properties
        // https://mail.kde.org/pipermail/taglib-devel/2015-May/002729.html
        list = file.properties()["ALBUMARTIST"];
        if (list.isEmpty())
            return;
        val = list.toString(entrySeparator);
        break;
    case M_COMPOSER:
        list = file.properties()["COMPOSER"];
        if (list.isEmpty())
            return;
        val = list.toString(entrySeparator);
        break;
    case M_CONDUCTOR:
        list = file.properties()["CONDUCTOR"];
        if (list.isEmpty())
            return;
        val = list.toString(entrySeparator);
        break;
    case M_ORCHESTRA:
        list = file.properties()["ORCHESTRA"];
        if (list.isEmpty())
            return;
        val = list.toString(entrySeparator);
        break;
    default:
        return;
    }

    if ((field != M_DATE) && (field != M_CREATION_DATE) && (field != M_TRACKNUMBER) && (field != M_PARTNUMBER)) {
        if (!legacyEntrySeparator.empty() && checkLegacy)
            val = val.split(legacyEntrySeparator).toString(entrySeparator);
        value = val.toCString(true);
    }

    value = trimString(value);

    if (!value.empty()) {
        item->setMetadata(field, sc->convert(value));
        //        log_debug("Setting metadata on item: {}, {}", field, sc->convert(value).c_str());
    }
}

void TagLibHandler::populateGenericTags(const std::shared_ptr<CdsItem>& item, const TagLib::File& file) const
{
    if (!file.tag())
        return;

    const TagLib::Tag* tag = file.tag();
    for (auto&& key : mt_keys)
        addField(key.first, file, tag, item);

    const TagLib::AudioProperties* audioProps = file.audioProperties();
    if (!audioProps)
        return;

    auto res = item->getResource(0);

    // UPnP bitrate is in bytes/second
    int temp = audioProps->bitrate() * 1024 / 8; // kbit/second -> byte/second
    if (temp > 0) {
        res->addAttribute(R_BITRATE, fmt::to_string(temp));
    }

    temp = audioProps->lengthInMilliseconds();
    if (temp > 0) {
        res->addAttribute(R_DURATION, millisecondsToHMSF(temp));
    }

    temp = audioProps->sampleRate();
    if (temp > 0) {
        res->addAttribute(R_SAMPLEFREQUENCY, fmt::to_string(temp));
    }

    temp = audioProps->channels();
    if (temp > 0) {
        res->addAttribute(R_NRAUDIOCHANNELS, fmt::to_string(temp));
    }
}

void TagLibHandler::populateAuxTags(const std::shared_ptr<CdsItem>& item, const TagLib::PropertyMap& propertyMap, const std::unique_ptr<StringConverter>& sc) const
{
    std::vector<std::string> aux_tags_list = config->getArrayOption(CFG_IMPORT_LIBOPTS_ID3_AUXDATA_TAGS_LIST);
    for (auto&& desiredTag : aux_tags_list) {

        if (desiredTag.empty()) {
            continue;
        }

        if (propertyMap.contains(desiredTag.c_str())) {
            const auto property = propertyMap[desiredTag.c_str()];
            if (property.isEmpty())
                continue;

            auto val = property.toString(entrySeparator);
            if (!legacyEntrySeparator.empty())
                val = val.split(legacyEntrySeparator).toString(entrySeparator);
            std::string value(val.toCString(true));
            value = sc->convert(value);
            log_debug("Adding auxdata: {} with value {}", desiredTag.c_str(), value.c_str());
            item->setAuxData(desiredTag, value);
        }
    }
}

/// \brief read metadata from file and add to object
/// \param obj Object to handle
void TagLibHandler::fillMetadata(std::shared_ptr<CdsObject> obj)
{
    auto item = std::dynamic_pointer_cast<CdsItem>(obj);
    if (item == nullptr)
        return;

    auto mappings = config->getDictionaryOption(CFG_IMPORT_MAPPINGS_MIMETYPE_TO_CONTENTTYPE_LIST);
    std::string content_type = getValueOrDefault(mappings, item->getMimeType());

    TagLib::FileStream fs(item->getLocation().c_str(), true); // true = Read only

    if (content_type == CONTENT_TYPE_MP3) {
        extractMP3(&fs, item);
    } else if (content_type == CONTENT_TYPE_FLAC) {
        extractFLAC(&fs, item);
    } else if (content_type == CONTENT_TYPE_MP4) {
        extractMP4(&fs, item);
    } else if (content_type == CONTENT_TYPE_OGG) {
        extractOgg(&fs, item);
    } else if (content_type == CONTENT_TYPE_APE) {
        extractAPE(&fs, item);
    } else if (content_type == CONTENT_TYPE_WMA) {
        extractASF(&fs, item);
    } else if (content_type == CONTENT_TYPE_WAVPACK) {
        extractWavPack(&fs, item);
    } else if (content_type == CONTENT_TYPE_AIFF) {
        extractAiff(&fs, item);
    } else {
        log_warning("TagLibHandler {}: Does not handle the {} content type", item->getLocation().c_str(), content_type.c_str());
    }
    log_debug("TagLib handler done.");
}

bool TagLibHandler::isValidArtworkContentType(const std::string& art_mimetype)
{
    // saw that simply "PNG" was used with some mp3's, so mimetype setting
    // was probably invalid
    return art_mimetype.find('/') != std::string::npos;
}

std::string TagLibHandler::getContentTypeFromByteVector(const TagLib::ByteVector& data) const
{
#ifdef HAVE_MAGIC
    auto art_mimetype = mime->bufferToMimeType(data.data(), data.size());
    return art_mimetype.empty() ? MIMETYPE_DEFAULT : art_mimetype;
#else
    return MIMETYPE_DEFAULT;
#endif
}

void TagLibHandler::addArtworkResource(const std::shared_ptr<CdsItem>& item, const std::string& art_mimetype)
{
    // if we could not determine the mimetype, then there is no
    // point to add the resource - it's probably garbage
    log_debug("Found artwork of type {} in file {}", art_mimetype.c_str(), item->getLocation().c_str());

    if (art_mimetype != MIMETYPE_DEFAULT) {
        auto resource = std::make_shared<CdsResource>(CH_ID3);
        resource->addAttribute(R_PROTOCOLINFO, renderProtocolInfo(art_mimetype));
        resource->addParameter(RESOURCE_CONTENT_TYPE, ID3_ALBUM_ART);
        item->addResource(resource);
    }
}

/// \brief stream content of object or resource to client
/// \param obj Object to stream
/// \param resNum number of resource
/// \return iohandler to stream to client
std::unique_ptr<IOHandler> TagLibHandler::serveContent(std::shared_ptr<CdsObject> obj, int resNum)
{
    auto item = std::dynamic_pointer_cast<CdsItem>(obj);
    if (item == nullptr) // not streamable
        return nullptr;

    auto mappings = config->getDictionaryOption(CFG_IMPORT_MAPPINGS_MIMETYPE_TO_CONTENTTYPE_LIST);
    std::string content_type = getValueOrDefault(mappings, item->getMimeType());

    TagLib::FileStream roStream(item->getLocation().c_str(), true); // Open read only

    if (content_type == CONTENT_TYPE_MP3) {
        // stream album art from MP3 file
        TagLib::MPEG::File f(&roStream, TagLib::ID3v2::FrameFactory::instance());

        if (!f.isValid())
            throw_std_runtime_error("Could not open file: {}", item->getLocation().c_str());

        if (!f.ID3v2Tag())
            throw_std_runtime_error("resource has no album information");

        TagLib::ID3v2::FrameList list = f.ID3v2Tag()->frameList("APIC");
        if (list.isEmpty())
            throw_std_runtime_error("resource has no album information");

        auto art = dynamic_cast<TagLib::ID3v2::AttachedPictureFrame*>(list.front());

        return std::make_unique<MemIOHandler>(art->picture().data(), art->picture().size());
    }
    if (content_type == CONTENT_TYPE_FLAC) {
        // stream album art from FLAC file
        TagLib::FLAC::File f(&roStream, TagLib::ID3v2::FrameFactory::instance());

        if (!f.isValid())
            throw_std_runtime_error("Could not open flac file: {}", item->getLocation().c_str());

        if (f.pictureList().isEmpty())
            throw_std_runtime_error("flac resource has no picture information");

        TagLib::FLAC::Picture* pic = f.pictureList().front();
        const TagLib::ByteVector& data = pic->data();

        return std::make_unique<MemIOHandler>(data.data(), data.size());
    }
    if (content_type == CONTENT_TYPE_MP4) {
        // stream album art from MP4 file
        TagLib::MP4::File f(&roStream);

        if (!f.isValid()) {
            throw_std_runtime_error("Could not open mp4 file: {}", item->getLocation().c_str());
        }

        if (!f.hasMP4Tag()) {
            throw_std_runtime_error("mp4 resource has no tag information");
        }

        auto tag = f.tag();

        if (!tag->contains("covr")) {
            throw_std_runtime_error("mp4 file has no 'covr' item");
        }

        auto coverItem = tag->item("covr");
        TagLib::MP4::CoverArtList coverArtList = coverItem.toCoverArtList();
        if (coverArtList.isEmpty()) {
            throw_std_runtime_error("mp4 resource has no picture information");
        }

        const TagLib::MP4::CoverArt& coverArt = coverArtList.front();
        const TagLib::ByteVector& data = coverArt.data();

        return std::make_unique<MemIOHandler>(data.data(), data.size());
    }
    if (content_type == CONTENT_TYPE_WMA) {
        // stream album art from WMA file
        TagLib::ASF::File f(&roStream);

        if (!f.isValid())
            throw_std_runtime_error("Could not open flac file: {}", item->getLocation().c_str());

        const TagLib::ASF::AttributeListMap& attrListMap = f.tag()->attributeListMap();
        if (!attrListMap.contains("WM/Picture"))
            throw_std_runtime_error("wma file has no picture information");

        const TagLib::ASF::AttributeList& attrList = attrListMap["WM/Picture"];
        if (attrList.isEmpty())
            throw_std_runtime_error("wma list has no picture information");

        const TagLib::ASF::Picture& wmpic = attrList[0].toPicture();
        if (!wmpic.isValid())
            throw_std_runtime_error("wma pic not valid");

        const TagLib::ByteVector& data = wmpic.picture();

        return std::make_unique<MemIOHandler>(data.data(), data.size());
    }
    if (content_type == CONTENT_TYPE_OGG) {
        // stream album art from Ogg/Vorbis file
        TagLib::Ogg::Vorbis::File f(&roStream);

        if (!f.isValid() || !f.tag())
            throw_std_runtime_error("Could not open vorbis file: {}", item->getLocation().c_str());

        const TagLib::List<TagLib::FLAC::Picture*> picList = f.tag()->pictureList();
        if (picList.isEmpty())
            throw_std_runtime_error("vorbis file has no picture information");

        const TagLib::FLAC::Picture* pic = picList.front();
        const TagLib::ByteVector& data = pic->data();

        return std::make_unique<MemIOHandler>(data.data(), data.size());
    }

    throw_std_runtime_error("Unsupported content_type: {}", content_type.c_str());
}

void TagLibHandler::extractMP3(TagLib::IOStream* roStream, const std::shared_ptr<CdsItem>& item) const
{
    TagLib::MPEG::File mp3(roStream, TagLib::ID3v2::FrameFactory::instance());

    if (!mp3.isValid()) {
        log_info("TagLibHandler {}: does not appear to be a valid mp3 file", item->getLocation().c_str());
        return;
    }
    populateGenericTags(item, mp3);

    if (!mp3.hasID3v2Tag()) {
        log_debug("{}: has no IDv2 tags", item->getLocation().c_str());
        return;
    }

    auto sc = StringConverter::i2i(config);

    auto frameListMap = mp3.ID3v2Tag()->frameListMap();
    // http://id3.org/id3v2.4.0-frames "4.2.6. User defined text information frame"
    bool hasTXXXFrames = frameListMap.contains("TXXX");

    std::vector<std::string> aux_tags_list = config->getArrayOption(CFG_IMPORT_LIBOPTS_ID3_AUXDATA_TAGS_LIST);
    for (auto&& desiredFrame : aux_tags_list) {
        if (desiredFrame.empty()) {
            continue;
        }

        if (frameListMap.contains(desiredFrame.c_str())) {
            const auto frameList = frameListMap[desiredFrame.c_str()];
            if (frameList.isEmpty())
                continue;

            std::string value;
            for (auto&& frame : frameList) {
                const auto textFrame = dynamic_cast<const TagLib::ID3v2::TextIdentificationFrame*>(frame);
                if (textFrame == nullptr)
                    continue;
                TagLib::String frameContents = textFrame->fieldList().toString(entrySeparator);
                if (!value.empty())
                    value += entrySeparator;
                if (!legacyEntrySeparator.empty())
                    frameContents = frameContents.split(legacyEntrySeparator).toString(entrySeparator);
                value += sc->convert(frameContents.toCString(true));
            }
            log_debug("Adding auxdata: {} with value {}", desiredFrame.c_str(), value.c_str());
            item->setAuxData(desiredFrame, value);
        } else if (hasTXXXFrames && startswith(desiredFrame, "TXXX:")) {
            const auto frameList = frameListMap["TXXX"];
            //log_debug("TXXX Frame list has {} elements", frameList.size());

            std::string desiredSubTag = desiredFrame.substr(5);
            if (desiredSubTag.empty())
                continue;

            for (auto&& frame : frameList) {
                const auto textFrame = dynamic_cast<const TagLib::ID3v2::TextIdentificationFrame*>(frame);
                if (textFrame == nullptr)
                    continue;
                std::string content;
                std::string subTag;
                for (auto&& field : textFrame->fieldList()) {
                    if (subTag.empty()) {
                        // first element is subTag name
                        subTag = field.toCString(true);
                    } else {
                        content = fmt::format("{}{}{}", content, field.toCString(true), entrySeparator);
                    }
                }

                // remove entrySeparator at the end again
                if (content.length() > entrySeparator.length())
                    content = content.substr(0, content.length() - entrySeparator.length());
                // log_debug("TXXX Tag: {}", subTag.c_str());

                if (desiredSubTag == subTag) {
                    if (content.substr(0, subTag.length()) == subTag)
                        content = content.substr(subTag.length() + 1); // Avoid leading tag for options unknown to taglib

                    log_debug("Adding auxdata: '{}' with value '{}'", desiredFrame, content);
                    item->setAuxData(desiredFrame, content);
                    break;
                }
            }
        }
    }

    const TagLib::ID3v2::FrameList apicFrameList = mp3.ID3v2Tag()->frameList("APIC");
    if (!apicFrameList.isEmpty()) {
        auto art = dynamic_cast<const TagLib::ID3v2::AttachedPictureFrame*>(apicFrameList.front());

        const TagLib::ByteVector pic = art->picture();
        std::string art_mimetype = sc->convert(art->mimeType().toCString(true));
        if (!isValidArtworkContentType(art_mimetype)) {
            art_mimetype = getContentTypeFromByteVector(pic);
        }

        addArtworkResource(item, art_mimetype);
    }
}

void TagLibHandler::extractOgg(TagLib::IOStream* roStream, const std::shared_ptr<CdsItem>& item) const
{
    TagLib::Ogg::Vorbis::File vorbis(roStream);

    if (!vorbis.isValid()) {
        log_info("TagLibHandler {}: does not appear to be a valid ogg file", item->getLocation().c_str());
        return;
    }
    populateGenericTags(item, vorbis);

    if (!vorbis.tag())
        return;

    auto sc = StringConverter::i2i(config);
    auto propertyMap = vorbis.properties();
    populateAuxTags(item, propertyMap, sc);

    // Vorbis uses the FLAC binary picture structure...
    // https://wiki.xiph.org/VorbisComment#Cover_art
    // The unofficial COVERART field is not supported.
    const TagLib::List<TagLib::FLAC::Picture*> picList = vorbis.tag()->pictureList();
    if (picList.isEmpty())
        return;

    const TagLib::FLAC::Picture* pic = picList.front();
    const TagLib::ByteVector& data = pic->data();

    std::string art_mimetype = sc->convert(pic->mimeType().toCString(true));
    if (!isValidArtworkContentType(art_mimetype)) {
        art_mimetype = getContentTypeFromByteVector(data);
    }
    addArtworkResource(item, art_mimetype);
}

void TagLibHandler::extractASF(TagLib::IOStream* roStream, const std::shared_ptr<CdsItem>& item) const
{
    TagLib::ASF::File asf(roStream);

    if (!asf.isValid()) {
        log_info("TagLibHandler {}: does not appear to be a valid asf/wma file", item->getLocation().c_str());
        return;
    }
    populateGenericTags(item, asf);

    auto sc = StringConverter::i2i(config);
    auto propertyMap = asf.properties();
    populateAuxTags(item, propertyMap, sc);

    auto audioProps = asf.audioProperties();
    auto temp = audioProps->bitsPerSample();
    auto res = item->getResource(0);
    if (temp > 0) {
        res->addAttribute(R_BITS_PER_SAMPLE, fmt::to_string(temp));
    }

    const TagLib::ASF::AttributeListMap& attrListMap = asf.tag()->attributeListMap();
    if (attrListMap.contains("WM/Picture")) {
        const TagLib::ASF::AttributeList& attrList = attrListMap["WM/Picture"];
        if (attrList.isEmpty())
            return;

        const TagLib::ASF::Picture& wmpic = attrList[0].toPicture();
        if (!wmpic.isValid())
            return;

        std::string art_mimetype = sc->convert(wmpic.mimeType().toCString(true));
        if (!isValidArtworkContentType(art_mimetype)) {
            art_mimetype = getContentTypeFromByteVector(wmpic.picture());
        }
        addArtworkResource(item, art_mimetype);
    }
}

void TagLibHandler::extractFLAC(TagLib::IOStream* roStream, const std::shared_ptr<CdsItem>& item) const
{
    TagLib::FLAC::File flac(roStream, TagLib::ID3v2::FrameFactory::instance());

    if (!flac.isValid()) {
        log_info("TagLibHandler {}: does not appear to be a valid flac file", item->getLocation().c_str());
        return;
    }
    populateGenericTags(item, flac);

    auto sc = StringConverter::i2i(config);
    auto propertyMap = flac.properties();
    populateAuxTags(item, propertyMap, sc);

    if (flac.pictureList().isEmpty()) {
        log_debug("TagLibHandler: flac resource has no picture information");
        return;
    }
    const TagLib::FLAC::Picture* pic = flac.pictureList().front();
    const TagLib::ByteVector& data = pic->data();

    auto audioProps = flac.audioProperties();
    auto temp = audioProps->bitsPerSample();
    auto res = item->getResource(0);
    if (temp > 0) {
        res->addAttribute(R_BITS_PER_SAMPLE, fmt::to_string(temp));
    }

    std::string art_mimetype = sc->convert(pic->mimeType().toCString(true));
    if (!isValidArtworkContentType(art_mimetype)) {
        art_mimetype = getContentTypeFromByteVector(data);
    }
    addArtworkResource(item, art_mimetype);
}

void TagLibHandler::extractAPE(TagLib::IOStream* roStream, const std::shared_ptr<CdsItem>& item) const
{
    TagLib::APE::File ape(roStream);

    if (!ape.isValid()) {
        log_info("TagLibHandler {}: does not appear to be a valid APE file", item->getLocation().c_str());
        return;
    }
    populateGenericTags(item, ape);

    auto sc = StringConverter::i2i(config);
    auto propertyMap = ape.properties();
    populateAuxTags(item, propertyMap, sc);

    auto audioProps = ape.audioProperties();
    auto temp = audioProps->bitsPerSample();
    auto res = item->getResource(0);
    if (temp > 0) {
        res->addAttribute(R_BITS_PER_SAMPLE, fmt::to_string(temp));
    }
}

void TagLibHandler::extractWavPack(TagLib::IOStream* roStream, const std::shared_ptr<CdsItem>& item) const
{
    TagLib::WavPack::File wavpack(roStream);

    if (!wavpack.isValid()) {
        log_info("TagLibHandler {}: does not appear to be a valid WavPack file", item->getLocation().c_str());
        return;
    }
    populateGenericTags(item, wavpack);

    auto sc = StringConverter::i2i(config);
    auto propertyMap = wavpack.properties();
    populateAuxTags(item, propertyMap, sc);

    auto audioProps = wavpack.audioProperties();
    auto temp = audioProps->bitsPerSample();
    auto res = item->getResource(0);
    if (temp > 0) {
        res->addAttribute(R_BITS_PER_SAMPLE, fmt::to_string(temp));
    }
}

void TagLibHandler::extractMP4(TagLib::IOStream* roStream, const std::shared_ptr<CdsItem>& item) const
{
    TagLib::MP4::File mp4(roStream);

    if (!mp4.isValid()) {
        log_info("TagLibHandler {}: does not appear to be a valid mp4 file", item->getLocation().c_str());
        return;
    }

    populateGenericTags(item, mp4);

    if (!mp4.hasMP4Tag()) {
        log_info("TagLibHandler {}: mp4 file has no tag information", item->getLocation().c_str());
        return;
    }

    auto sc = StringConverter::i2i(config);
    auto propertyMap = mp4.tag()->properties();
    populateAuxTags(item, propertyMap, sc);

    auto audioProps = mp4.audioProperties();
    auto temp = audioProps->bitsPerSample();
    auto res = item->getResource(0);
    if (temp > 0) {
        res->addAttribute(R_BITS_PER_SAMPLE, fmt::to_string(temp));
    }

    if (mp4.tag()->contains("covr")) {
        auto coverItem = mp4.tag()->item("covr");
        TagLib::MP4::CoverArtList coverArtList = coverItem.toCoverArtList();
        if (coverArtList.isEmpty()) {
            log_info("TagLibHandler {}: mp4 file has no coverart",
                item->getLocation().c_str());
            return;
        }

        auto& coverArt = coverArtList.front();
        auto art_mimetype = getContentTypeFromByteVector(coverArt.data());
        if (!art_mimetype.empty()) {
            addArtworkResource(item, art_mimetype);
        }
    } else {
        log_debug("TagLibHandler {}: mp4 file has no 'covr' item",
            item->getLocation().c_str());
    }
}

void TagLibHandler::extractAiff(TagLib::IOStream* roStream, const std::shared_ptr<CdsItem>& item) const
{
    TagLib::RIFF::AIFF::File aiff(roStream);

    if (!aiff.isValid()) {
        log_info("TagLibHandler {}: does not appear to be a valid AIFF file", item->getLocation().c_str());
        return;
    }
    populateGenericTags(item, aiff);

    auto sc = StringConverter::i2i(config);
    auto propertyMap = aiff.properties();
    populateAuxTags(item, propertyMap, sc);

    auto audioProps = aiff.audioProperties();
    auto temp = audioProps->bitsPerSample();
    auto res = item->getResource(0);
    if (temp > 0) {
        res->addAttribute(R_BITS_PER_SAMPLE, fmt::to_string(temp));
    }
}

#endif // HAVE_TAGLIB
