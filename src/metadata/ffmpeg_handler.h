/*MT*

    MediaTomb - http://www.mediatomb.cc/

    ffmpeg_handler.h - this file is part of MediaTomb.

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
/*
    This code was contributed by
    Copyright (C) 2007 Ingo Preiml <ipreiml@edu.uni-klu.ac.at>
*/

/// @file metadata/ffmpeg_handler.h
/// @brief Definition of the FfmpegHandler class - getting metadata via
/// ffmpeg library calls.

#ifndef __FFMPEG_HANDLER_H__
#define __FFMPEG_HANDLER_H__
#ifdef HAVE_FFMPEG

#include "cds/cds_enums.h"
#include "metadata_enums.h"
#include "metadata_handler.h"

#include <array>

// forward declarations
class CdsItem;
class FfmpegObject;
class IOHandler;
class StringConverter;

struct AVFormatContext;
struct AVDictionaryEntry;
struct AVStream;

/// @brief This class is responsible for reading id3 tags metadata
class FfmpegHandler : public MediaMetadataHandler {
public:
    explicit FfmpegHandler(const std::shared_ptr<Context>& context);

    bool isSupported(const std::string& contentType,
        bool isOggTheora,
        const std::string& mimeType,
        ObjectType mediaType) override;
    bool fillMetadata(const std::shared_ptr<CdsObject>& obj) override;
    std::unique_ptr<IOHandler> serveContent(
        const std::shared_ptr<CdsObject>& obj,
        const std::shared_ptr<CdsResource>& resource) override;
    std::string getMimeType() const override;

private:
    /// @brief get all AUX values as configured
    bool addFfmpegAuxdataFields(
        const std::shared_ptr<CdsItem>& item,
        const FfmpegObject& ffmpegObject) const;
    /// @brief get all metadata fields
    bool addFfmpegMetadataFields(
        const std::shared_ptr<CdsItem>& item,
        const FfmpegObject& ffmpegObject) const;
    /// @brief get one metadata field
    bool getFfmpegMetadataField(
        const std::shared_ptr<CdsItem>& item,
        const FfmpegObject& ffmpegObject,
        AVDictionaryEntry* avEntry,
        ObjectType streamType,
        std::map<MetadataFields, bool>& emptyProperties,
        std::map<std::string, bool>& emptySpecProperties) const;
    /// @brief get additional resource fields
    bool addFfmpegResourceFields(
        const std::shared_ptr<CdsItem>& item,
        const FfmpegObject& ffmpegObject);
    /// @brief fabricate comment from metadata
    bool addFfmpegComment(
        const std::shared_ptr<CdsItem>& item,
        const FfmpegObject& ffmpegObject) const;
    /// @brief try to extract mime type and content type from stream data
    std::string getContentTypeFromByteVector(const std::vector<std::uint8_t>& data) const;
    /// @brief load resource attributes from stream
    static void setResourceAttributes(
        const std::shared_ptr<CdsResource>& res,
        AVStream* st,
        long long stream_number);

    static constexpr std::array propertyMap {
        std::pair(MetadataFields::M_TITLE, "title"),
        std::pair(MetadataFields::M_ARTIST, "artist"),
        std::pair(MetadataFields::M_ALBUM, "album"),
        std::pair(MetadataFields::M_GENRE, "genre"),
        std::pair(MetadataFields::M_DESCRIPTION, "description"),
        std::pair(MetadataFields::M_TRACKNUMBER, "track"),
        std::pair(MetadataFields::M_PARTNUMBER, "discnumber"),
        std::pair(MetadataFields::M_ALBUMARTIST, "album_artist"),
        std::pair(MetadataFields::M_COMPOSER, "composer"),
        std::pair(MetadataFields::M_DATE, "date"),
        std::pair(MetadataFields::M_UPNP_DATE, "date"),
        std::pair(MetadataFields::M_CREATION_DATE, "creation_time"),
    };
    static constexpr std::array resourceMap {
        std::pair(ResourceAttribute::LANGUAGE, "language"),
        std::pair(ResourceAttribute::LYRICS, "lyrics"),
    };
    /// @brief activate separation of artwork found by handler
    bool artWorkEnabled;
    /// @brief activate reading metadata from all streams
    bool streamsEnabled;
    /// @brief number of bytes to read for mime detection of subtitles
    unsigned int subtitleSeekSize;
};

#endif // HAVE_FFMPEG
#endif //__FFMPEG_HANDLER_H__
