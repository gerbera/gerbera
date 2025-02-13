/*GRB*
Gerbera - https://gerbera.io/

    cds_enums.h - this file is part of Gerbera.

    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>

    Copyright (C) 2006-2010 Gena Batyan <bgeradz@mediatomb.cc>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>,
                            Leonhard Wimmer <leo@mediatomb.cc>

    Copyright (C) 2016-2025 Gerbera Contributors

    Gerbera is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.

    Gerbera is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Gerbera.  If not, see <http://www.gnu.org/licenses/>.
*/

/// \file cds_enums.h

#ifndef __CDS_TYPES_H__
#define __CDS_TYPES_H__

#include <map>
#include <string>

// ATTENTION: These values need to be changed in web/js/items.js too.
#define OBJECT_TYPE_CONTAINER 0x00000001u
#define OBJECT_TYPE_ITEM 0x00000002u
#define OBJECT_TYPE_ITEM_EXTERNAL_URL 0x00000008u

#define STRING_OBJECT_TYPE_CONTAINER "container"
#define STRING_OBJECT_TYPE_ITEM "item"
#define STRING_OBJECT_TYPE_EXTERNAL_URL "external_url"

static constexpr bool IS_CDS_CONTAINER(unsigned int type)
{
    return type & OBJECT_TYPE_CONTAINER;
}
static constexpr bool IS_CDS_ITEM_EXTERNAL_URL(unsigned int type)
{
    return type & OBJECT_TYPE_ITEM_EXTERNAL_URL;
}

#define OBJECT_FLAG_RESTRICTED 0x00000001u
#define OBJECT_FLAG_SEARCHABLE 0x00000002u
#define OBJECT_FLAG_USE_RESOURCE_REF 0x00000004u
#define OBJECT_FLAG_PERSISTENT_CONTAINER 0x00000008u
#define OBJECT_FLAG_PLAYLIST_REF 0x00000010u
#define OBJECT_FLAG_PROXY_URL 0x00000020u
#define OBJECT_FLAG_ONLINE_SERVICE 0x00000040u
#define OBJECT_FLAG_OGG_THEORA 0x00000080u

enum class AutoscanType : int {
    None = 0,
    Ui = 1,
    Config = 2,
};

enum class ResourcePurpose : int {
    Content = 0,
    Thumbnail,
    Subtitle,
    Transcode,
};

enum class ObjectType : int {
    Unknown = 0,
    Folder,
    Playlist,
    Audio,
    Video,
    Image,
#ifdef ONLINE_SERVICES
    OnlineService,
#endif
};

enum class ResourceAttribute : int {
    SIZE = 0,
    DURATION,
    BITRATE,
    SAMPLEFREQUENCY,
    NRAUDIOCHANNELS,
    RESOLUTION,
    COLORDEPTH,
    PROTOCOLINFO,
    RESOURCE_FILE,
    TYPE,
    FANART_OBJ_ID,
    FANART_RES_ID,
    BITS_PER_SAMPLE,
    LANGUAGE,
    AUDIOCODEC,
    VIDEOCODEC,
    FORMAT,
    ORIENTATION,
    PIXELFORMAT,
    MAX
};

// content handler Id's
enum class ContentHandler : int {
    DEFAULT = 0,
    LIBEXIF = 1,
    ID3 = 2,
    TRANSCODE = 3,
    EXTURL = 4,
    MP4 = 5,
    FFTH = 6,
    FLAC = 7,
    FANART = 8,
    MATROSKA = 9,
    SUBTITLE = 10,
    CONTAINERART = 11,
    WAVPACK = 12,
    METAFILE = 13,
    RESOURCE = 20,
};

class EnumMapper {
private:
    inline static const std::map<ResourcePurpose, std::string> purposeToDisplay {
        { ResourcePurpose::Content, "Content" },
        { ResourcePurpose::Thumbnail, "Thumbnail" },
        { ResourcePurpose::Subtitle, "Subtitle" },
        { ResourcePurpose::Transcode, "Transcode" },
    };
    inline static const std::map<ResourceAttribute, std::string> attrToName {
        { ResourceAttribute::SIZE, "size" },
        { ResourceAttribute::DURATION, "duration" },
        { ResourceAttribute::BITRATE, "bitrate" },
        { ResourceAttribute::SAMPLEFREQUENCY, "sampleFrequency" },
        { ResourceAttribute::NRAUDIOCHANNELS, "nrAudioChannels" },
        { ResourceAttribute::RESOLUTION, "resolution" },
        { ResourceAttribute::COLORDEPTH, "colorDepth" },
        { ResourceAttribute::PROTOCOLINFO, "protocolInfo" },
        { ResourceAttribute::RESOURCE_FILE, "resFile" },
        { ResourceAttribute::FANART_OBJ_ID, "fanArtObject" },
        { ResourceAttribute::FANART_RES_ID, "fanArtResource" },
        { ResourceAttribute::BITS_PER_SAMPLE, "bitsPerSample" },
        { ResourceAttribute::LANGUAGE, "dc:language" },
        { ResourceAttribute::AUDIOCODEC, "sec:acodec" },
        { ResourceAttribute::VIDEOCODEC, "sec:vcodec" },
        { ResourceAttribute::FORMAT, "format" },
        { ResourceAttribute::TYPE, "type" },
        { ResourceAttribute::ORIENTATION, "orientation" },
        { ResourceAttribute::PIXELFORMAT, "pixelFormat" },
        { ResourceAttribute::MAX, "unknown" },
    };
    inline static const std::map<ResourceAttribute, std::string> attrToDisplay {
        { ResourceAttribute::SIZE, "size" },
        { ResourceAttribute::DURATION, "duration" },
        { ResourceAttribute::BITRATE, "bitrate" },
        { ResourceAttribute::SAMPLEFREQUENCY, "sampleFrequency" },
        { ResourceAttribute::NRAUDIOCHANNELS, "nrAudioChannels" },
        { ResourceAttribute::RESOLUTION, "resolution" },
        { ResourceAttribute::COLORDEPTH, "colorDepth" },
        { ResourceAttribute::PROTOCOLINFO, "protocolInfo" },
        { ResourceAttribute::RESOURCE_FILE, "resFile" },
        { ResourceAttribute::FANART_OBJ_ID, "fanArtObject" },
        { ResourceAttribute::FANART_RES_ID, "fanArtResource" },
        { ResourceAttribute::BITS_PER_SAMPLE, "bitsPerSample" },
        { ResourceAttribute::LANGUAGE, "language" },
        { ResourceAttribute::AUDIOCODEC, "audioCodec" },
        { ResourceAttribute::VIDEOCODEC, "videoCodec" },
        { ResourceAttribute::FORMAT, "format" },
        { ResourceAttribute::TYPE, "type" },
        { ResourceAttribute::ORIENTATION, "orientation" },
        { ResourceAttribute::PIXELFORMAT, "pixelFormat" },
        { ResourceAttribute::MAX, "unknown" },
    };

public:
    static std::string mapContentHandler2String(ContentHandler ch);
    static bool checkContentHandler(const std::string& contHandler);
    static ContentHandler remapContentHandler(const std::string& contHandler);
    static ContentHandler remapContentHandler(int ch);

    static ResourcePurpose mapPurpose(const std::string& name);
    static ResourcePurpose remapPurpose(int ip) { return static_cast<ResourcePurpose>(ip); }
    static std::string getPurposeDisplay(ResourcePurpose purpose);

    static std::string getAttributeName(ResourceAttribute attr);
    static std::string getAttributeDisplay(ResourceAttribute attr);
    static ResourceAttribute mapAttributeName(const std::string& name);
};

#endif
