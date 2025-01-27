/*MT*

    MediaTomb - http://www.mediatomb.cc/

    metadata_handler.h - this file is part of MediaTomb.

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

/// \file metadata_handler.h
/// \brief Definition of the MetadataHandler class.
#ifndef __METADATA_HANDLER_H__
#define __METADATA_HANDLER_H__

#include "common.h"

#include <map>
#include <memory>
#include <string>
#include <vector>

// forward declarations
class CdsObject;
class CdsResource;
class Config;
class Context;
class ConverterManager;
class IOHandler;
class Mime;
enum class ConfigVal;
enum class ContentHandler;
enum class MetadataFields;

/// \brief This class is responsible for providing access to metadata information
class MetadataHandler {
protected:
    std::shared_ptr<Config> config;
    std::shared_ptr<Mime> mime;
    std::map<std::string, std::string> mimeContentTypeMappings;

public:
    explicit MetadataHandler(const std::shared_ptr<Context>& context);
    virtual ~MetadataHandler() = default;

    /// \brief read metadata from file and add to object
    /// \param obj Object to handle
    virtual void fillMetadata(const std::shared_ptr<CdsObject>& obj) = 0;

    /// \brief stream content of object or resource to client
    /// \param obj Object to stream
    /// \param resource the resource
    /// \return iohandler to stream to client
    virtual std::unique_ptr<IOHandler> serveContent(const std::shared_ptr<CdsObject>& obj, const std::shared_ptr<CdsResource>& resource) = 0;
    virtual std::string getMimeType() const { return MIMETYPE_DEFAULT; }
};

/// \brief This class is responsible for providing access to metadata information
/// of various media.
class MediaMetadataHandler : public MetadataHandler {
protected:
    bool isEnabled {};
    bool isCommentEnabled {};
    std::map<std::string, std::string> metaTags;
    std::vector<std::string> auxTags;
    std::map<std::string, std::string> commentMap;
    std::shared_ptr<ConverterManager> converterManager;

public:
    explicit MediaMetadataHandler(
        const std::shared_ptr<Context>& context,
        ConfigVal enableOption);
    explicit MediaMetadataHandler(
        const std::shared_ptr<Context>& context,
        ConfigVal enableOption,
        ConfigVal metaOption,
        ConfigVal auxOption);
    explicit MediaMetadataHandler(
        const std::shared_ptr<Context>& context,
        ConfigVal enableOption,
        ConfigVal metaOption,
        ConfigVal auxOption,
        ConfigVal enableCommentOption,
        ConfigVal commentOption);
    virtual ~MediaMetadataHandler() = default;
};

#endif // __METADATA_HANDLER_H__
