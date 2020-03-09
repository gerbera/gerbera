/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    metadata_handler.h - this file is part of MediaTomb.
    
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

/// \file metadata_handler.h
/// \brief Definition of the MetadataHandler class.
#ifndef __METADATA_HANDLER_H__
#define __METADATA_HANDLER_H__

#include "cds_objects.h"
#include "common.h"
#include "iohandler/io_handler.h"

// content handler Id's
constexpr auto CH_DEFAULT = 0;
constexpr auto CH_LIBEXIF = 1;
constexpr auto CH_ID3 = 2;
constexpr auto CH_TRANSCODE = 3;
constexpr auto CH_EXTURL = 4;
constexpr auto CH_MP4 = 5;
constexpr auto CH_FFTH = 6;
constexpr auto CH_FLAC = 7;
constexpr auto CH_FANART = 8;
constexpr auto CH_MATROSKA = 9;

constexpr auto CONTENT_TYPE_MP3 = "mp3";
constexpr auto CONTENT_TYPE_OGG = "ogg";
constexpr auto CONTENT_TYPE_FLAC = "flac";
constexpr auto CONTENT_TYPE_WMA = "wma";
constexpr auto CONTENT_TYPE_WAVPACK = "wv";
constexpr auto CONTENT_TYPE_APE = "ape";
constexpr auto CONTENT_TYPE_JPG = "jpg";
constexpr auto CONTENT_TYPE_PLAYLIST = "playlist";
constexpr auto CONTENT_TYPE_MP4 = "mp4";
constexpr auto CONTENT_TYPE_PCM = "pcm";
constexpr auto CONTENT_TYPE_AVI = "avi";
constexpr auto CONTENT_TYPE_MPEG = "mpeg";
constexpr auto CONTENT_TYPE_QUICKTIME = "quicktime";
constexpr auto CONTENT_TYPE_MKV = "mkv";
constexpr auto CONTENT_TYPE_MKA = "mka";
constexpr auto CONTENT_TYPE_AIFF = "aiff";
constexpr auto CONTENT_TYPE_DSD = "dsd";

constexpr auto OGG_THEORA = "t";

constexpr auto RESOURCE_CONTENT_TYPE = "rct";
constexpr auto RESOURCE_HANDLER = "rh";

constexpr auto ID3_ALBUM_ART = "aa";
constexpr auto EXIF_THUMBNAIL = "EX_TH";
constexpr auto THUMBNAIL = "th"; // thumbnail without need for special handling

typedef enum {
    M_TITLE = 0,
    M_ARTIST,
    M_ALBUM,
    M_DATE,
    M_UPNP_DATE,
    M_GENRE,
    M_DESCRIPTION,
    M_LONGDESCRIPTION,
    M_TRACKNUMBER,
    M_ALBUMARTURI,
    M_REGION,
    /// \todo make sure that those are only used with appropriate upnp classes
    M_AUTHOR,
    M_DIRECTOR,
    M_PUBLISHER,
    M_RATING,
    M_ACTOR,
    M_PRODUCER,
    M_ALBUMARTIST,

    // Classical Music Related Fields
    M_COMPOSER,
    M_CONDUCTOR,
    M_ORCHESTRA,

    M_MAX
} metadata_fields_t;

typedef struct mt_key mt_key;
struct mt_key {
    const char* sym;
    const char* upnp;
};

extern mt_key MT_KEYS[];

// res tag attributes
typedef enum {
    R_SIZE = 0,
    R_DURATION,
    R_BITRATE,
    R_SAMPLEFREQUENCY,
    R_NRAUDIOCHANNELS,
    R_RESOLUTION,
    R_COLORDEPTH,
    R_PROTOCOLINFO,
    R_MAX
} resource_attributes_t;

typedef struct res_key res_key;
struct res_key {
    const char* sym;
    const char* upnp;
};

extern res_key RES_KEYS[];

// forward declaration
class ConfigManager;

/// \brief This class is responsible for providing access to metadata information
/// of various media.
class MetadataHandler {
protected:
    std::shared_ptr<ConfigManager> config;

public:
    /// \brief Definition of the supported metadata fields.

    explicit MetadataHandler(std::shared_ptr<ConfigManager> config);

    static void setMetadata(const std::shared_ptr<ConfigManager>& config, const std::shared_ptr<CdsItem>& item);
    static std::string getMetaFieldName(metadata_fields_t field);
    static std::string getResAttrName(resource_attributes_t attr);
    static std::unique_ptr<MetadataHandler> createHandler(const std::shared_ptr<ConfigManager>& config, int handlerType);

    virtual void fillMetadata(std::shared_ptr<CdsItem> item) = 0;
    virtual std::unique_ptr<IOHandler> serveContent(std::shared_ptr<CdsItem> item, int resNum) = 0;
    virtual std::string getMimeType();
};

#endif // __METADATA_HANDLER_H__
