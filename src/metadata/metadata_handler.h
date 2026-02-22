/*MT*

    MediaTomb - http://www.mediatomb.cc/

    metadata_handler.h - this file is part of MediaTomb.

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

/// @file metadata/metadata_handler.h
/// @brief Definition of the MetadataHandler class.
#ifndef __METADATA_HANDLER_H__
#define __METADATA_HANDLER_H__

#include "common.h"

#include <map>
#include <memory>
#include <string>
#include <vector>

// forward declarations
class CdsObject;
class CdsItem;
class CdsResource;
class Config;
class Context;
class ConverterManager;
class IOHandler;
class Mime;
enum class ConfigVal;
enum class ContentHandler;
enum class ObjectType;
enum class MetadataFields;

/// @brief This class is responsible for providing access to metadata information
class MetadataHandler {
protected:
    /// @brief access configurtion
    std::shared_ptr<Config> config;
    /// @brief access mime handler
    std::shared_ptr<Mime> mime;
    /// @brief store all mime type mappings from configuration
    std::map<std::string, std::string> mimeContentTypeMappings;

public:
    explicit MetadataHandler(const std::shared_ptr<Context>& context);
    virtual ~MetadataHandler();

    MetadataHandler(const MetadataHandler&) = delete;
    MetadataHandler& operator=(const MetadataHandler&) = delete;

    /// @brief check whether the handler is enabled for the file type
    virtual bool isEnabled(const std::string& contentType) { return true; }
    /// @brief check whether file type is supported by handler
    virtual bool isSupported(const std::string& contentType,
        bool isOggTheora,
        const std::string& mimeType,
        ObjectType mediaType) { return true; }
    /// @brief read metadata from file and add to object
    /// @param obj Object to handle
    virtual bool fillMetadata(const std::shared_ptr<CdsObject>& obj) = 0;

    /// @brief stream content of object or resource to client
    /// @param obj Object to stream
    /// @param resource the resource
    /// @return iohandler to stream to client
    virtual std::unique_ptr<IOHandler> serveContent(
        const std::shared_ptr<CdsObject>& obj,
        const std::shared_ptr<CdsResource>& resource)
        = 0;
    virtual std::string getMimeType() const { return MIMETYPE_DEFAULT; }
};

/// @brief This class is responsible for providing access to metadata information
/// of various media.
class MediaMetadataHandler : public MetadataHandler {
protected:
    /// @brief allow handler to be disabled by config
    bool enabled {};
    /// @brief allow comment generation to be disabled by config
    bool isCommentEnabled {};
    /// @brief store all found metadata tags
    std::map<std::string, std::string> metaTags;
    /// @brief store all found aux tags
    std::vector<std::string> auxTags;
    /// @brief store all found comment items
    std::map<std::string, std::string> commentMap;
    /// @brief access converter
    std::shared_ptr<ConverterManager> converterManager;
    /// @brief allow contenttypes to be disabled by config
    bool enabledContentTypes {};
    /// @brief only handle content types
    std::vector<std::string> contentTypes;

    /// @brief check mimetype validity
    static bool isValidArtworkContentType(std::string_view artMimetype);
    /// @brief create resource to store artwork image information
    static std::shared_ptr<CdsResource> addArtworkResource(
        const std::shared_ptr<CdsItem>& item,
        ContentHandler ch,
        const std::string& artMimetype);

public:
    explicit MediaMetadataHandler(
        const std::shared_ptr<Context>& context,
        ConfigVal enableOption);
    explicit MediaMetadataHandler(
        const std::shared_ptr<Context>& context,
        ConfigVal enableOption,
        ConfigVal enableContentOption,
        ConfigVal contentOption,
        ConfigVal metaOption,
        ConfigVal auxOption);
    explicit MediaMetadataHandler(
        const std::shared_ptr<Context>& context,
        ConfigVal enableOption,
        ConfigVal enableContentOption,
        ConfigVal contentOption,
        ConfigVal metaOption,
        ConfigVal auxOption,
        ConfigVal enableCommentOption,
        ConfigVal commentOption);
    ~MediaMetadataHandler() override;

    bool isEnabled(const std::string& contentType) override;
};

#endif // __METADATA_HANDLER_H__
