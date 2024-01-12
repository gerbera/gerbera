/*MT*

    MediaTomb - http://www.mediatomb.cc/

    metadata_handler.h - this file is part of MediaTomb.

    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>

    Copyright (C) 2006-2010 Gena Batyan <bgeradz@mediatomb.cc>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>,
                            Leonhard Wimmer <leo@mediatomb.cc>

    Copyright (C) 2016-2024 Gerbera Contributors

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
class CdsResource;
class ContentManager;
class IOHandler;
enum class ContentHandler;

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

const static auto mt_single = std::map<metadata_fields_t, bool> {
    std::pair(M_TITLE, true),
    std::pair(M_ARTIST, false),
    std::pair(M_ALBUM, true),
    std::pair(M_DATE, true),
    std::pair(M_CREATION_DATE, true),
    std::pair(M_UPNP_DATE, true),
    std::pair(M_GENRE, false),
    std::pair(M_DESCRIPTION, false),
    std::pair(M_LONGDESCRIPTION, false),
    std::pair(M_PARTNUMBER, true),
    std::pair(M_TRACKNUMBER, true),
    std::pair(M_ALBUMARTURI, false),
    std::pair(M_REGION, false),
    std::pair(M_CREATOR, false),
    std::pair(M_AUTHOR, false),
    std::pair(M_DIRECTOR, false),
    std::pair(M_PUBLISHER, false),
    std::pair(M_RATING, true),
    std::pair(M_ACTOR, false),
    std::pair(M_PRODUCER, false),
    std::pair(M_ALBUMARTIST, false),
    std::pair(M_COMPOSER, false),
    std::pair(M_CONDUCTOR, false),
    std::pair(M_ORCHESTRA, false),
    std::pair(M_CONTENT_CLASS, true),
};
using MetadataIterator = EnumIterator<metadata_fields_t, metadata_fields_t::M_TITLE, metadata_fields_t::M_MAX>;

/// \brief This class is responsible for providing access to metadata information
/// of various media.
class MetadataHandler {
protected:
    std::shared_ptr<Config> config;
    std::shared_ptr<Mime> mime;

public:
    /// \brief Definition of the supported metadata fields.
    static std::map<metadata_fields_t, std::string> mt_keys;

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
    /// \param resource the resource
    /// \return iohandler to stream to client
    virtual std::unique_ptr<IOHandler> serveContent(const std::shared_ptr<CdsObject>& obj, const std::shared_ptr<CdsResource>& resource) = 0;
    virtual std::string getMimeType() const;

    static metadata_fields_t remapMetaDataField(const std::string& fieldName);
};

#endif // __METADATA_HANDLER_H__
