/*MT*

    MediaTomb - http://www.mediatomb.cc/

    taglib_handler.cc - this file is part of MediaTomb.

    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>

    Copyright (C) 2006-2010 Gena Batyan <bgeradz@mediatomb.cc>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>,
                            Leonhard Wimmer <leo@mediatomb.cc>

    Copyright (C) 2016-2025 Gerbera Contributors

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
#define GRB_LOG_FAC GrbLogFacility::taglib

#include "taglib_handler.h" // API

#include "cds/cds_item.h"
#include "config/config.h"
#include "config/config_val.h"
#include "exceptions.h"
#include "iohandler/mem_io_handler.h"
#include "metadata_enums.h"
#include "util/grb_time.h"
#include "util/mime.h"
#include "util/string_converter.h"
#include "util/tools.h"

#include <aifffile.h>
#include <apefile.h>
#include <asffile.h>
#include <attachedpictureframe.h>
#include <flacfile.h>
#include <id3v2tag.h>
#include <mp4file.h>
#include <mpegfile.h>
#include <oggflacfile.h>
#include <opusfile.h>
#include <speexfile.h>
#include <tdebuglistener.h>
#include <textidentificationframe.h>
#include <tfilestream.h>
#include <tiostream.h>
#include <tpropertymap.h>
#include <vorbisfile.h>
#include <wavpackfile.h>

#if TAGLIB_MAJOR_VERSION >= 2
#include <mp4itemfactory.h>
#endif

class GerberaTagLibDebugListener : public TagLib::DebugListener {
private:
    GerberaTagLibDebugListener()
    {
        // Install listener.
        TagLib::setDebugListener(this);
    }

    virtual void printMessage(const TagLib::String& msg) override
    {
        // Remove trailing newlines.
        log_debug("{}", msg.stripWhiteSpace().to8Bit(true));
    }
    static GerberaTagLibDebugListener grbListener;
};

GerberaTagLibDebugListener GerberaTagLibDebugListener::grbListener;

TagLibHandler::TagLibHandler(const std::shared_ptr<Context>& context)
    : MediaMetadataHandler(context,
          ConfigVal::IMPORT_LIBOPTS_ID3_ENABLED,
          ConfigVal::IMPORT_LIBOPTS_ID3_METADATA_TAGS_LIST,
          ConfigVal::IMPORT_LIBOPTS_ID3_AUXDATA_TAGS_LIST,
          ConfigVal::IMPORT_LIBOPTS_ID3_COMMENT_ENABLED,
          ConfigVal::IMPORT_LIBOPTS_ID3_COMMENT_LIST)
{
    entrySeparator = this->config->getOption(ConfigVal::IMPORT_LIBOPTS_ENTRY_SEP);
    legacyEntrySeparator = this->config->getOption(ConfigVal::IMPORT_LIBOPTS_ENTRY_LEGACY_SEP);
    trimStringInPlace(legacyEntrySeparator);
    if (legacyEntrySeparator.empty())
        legacyEntrySeparator = "\n";
}

void TagLibHandler::addField(
    MetadataFields field,
    const TagLib::File& file,
    const TagLib::Tag* tag,
    const std::shared_ptr<CdsItem>& item) const
{
    if (!tag || tag->isEmpty())
        return;

    auto sc = converterManager->i2i(); // sure is sure

    std::vector<std::string> value;

    auto propertyMap = std::map<MetadataFields, std::string> {
        { MetadataFields::M_ARTIST, "ARTIST" },
        { MetadataFields::M_ALBUMARTIST, "ALBUMARTIST" },
        { MetadataFields::M_COMPOSER, "COMPOSER" },
        { MetadataFields::M_CONDUCTOR, "CONDUCTOR" },
        { MetadataFields::M_GENRE, "GENRE" },
        { MetadataFields::M_ORCHESTRA, "ORCHESTRA" },
    };

    switch (field) {
    case MetadataFields::M_TITLE:
        value.push_back(tag->title().to8Bit(true));
        break;
    case MetadataFields::M_ALBUM:
        value.push_back(tag->album().to8Bit(true));
        break;
    case MetadataFields::M_DATE:
    case MetadataFields::M_UPNP_DATE: {
        unsigned int i = tag->year();
        if (i == 0)
            return;
        value.push_back(fmt::format("{}-01-01", i));
        break;
    }
    case MetadataFields::M_DESCRIPTION:
        value.push_back(tag->comment().to8Bit(true));
        break;
    case MetadataFields::M_TRACKNUMBER: {
        std::uint32_t i = tag->track();
        if (i == 0 || i > static_cast<std::uint32_t>(std::numeric_limits<int>::max()))
            return;
        value.push_back(fmt::to_string(i));
        item->setTrackNumber(i);
        break;
    }
    case MetadataFields::M_PARTNUMBER: {
        auto list = file.properties()["DISCNUMBER"];
        if (!list.isEmpty()) {
            value.push_back(list[0].to8Bit(true));
        } else {
            auto list2 = file.properties()["TPOS"];
            if (list2.isEmpty())
                return;
            value.push_back(list2[0].to8Bit(true));
        }
        item->setPartNumber(stoiString(value[0]));
        break;
    }
    default: {
        auto it = propertyMap.find(field);
        if (it == propertyMap.end())
            return;
        // we have to use file.properties() instead of tag->properties()
        // because the latter returns incomplete properties
        // https://mail.kde.org/pipermail/taglib-devel/2015-May/002729.html
        auto list = file.properties()[it->second];
        if (list.isEmpty())
            return;
        for (auto&& entry : list) {
            for (auto&& val : entry.split(legacyEntrySeparator))
                value.push_back(val.to8Bit(true));
        }
    }
    }

    if (!value.empty()) {
        for (auto&& entry : value) {
            trimStringInPlace(entry);
            if (!entry.empty()) {
                auto [val, err] = sc->convert(entry);
                if (!err.empty()) {
                    log_warning("{}: {}", item->getLocation().string(), err);
                }
                item->addMetaData(field, val);
                log_debug("Setting metadata on item: {} = {}", field, val);
            }
        }
    }
}

void TagLibHandler::addSpecialFields(
    const TagLib::File& file,
    const TagLib::Tag* tag,
    const std::shared_ptr<CdsItem>& item) const
{
    if (!tag || tag->isEmpty())
        return;

    auto sc = converterManager->i2i(); // sure is sure

    for (auto&& [key, meta] : metaTags) {
        auto list = file.properties()[key];
        if (list.isEmpty())
            return;
        for (auto&& val : list) {
            for (auto&& entrySeg : val.split(legacyEntrySeparator)) {
                std::string entry = entrySeg.to8Bit(true);
                trimStringInPlace(entry);
                if (!entry.empty()) {
                    auto [val, err] = sc->convert(entry);
                    if (!err.empty()) {
                        log_warning("{}: {}", item->getLocation().string(), err);
                    }
                    item->addMetaData(meta, val);
                }
            }
        }
    }
}

void TagLibHandler::populateGenericTags(
    const std::shared_ptr<CdsItem>& item,
    const TagLib::File& file,
    const TagLib::PropertyMap& propertyMap,
    const std::shared_ptr<StringConverter>& sc) const
{
    if (!file.tag())
        return;

    const TagLib::Tag* tag = file.tag();
    for (auto&& [field, key] : MetaEnumMapper::mt_keys)
        addField(field, file, tag, item);

    addSpecialFields(file, tag, item);

    const TagLib::AudioProperties* audioProps = file.audioProperties();
    if (!audioProps)
        return;

    auto res = item->getResource(ContentHandler::DEFAULT);

    // UPnP bitrate is in bytes/second
    int temp = audioProps->bitrate() * 1024 / 8; // kbit/second -> byte/second
    if (temp > 0) {
        res->addAttribute(ResourceAttribute::BITRATE, fmt::to_string(temp));
    }

    temp = audioProps->lengthInMilliseconds();
    if (temp > 0) {
        res->addAttribute(ResourceAttribute::DURATION, millisecondsToHMSF(temp));
    }

    temp = audioProps->sampleRate();
    if (temp > 0) {
        res->addAttribute(ResourceAttribute::SAMPLEFREQUENCY, fmt::to_string(temp));
    }

    temp = audioProps->channels();
    if (temp > 0) {
        res->addAttribute(ResourceAttribute::NRAUDIOCHANNELS, fmt::to_string(temp));
    }

    populateAuxTags(item, propertyMap, sc);
    if (item->getMetaData(MetadataFields::M_DESCRIPTION).empty() && isCommentEnabled)
        makeComment(item, propertyMap, sc);
}

void TagLibHandler::populateAuxTags(
    const std::shared_ptr<CdsItem>& item,
    const TagLib::PropertyMap& propertyMap,
    const std::shared_ptr<StringConverter>& sc) const
{
    for (auto&& desiredTag : auxTags) {
        if (desiredTag.empty()) {
            continue;
        }

        if (propertyMap.contains(desiredTag.c_str())) {
            auto&& property = propertyMap[desiredTag.c_str()];
            if (property.isEmpty())
                continue;

            auto entry = property.toString(entrySeparator);
            if (!legacyEntrySeparator.empty())
                entry = entry.split(legacyEntrySeparator).toString(entrySeparator);
            std::string value(entry.to8Bit(true));
            auto [val, err] = sc->convert(value);
            if (!err.empty()) {
                log_warning("{}: {}", item->getLocation().string(), err);
            }
            log_debug("Adding auxdata: {} with value {}", desiredTag, val);
            item->setAuxData(desiredTag, val);
        }
    }
}

void TagLibHandler::makeComment(
    const std::shared_ptr<CdsItem>& item,
    const TagLib::PropertyMap& propertyMap,
    const std::shared_ptr<StringConverter>& sc) const
{
    std::vector<std::string> snippets;
    for (auto&& [label, desiredTag] : commentMap) {
        if (desiredTag.empty()) {
            continue;
        }

        if (propertyMap.contains(desiredTag.c_str())) {
            auto&& property = propertyMap[desiredTag.c_str()];
            if (property.isEmpty())
                continue;

            auto entry = property.toString(entrySeparator);
            if (!legacyEntrySeparator.empty())
                entry = entry.split(legacyEntrySeparator).toString(entrySeparator);
            std::string value(entry.to8Bit(true));
            auto [val, err] = sc->convert(value);
            if (!err.empty()) {
                log_warning("{}: {}", item->getLocation().string(), err);
            }
            log_debug("Adding auxdata: {} with value {}", desiredTag, val);
            snippets.push_back(fmt::format("{}: {}", label, val));
        }
    }
    item->addMetaData(MetadataFields::M_DESCRIPTION, fmt::format("{}", fmt::join(snippets, ", ")));
    log_debug("Fabricated Comment: {}", fmt::format("{}", fmt::join(snippets, ", ")));
}

/// \brief read metadata from file and add to object
/// \param obj Object to handle
void TagLibHandler::fillMetadata(const std::shared_ptr<CdsObject>& obj)
{
    auto item = std::dynamic_pointer_cast<CdsItem>(obj);
    if (!item || !isEnabled)
        return;

    std::string contentType = getValueOrDefault(mimeContentTypeMappings, item->getMimeType());

    log_debug("Reading {}, content type {}", item->getLocation().c_str(), contentType);
    auto fs = TagLib::FileStream(item->getLocation().c_str(), true); // true = Read only

    if (contentType == CONTENT_TYPE_MP3) {
        extractMP3(fs, item);
    } else if (contentType == CONTENT_TYPE_FLAC) {
        extractFLAC(fs, item);
    } else if (contentType == CONTENT_TYPE_MP4) {
        extractMP4(fs, item);
    } else if (contentType == CONTENT_TYPE_OGG) {
        extractOgg(fs, item);
    } else if (contentType == CONTENT_TYPE_APE) {
        extractAPE(fs, item);
    } else if (contentType == CONTENT_TYPE_WMA) {
        extractASF(fs, item);
    } else if (contentType == CONTENT_TYPE_WAVPACK) {
        extractWavPack(fs, item);
    } else if (contentType == CONTENT_TYPE_AIFF) {
        extractAiff(fs, item);
    } else {
        log_warning("TagLibHandler: File '{}' has unhandled content type '{}'", item->getLocation().c_str(), contentType.c_str());
    }
    log_debug("Done {}", item->getLocation().c_str());
}

bool TagLibHandler::isValidArtworkContentType(std::string_view artMimetype)
{
    // saw that simply "PNG" was used with some mp3's, so mimetype setting
    // was probably invalid
    return artMimetype.find('/') != std::string_view::npos;
}

std::string TagLibHandler::getContentTypeFromByteVector(const TagLib::ByteVector& data) const
{
#ifdef HAVE_MAGIC
    auto artMimetype = mime->bufferToMimeType(data.data(), data.size());
    return artMimetype.empty() ? MIMETYPE_DEFAULT : artMimetype;
#else
    return MIMETYPE_DEFAULT;
#endif
}

void TagLibHandler::addArtworkResource(const std::shared_ptr<CdsItem>& item, const std::string& artMimetype)
{
    // if we could not determine the mimetype, then there is no
    // point to add the resource - it's probably garbage
    log_debug("Found artwork of type {} in file {}", artMimetype, item->getLocation().c_str());

    if (artMimetype != MIMETYPE_DEFAULT) {
        auto resource = std::make_shared<CdsResource>(ContentHandler::ID3, ResourcePurpose::Thumbnail);
        resource->addAttribute(ResourceAttribute::PROTOCOLINFO, renderProtocolInfo(artMimetype));
        item->addResource(resource);
    }
}

/// \brief stream content of object or resource to client
/// \param obj Object to stream
/// \param resource the resource
/// \return iohandler to stream to client
std::unique_ptr<IOHandler> TagLibHandler::serveContent(
    const std::shared_ptr<CdsObject>& obj,
    const std::shared_ptr<CdsResource>& resource)
{
    auto item = std::dynamic_pointer_cast<CdsItem>(obj);
    if (!item) // not streamable
        return nullptr;

    std::string contentType = getValueOrDefault(mimeContentTypeMappings, item->getMimeType());
    auto itemLocation = item->getLocation().c_str();

    auto roStream = TagLib::FileStream(itemLocation, true); // Open read only

    if (contentType == CONTENT_TYPE_MP3) {
        // stream album art from MP3 file
#if TAGLIB_MAJOR_VERSION >= 2
        auto f = TagLib::MPEG::File(&roStream, true, TagLib::MPEG::Properties::Average, TagLib::ID3v2::FrameFactory::instance());
#else
        auto f = TagLib::MPEG::File(&roStream, TagLib::ID3v2::FrameFactory::instance());
#endif

        if (!f.isValid())
            throw_std_runtime_error("Could not open file {} as MP3", itemLocation);

        if (!f.ID3v2Tag())
            throw_std_runtime_error("MP3 resource {} has no ID3 information", itemLocation);

        auto&& list = f.ID3v2Tag()->frameList("APIC");
        if (list.isEmpty())
            throw_std_runtime_error("MP3 resource {} has no album information", itemLocation);

        auto art = dynamic_cast<TagLib::ID3v2::AttachedPictureFrame*>(list.front());
        if (!art) {
            return nullptr;
        }

        return std::make_unique<MemIOHandler>(art->picture().data(), art->picture().size());
    }
    if (contentType == CONTENT_TYPE_FLAC) {
        // stream album art from FLAC file
#if TAGLIB_MAJOR_VERSION >= 2
        auto f = TagLib::FLAC::File(&roStream, true, TagLib::MPEG::Properties::Average, TagLib::ID3v2::FrameFactory::instance());
#else
        auto f = TagLib::FLAC::File(&roStream, TagLib::ID3v2::FrameFactory::instance());
#endif

        if (!f.isValid())
            throw_std_runtime_error("Could not open file {} as flac", itemLocation);

        if (f.pictureList().isEmpty())
            throw_std_runtime_error("Flac resource {} has empty picture information", itemLocation);

        const TagLib::FLAC::Picture* pic = f.pictureList().front();
        const TagLib::ByteVector& data = pic->data();

        return std::make_unique<MemIOHandler>(data.data(), data.size());
    }
    if (contentType == CONTENT_TYPE_MP4) {
        // stream album art from MP4 file
        auto f = TagLib::MP4::File(&roStream);

        if (!f.isValid()) {
            throw_std_runtime_error("Could not open file {} as MP4", itemLocation);
        }

        if (!f.hasMP4Tag()) {
            throw_std_runtime_error("MP4 resource {} has no tag information", itemLocation);
        }

        auto tag = f.tag();

        if (!tag->contains("covr")) {
            throw_std_runtime_error("MP4 resource {} has no 'covr' item", itemLocation);
        }

        auto coverItem = tag->item("covr");
        TagLib::MP4::CoverArtList coverArtList = coverItem.toCoverArtList();
        if (coverArtList.isEmpty()) {
            throw_std_runtime_error("MP4 resource {} has empty 'covr' information", itemLocation);
        }

        const TagLib::MP4::CoverArt& coverArt = coverArtList.front();
        const TagLib::ByteVector& data = coverArt.data();

        return std::make_unique<MemIOHandler>(data.data(), data.size());
    }
    if (contentType == CONTENT_TYPE_WMA) {
        // stream album art from WMA file
        auto f = TagLib::ASF::File(&roStream);

        if (!f.isValid())
            throw_std_runtime_error("Could not open file {} as WMA", itemLocation);

        const TagLib::ASF::AttributeListMap& attrListMap = f.tag()->attributeListMap();
        if (!attrListMap.contains("WM/Picture"))
            throw_std_runtime_error("WMA resource {} has no picture information", itemLocation);

        const TagLib::ASF::AttributeList& attrList = attrListMap["WM/Picture"];
        if (attrList.isEmpty())
            throw_std_runtime_error("WMA resource {} has empty picture information", itemLocation);

        const TagLib::ASF::Picture& wmpic = attrList[0].toPicture();
        if (!wmpic.isValid())
            throw_std_runtime_error("WMA resource {} has invalid picture", itemLocation);

        const TagLib::ByteVector& data = wmpic.picture();

        return std::make_unique<MemIOHandler>(data.data(), data.size());
    }
    if (contentType == CONTENT_TYPE_OGG) {
        // stream album art from Ogg/Vorbis file
        auto f = getOggFile(roStream);

        if (!f || !f->isValid() || !f->tag())
            throw_std_runtime_error("Could not open file {} as OGG", itemLocation);

        const auto picList = dynamic_cast<TagLib::Ogg::XiphComment*>(f->tag())->pictureList();
        if (picList.isEmpty())
            throw_std_runtime_error("OGG resource {} has empty picture information", itemLocation);

        const TagLib::FLAC::Picture* pic = picList.front();
        const TagLib::ByteVector& data = pic->data();

        return std::make_unique<MemIOHandler>(data.data(), data.size());
    }

    throw_std_runtime_error("File {} has unsupported content type {}", itemLocation, contentType);
}

void TagLibHandler::extractMP3(TagLib::IOStream& roStream, const std::shared_ptr<CdsItem>& item) const
{
#if TAGLIB_MAJOR_VERSION >= 2
    auto mp3 = TagLib::MPEG::File(&roStream, true, TagLib::MPEG::Properties::Average, TagLib::ID3v2::FrameFactory::instance());
#else
    auto mp3 = TagLib::MPEG::File(&roStream, TagLib::ID3v2::FrameFactory::instance());
#endif
    if (!mp3.isValid()) {
        log_info("TagLibHandler {}: does not appear to be a valid mp3 file", item->getLocation().c_str());
        return;
    }

    auto sc = converterManager->i2i();
    populateGenericTags(item, mp3, mp3.hasID3v2Tag() ? mp3.ID3v2Tag()->properties() : mp3.properties(), sc);

    if (!mp3.hasID3v2Tag()) {
        log_debug("{}: has no IDv2 tags", item->getLocation().c_str());
        return;
    }

    auto&& frameListMap = mp3.ID3v2Tag()->frameListMap();
    // http://id3.org/id3v2.4.0-frames "4.2.6. User defined text information frame"
    bool hasTXXXFrames = frameListMap.contains("TXXX");

    for (auto&& desiredFrame : auxTags) {
        if (desiredFrame.empty()) {
            continue;
        }

        if (frameListMap.contains(desiredFrame.c_str())) {
            auto&& frameList = frameListMap[desiredFrame.c_str()];
            if (frameList.isEmpty())
                continue;

            std::vector<std::string> content;
            for (auto&& frame : frameList) {
                auto textFrame = dynamic_cast<const TagLib::ID3v2::TextIdentificationFrame*>(frame);
                if (!textFrame)
                    continue;
                for (auto&& field : textFrame->fieldList()) {
                    if (legacyEntrySeparator.empty()) {
                        auto [val, err] = sc->convert(field.to8Bit(true));
                        if (!err.empty()) {
                            log_warning("{}: {}", item->getLocation().string(), err);
                        }
                        content.push_back(val);
                    } else {
                        for (auto&& fval : field.split(legacyEntrySeparator)) {
                            auto [val, err] = sc->convert(fval.to8Bit(true));
                            if (!err.empty()) {
                                log_warning("{}: {}", item->getLocation().string(), err);
                            }
                            content.push_back(val);
                        }
                    }
                }
            }
            if (!content.empty()) {
                log_debug("Adding auxdata: {} with value '{}'", desiredFrame, fmt::to_string(fmt::join(content, entrySeparator)));
                item->setAuxData(desiredFrame, fmt::format("{}", fmt::join(content, entrySeparator)));
            }
        } else if (hasTXXXFrames && startswith(desiredFrame, "TXXX:")) {
            auto&& frameList = frameListMap["TXXX"];
            log_debug("TXXX Frame list has {} elements", frameList.size());

            std::string desiredSubTag = desiredFrame.substr(5);
            if (desiredSubTag.empty())
                continue;

            for (auto&& frame : frameList) {
                const auto textFrame = dynamic_cast<const TagLib::ID3v2::TextIdentificationFrame*>(frame);
                if (!textFrame)
                    continue;
                std::vector<std::string> content;
                std::string subTag;
                for (auto&& field : textFrame->fieldList()) {
                    if (subTag.empty()) {
                        // first element is subTag name
                        auto [val, err] = sc->convert(field.to8Bit(true));
                        if (!err.empty()) {
                            log_warning("{}: {}", item->getLocation().string(), err);
                        }
                        subTag = val;
                    } else if (legacyEntrySeparator.empty()) {
                        auto [val, err] = sc->convert(field.to8Bit(true));
                        if (!err.empty()) {
                            log_warning("{}: {}", item->getLocation().string(), err);
                        }
                        content.push_back(val);
                    } else {
                        for (auto&& fval : field.split(legacyEntrySeparator)) {
                            auto [val, err] = sc->convert(fval.to8Bit(true));
                            if (!err.empty()) {
                                log_warning("{}: {}", item->getLocation().string(), err);
                            }
                            content.push_back(val);
                        }
                    }
                }
                log_debug("TXXX Tag: {}", subTag);

                if (desiredSubTag == subTag && !content.empty()) {
                    if (content[0] == subTag)
                        content.erase(content.begin()); // Avoid leading tag for options unknown to taglib

                    log_debug("Adding auxdata: '{}' with value '{}'", desiredFrame, fmt::to_string(fmt::join(content, entrySeparator)));
                    item->setAuxData(desiredFrame, fmt::format("{}", fmt::join(content, entrySeparator)));
                    break;
                }
            }
        }
    }

    auto&& apicFrameList = mp3.ID3v2Tag()->frameList("APIC");
    if (!apicFrameList.isEmpty()) {
        auto art = dynamic_cast<const TagLib::ID3v2::AttachedPictureFrame*>(apicFrameList.front());
        if (!art) {
            return;
        }

        auto pic = art->picture();
        auto [artMimetype, err] = sc->convert(art->mimeType().to8Bit(true));
        if (!err.empty()) {
            log_warning("{}: {}", item->getLocation().string(), err);
        }
        if (!isValidArtworkContentType(artMimetype)) {
            artMimetype = getContentTypeFromByteVector(pic);
        }

        addArtworkResource(item, artMimetype);
    }
}

std::unique_ptr<TagLib::File> TagLibHandler::getOggFile(TagLib::IOStream& ioStream)
{
    if (TagLib::Ogg::Vorbis::File::isSupported(&ioStream)) {
        return std::make_unique<TagLib::Ogg::Vorbis::File>(&ioStream);
    }

    if (TagLib::Ogg::Opus::File::isSupported(&ioStream)) {
        return std::make_unique<TagLib::Ogg::Opus::File>(&ioStream);
    }

    if (TagLib::Ogg::Speex::File::isSupported(&ioStream)) {
        return std::make_unique<TagLib::Ogg::Speex::File>(&ioStream);
    }

    if (TagLib::Ogg::FLAC::File::isSupported(&ioStream)) {
        return std::make_unique<TagLib::Ogg::FLAC::File>(&ioStream);
    }

    log_warning("Could not match supported Taglib OGG File for {}", ioStream.name());
    return nullptr;
}

void TagLibHandler::extractOgg(TagLib::IOStream& roStream, const std::shared_ptr<CdsItem>& item) const
{
    auto oggFile = getOggFile(roStream);

    if (!oggFile || !oggFile->isValid()) {
        log_info("TagLibHandler {}: does not appear to be a valid ogg file", item->getLocation().c_str());
        return;
    }
    auto sc = converterManager->i2i();
    populateGenericTags(item, *oggFile, oggFile->properties(), sc);

    if (!oggFile->tag()) {
        log_debug("{}: has no tags", item->getLocation().c_str());
        return;
    }

    // Vorbis uses the FLAC binary picture structure...
    // https://wiki.xiph.org/VorbisComment#Cover_art
    // The unofficial COVERART field is not supported.
    const auto picList = dynamic_cast<TagLib::Ogg::XiphComment*>(oggFile->tag())->pictureList();
    if (picList.isEmpty())
        return;

    const TagLib::FLAC::Picture* pic = picList.front();
    const TagLib::ByteVector& data = pic->data();

    auto [artMimetype, err] = sc->convert(pic->mimeType().to8Bit(true));
    if (!err.empty()) {
        log_warning("{}: {}", item->getLocation().string(), err);
    }
    if (!isValidArtworkContentType(artMimetype)) {
        artMimetype = getContentTypeFromByteVector(data);
    }
    addArtworkResource(item, artMimetype);
}

void TagLibHandler::extractASF(TagLib::IOStream& roStream, const std::shared_ptr<CdsItem>& item) const
{
    auto asf = TagLib::ASF::File(&roStream);

    if (!asf.isValid()) {
        log_info("TagLibHandler {}: does not appear to be a valid asf/wma file", item->getLocation().c_str());
        return;
    }
    auto sc = converterManager->i2i();
    populateGenericTags(item, asf, asf.properties(), sc);

    setBitsPerSample(item, asf);

    const TagLib::ASF::AttributeListMap& attrListMap = asf.tag()->attributeListMap();
    if (attrListMap.contains("WM/Picture")) {
        const TagLib::ASF::AttributeList& attrList = attrListMap["WM/Picture"];
        if (attrList.isEmpty())
            return;

        const TagLib::ASF::Picture& wmpic = attrList[0].toPicture();
        if (!wmpic.isValid())
            return;

        auto [artMimetype, err] = sc->convert(wmpic.mimeType().to8Bit(true));
        if (!err.empty()) {
            log_warning("{}: {}", item->getLocation().string(), err);
        }
        if (!isValidArtworkContentType(artMimetype)) {
            artMimetype = getContentTypeFromByteVector(wmpic.picture());
        }
        addArtworkResource(item, artMimetype);
    }
}

void TagLibHandler::extractFLAC(TagLib::IOStream& roStream, const std::shared_ptr<CdsItem>& item) const
{
#if TAGLIB_MAJOR_VERSION >= 2
    auto flac = TagLib::FLAC::File(&roStream, true, TagLib::MPEG::Properties::Average, TagLib::ID3v2::FrameFactory::instance());
#else
    auto flac = TagLib::FLAC::File(&roStream, TagLib::ID3v2::FrameFactory::instance());
#endif

    if (!flac.isValid()) {
        log_info("TagLibHandler {}: does not appear to be a valid flac file", item->getLocation().c_str());
        return;
    }
    auto sc = converterManager->i2i();
    populateGenericTags(item, flac, flac.properties(), sc);

    setBitsPerSample(item, flac);

    if (flac.pictureList().isEmpty()) {
        log_debug("TagLibHandler: flac resource has no picture information");
        return;
    }
    const TagLib::FLAC::Picture* pic = flac.pictureList().front();
    const TagLib::ByteVector& data = pic->data();

    auto [artMimetype, err] = sc->convert(pic->mimeType().to8Bit(true));
    if (!err.empty()) {
        log_warning("{}: {}", item->getLocation().string(), err);
    }
    if (!isValidArtworkContentType(artMimetype)) {
        artMimetype = getContentTypeFromByteVector(data);
    }
    addArtworkResource(item, artMimetype);
}

void TagLibHandler::extractAPE(TagLib::IOStream& roStream, const std::shared_ptr<CdsItem>& item) const
{
    auto ape = TagLib::APE::File(&roStream);

    if (!ape.isValid()) {
        log_info("TagLibHandler {}: does not appear to be a valid APE file", item->getLocation().c_str());
        return;
    }
    auto sc = converterManager->i2i();
    populateGenericTags(item, ape, ape.properties(), sc);

    setBitsPerSample(item, ape);
}

void TagLibHandler::extractWavPack(TagLib::IOStream& roStream, const std::shared_ptr<CdsItem>& item) const
{
    auto wavpack = TagLib::WavPack::File(&roStream);

    if (!wavpack.isValid()) {
        log_info("TagLibHandler {}: does not appear to be a valid WavPack file", item->getLocation().c_str());
        return;
    }
    auto sc = converterManager->i2i();
    populateGenericTags(item, wavpack, wavpack.properties(), sc);

    setBitsPerSample(item, wavpack);
}

void TagLibHandler::extractMP4(TagLib::IOStream& roStream, const std::shared_ptr<CdsItem>& item) const
{
#if TAGLIB_MAJOR_VERSION >= 2
    auto mp4 = TagLib::MP4::File(&roStream, true, TagLib::MP4::Properties::Average, TagLib::MP4::ItemFactory::instance());
#else
    auto mp4 = TagLib::MP4::File(&roStream);
#endif

    if (!mp4.isValid()) {
        log_info("TagLibHandler {}: does not appear to be a valid mp4 file", item->getLocation().c_str());
        return;
    }

    auto sc = converterManager->i2i();
    populateGenericTags(item, mp4, mp4.hasMP4Tag() ? mp4.tag()->properties() : mp4.properties(), sc);

    if (!mp4.hasMP4Tag()) {
        log_debug("TagLibHandler {}: mp4 file has no tag information", item->getLocation().c_str());
        return;
    }

    setBitsPerSample(item, mp4);

    if (mp4.tag()->contains("covr")) {
        auto coverItem = mp4.tag()->item("covr");
        TagLib::MP4::CoverArtList coverArtList = coverItem.toCoverArtList();
        if (coverArtList.isEmpty()) {
            log_info("TagLibHandler {}: mp4 file has no coverart",
                item->getLocation().c_str());
            return;
        }

        const auto& coverArt = coverArtList.front();
        auto artMimetype = getContentTypeFromByteVector(coverArt.data());
        if (!artMimetype.empty()) {
            addArtworkResource(item, artMimetype);
        }
    } else {
        log_debug("TagLibHandler {}: mp4 file has no 'covr' item",
            item->getLocation().c_str());
    }
}

void TagLibHandler::extractAiff(TagLib::IOStream& roStream, const std::shared_ptr<CdsItem>& item) const
{
    auto aiff = TagLib::RIFF::AIFF::File(&roStream);

    if (!aiff.isValid()) {
        log_info("TagLibHandler {}: does not appear to be a valid AIFF file", item->getLocation().c_str());
        return;
    }

    auto sc = converterManager->i2i();
    populateGenericTags(item, aiff, aiff.properties(), sc);

    setBitsPerSample(item, aiff);
}

template <class Media>
void TagLibHandler::setBitsPerSample(const std::shared_ptr<CdsItem>& item, Media& media) const
{
    auto audioProps = media.audioProperties();
    auto bps = audioProps->bitsPerSample();
    auto res = item->getResource(ContentHandler::DEFAULT);
    if (bps > 0) {
        res->addAttribute(ResourceAttribute::BITS_PER_SAMPLE, fmt::to_string(bps));
    }
}

#endif // HAVE_TAGLIB
