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

#include <set>

#include "common.h"
#include "context.h"
#include "util/enum_iterator.h"
#include "util/grb_fs.h"

// forward declaration
class CdsItem;
class CdsObject;
class ContentManager;
class IOHandler;

// content handler Id's
enum class ContentHandler {
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

enum metadata_fields_t {
    M_TITLE = 0,
    M_ARTIST,
    M_ALBUM,
    M_DATE,
    M_CREATION_DATE,
    M_UPNP_DATE,
    M_GENRE,
    M_DESCRIPTION,
    M_LONGDESCRIPTION,
    M_PARTNUMBER,
    M_TRACKNUMBER,
    M_ALBUMARTURI,
    M_REGION,
    /// \todo make sure that those are only used with appropriate upnp classes
    M_CREATOR,
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

    M_CONTENT_CLASS,

    M_MAX
};

using MetadataIterator = EnumIterator<metadata_fields_t, metadata_fields_t::M_TITLE, metadata_fields_t::M_MAX>;

/// \brief This class is responsible for providing access to metadata information
/// of various media.
class MetadataHandler {
protected:
    std::shared_ptr<Config> config;
    std::shared_ptr<Mime> mime;

public:
    static std::map<metadata_fields_t, std::string> mt_keys;
    /// \brief Definition of the supported metadata fields.

    explicit MetadataHandler(const std::shared_ptr<Context>& context);
    virtual ~MetadataHandler() = default;

    static void extractMetaData(const std::shared_ptr<Context>& context, const std::shared_ptr<ContentManager>& content, const std::shared_ptr<CdsItem>& item, const fs::directory_entry& dirEnt);
    static std::string getMetaFieldName(metadata_fields_t field);
    static std::unique_ptr<MetadataHandler> createHandler(const std::shared_ptr<Context>& context, const std::shared_ptr<ContentManager>& content, ContentHandler handlerType);

    /// \brief read metadata from file and add to object
    /// \param obj Object to handle
    virtual void fillMetadata(const std::shared_ptr<CdsObject>& obj) = 0;

    /// \brief stream content of object or resource to client
    /// \param obj Object to stream
    /// \param resNum number of resource
    /// \return iohandler to stream to client
    virtual std::unique_ptr<IOHandler> serveContent(const std::shared_ptr<CdsObject>& obj, int resNum) = 0;
    virtual std::string getMimeType() const;

    static std::string mapContentHandler2String(ContentHandler ch);
    static bool checkContentHandler(const std::string& contHandler);
    static ContentHandler remapContentHandler(const std::string& contHandler);
    static ContentHandler remapContentHandler(int ch);

    static metadata_fields_t remapMetaDataField(const std::string& fieldName);
};

#endif // __METADATA_HANDLER_H__
