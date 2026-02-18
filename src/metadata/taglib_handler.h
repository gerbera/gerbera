/*MT*

    MediaTomb - http://www.mediatomb.cc/

    taglib_handler.h - this file is part of MediaTomb.

    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>

    Copyright (C) 2006-2010 Gena Batyan <bgeradz@mediatomb.cc>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>,
                            Leonhard Wimmer <leo@mediatomb.cc>

    Copyright (C) 2016-2026 Gerbera Contributors

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

/// @file metadata/taglib_handler.h
/// @brief Definition of the TagHandler class.

#ifndef __METADATA_TAGLIB_H__
#define __METADATA_TAGLIB_H__

#ifdef HAVE_TAGLIB

#include "metadata_handler.h"

#include <map>
#include <tbytevector.h>
#include <tfile.h>
#include <tiostream.h>

// forward declarations
class CdsItem;
class StringConverter;

/// @brief This class is responsible for reading id3 or ogg tags metadata using taglib
class TagLibHandler : public MediaMetadataHandler {
public:
    explicit TagLibHandler(const std::shared_ptr<Context>& context);

    bool isSupported(const std::string& contentType,
        bool isOggTheora,
        const std::string& mimeType,
        ObjectType mediaType) override;
    /// @brief read metadata from file and add to object
    /// @param obj Object to handle
    bool fillMetadata(const std::shared_ptr<CdsObject>& obj) override;

    /// @brief stream content of object or resource to client
    /// @param obj Object to stream
    /// @param resource the resource
    /// @return iohandler to stream to client
    std::unique_ptr<IOHandler> serveContent(
        const std::shared_ptr<CdsObject>& obj,
        const std::shared_ptr<CdsResource>& resource) override;

private:
    std::string entrySeparator;
    std::string legacyEntrySeparator;

    /// @brief add metadata field from file content
    void addField(
        MetadataFields field,
        const TagLib::File& file,
        const TagLib::Tag* tag,
        const std::shared_ptr<CdsItem>& item) const;
    /// @brief add special field from file content
    void addSpecialFields(
        const TagLib::File& file,
        const TagLib::Tag* tag,
        const std::shared_ptr<CdsItem>& item) const;

    /// @brief open ogg file depending on sub type
    static std::unique_ptr<TagLib::File> getOggFile(TagLib::IOStream& ioStream);

    /// @brief read general tag
    void populateGenericTags(
        const std::shared_ptr<CdsItem>& item,
        const std::shared_ptr<CdsResource>& res,
        const TagLib::File& file,
        const TagLib::PropertyMap& propertyMap,
        const std::shared_ptr<StringConverter>& sc) const;
    /// @brief fill aux tags
    void populateAuxTags(
        const std::shared_ptr<CdsItem>& item,
        const TagLib::PropertyMap& propertyMap,
        const std::shared_ptr<StringConverter>& sc) const;
    /// @brief fabricate comment
    void makeComment(
        const std::shared_ptr<CdsItem>& item,
        const TagLib::PropertyMap& propertyMap,
        const std::shared_ptr<StringConverter>& sc) const;
    std::string getContentTypeFromByteVector(const TagLib::ByteVector& data) const;
    /// @brief read specific mp3 data
    void extractMP3(
        TagLib::IOStream& roStream,
        const std::shared_ptr<CdsItem>& item,
        const std::shared_ptr<CdsResource>& res) const;
    /// @brief read specific ogg vorbis data
    void extractOgg(
        TagLib::IOStream& roStream,
        const std::shared_ptr<CdsItem>& item,
        const std::shared_ptr<CdsResource>& res) const;
    /// @brief read specific asf data
    void extractASF(
        TagLib::IOStream& roStream,
        const std::shared_ptr<CdsItem>& item,
        const std::shared_ptr<CdsResource>& res) const;
    /// @brief read specific flac data
    void extractFLAC(
        TagLib::IOStream& roStream,
        const std::shared_ptr<CdsItem>& item,
        const std::shared_ptr<CdsResource>& res) const;
    /// @brief read specific ape data
    void extractAPE(
        TagLib::IOStream& roStream,
        const std::shared_ptr<CdsItem>& item,
        const std::shared_ptr<CdsResource>& res) const;
    /// @brief read specific wavpack data
    void extractWavPack(
        TagLib::IOStream& roStream,
        const std::shared_ptr<CdsItem>& item,
        const std::shared_ptr<CdsResource>& res) const;
    /// @brief read specific dsf data
    void extractDSF(
        TagLib::IOStream& roStream,
        const std::shared_ptr<CdsItem>& item,
        const std::shared_ptr<CdsResource>& res) const;
    /// @brief read specific mp4 data
    void extractMP4(
        TagLib::IOStream& roStream,
        const std::shared_ptr<CdsItem>& item,
        const std::shared_ptr<CdsResource>& res) const;
    /// @brief read specific aiff data
    void extractAiff(
        TagLib::IOStream& roStream,
        const std::shared_ptr<CdsItem>& item,
        const std::shared_ptr<CdsResource>& res) const;
    /// @brief read specific pcm data
    void extractPcm(
        TagLib::IOStream& roStream,
        const std::shared_ptr<CdsItem>& item,
        const std::shared_ptr<CdsResource>& res) const;
    /// @brief read specific matroska data
    void extractMkv(
        TagLib::IOStream& roStream,
        const std::shared_ptr<CdsItem>& item,
        const std::shared_ptr<CdsResource>& res) const;

    /// @brief set bits per sample based on media type
    template <class Media>
    void setBitsPerSample(
        const std::shared_ptr<CdsItem>& item,
        const std::shared_ptr<CdsResource>& res,
        Media& media) const;
};

#endif

#endif // __METADATA_TAGLIB_H__
