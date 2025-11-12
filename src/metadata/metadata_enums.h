/*MT*

    MediaTomb - http://www.mediatomb.cc/

    metadata_types.h - this file is part of MediaTomb.

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

/// @file metadata/metadata_enums.h

#ifndef __METADATA_ENUMS_H__
#define __METADATA_ENUMS_H__

#include "util/logger.h"

#include "util/enum_iterator.h"

#include <map>
#include <string>
#include <utility>

#define CONTENT_TYPE_AIFF "aiff"
#define CONTENT_TYPE_APE "ape"
#define CONTENT_TYPE_ASF "asf"
#define CONTENT_TYPE_AVI "avi"
#define CONTENT_TYPE_DSD "dsd"
#define CONTENT_TYPE_FLAC "flac"
#define CONTENT_TYPE_JPG "jpg"
#define CONTENT_TYPE_MKA "mka"
#define CONTENT_TYPE_MKV "mkv"
#define CONTENT_TYPE_MP3 "mp3"
#define CONTENT_TYPE_MP4 "mp4"
#define CONTENT_TYPE_MPEG "mpeg"
#define CONTENT_TYPE_OGG "ogg"
#define CONTENT_TYPE_PCM "pcm"
#define CONTENT_TYPE_PLAYLIST "playlist"
#define CONTENT_TYPE_PNG "png"
#define CONTENT_TYPE_WAVPACK "wv"
#define CONTENT_TYPE_WMA "wma"

#define MIME_TYPE_ASX_PLAYLIST "video/x-ms-asx"
#define MIME_TYPE_SRT_SUBTITLE "application/x-srt"

#define DC_DATE "dc:date"
#define DC_TITLE "dc:title"

/// @brief Known metadata fields
enum class MetadataFields {
    /// @brief dc:title
    M_TITLE = 0,
    /// @brief upnp:artist
    M_ARTIST,
    /// @brief upnp:album
    M_ALBUM,
    /// @brief dc:date
    M_DATE,
    /// @brief dc:created
    M_CREATION_DATE,
    /// @brief upnp:date
    M_UPNP_DATE,
    /// @brief upnp:genre
    M_GENRE,
    /// @brief dc:description
    M_DESCRIPTION,
    /// @brief upnp:longDescription
    M_LONGDESCRIPTION,
    /// @brief upnp:episodeSeason
    M_PARTNUMBER,
    /// @brief upnp:originalTrackNumber
    M_TRACKNUMBER,
    /// @brief upnp:albumArtURI
    M_ALBUMARTURI,
    /// @brief upnp:region
    M_REGION,
    /// @brief dc:creator
    M_CREATOR,
    /// @brief upnp:author
    M_AUTHOR,
    /// @brief upnp:director
    M_DIRECTOR,
    /// @brief dc:publisher
    M_PUBLISHER,
    /// @brief upnp:rating
    M_RATING,
    /// @brief upnp:actor
    M_ACTOR,
    /// @brief upnp:producer
    M_PRODUCER,
    /// @brief upnp:artist\@role[AlbumArtist]
    M_ALBUMARTIST,

    // Classical Music Related Fields
    /// @brief upnp:composer
    M_COMPOSER,
    /// @brief upnp:conductor
    M_CONDUCTOR,
    /// @brief upnp:orchestra
    M_ORCHESTRA,

    /// @brief For containers on Samsung
    M_CONTENT_CLASS,
    /// @brief For shortcut feature
    M_UPNP_SHORTCUT,

    /// @brief End of enum
    M_MAX
};

using MetadataIterator = EnumIterator<MetadataFields, MetadataFields::M_TITLE, MetadataFields::M_MAX>;

/// @brief Define single valued metadata
const static auto mt_single = std::map<MetadataFields, bool> {
    std::pair(MetadataFields::M_TITLE, true),
    std::pair(MetadataFields::M_ARTIST, false),
    std::pair(MetadataFields::M_ALBUM, true),
    std::pair(MetadataFields::M_DATE, true),
    std::pair(MetadataFields::M_CREATION_DATE, true),
    std::pair(MetadataFields::M_UPNP_DATE, true),
    std::pair(MetadataFields::M_GENRE, false),
    std::pair(MetadataFields::M_DESCRIPTION, false),
    std::pair(MetadataFields::M_LONGDESCRIPTION, false),
    std::pair(MetadataFields::M_PARTNUMBER, true),
    std::pair(MetadataFields::M_TRACKNUMBER, true),
    std::pair(MetadataFields::M_ALBUMARTURI, false),
    std::pair(MetadataFields::M_REGION, false),
    std::pair(MetadataFields::M_CREATOR, false),
    std::pair(MetadataFields::M_AUTHOR, false),
    std::pair(MetadataFields::M_DIRECTOR, false),
    std::pair(MetadataFields::M_PUBLISHER, false),
    std::pair(MetadataFields::M_RATING, true),
    std::pair(MetadataFields::M_ACTOR, false),
    std::pair(MetadataFields::M_PRODUCER, false),
    std::pair(MetadataFields::M_ALBUMARTIST, false),
    std::pair(MetadataFields::M_COMPOSER, false),
    std::pair(MetadataFields::M_CONDUCTOR, false),
    std::pair(MetadataFields::M_ORCHESTRA, false),
    std::pair(MetadataFields::M_CONTENT_CLASS, true),
    std::pair(MetadataFields::M_UPNP_SHORTCUT, true),
};

class MetaEnumMapper {
public:
    /// @brief Definition of the supported metadata fields.
    static std::map<MetadataFields, std::string> mt_keys;
    static MetadataFields remapMetaDataField(const std::string& fieldName);
    static std::string getMetaFieldName(MetadataFields field);
};

#endif
