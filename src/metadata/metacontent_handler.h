/*GRB*

    Gerbera - https://gerbera.io/

    metacontent_handler.h - this file is part of Gerbera.

    Copyright (C) 2020-2025 Gerbera Contributors

    Gerbera is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.

    Gerbera is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Gerbera.  If not, see <http://www.gnu.org/licenses/>.

    $Id$
*/

/// @file metadata/metacontent_handler.h
/// @brief Definition of the Metacontent, FanArt and Subtitle classes.

#ifndef __METADATA_CONTENT_H__
#define __METADATA_CONTENT_H__

#include "config/config.h"
#include "metadata_handler.h"

class ConfigDefinition;
class Content;
class Database;
class StringConverter;

/// @brief This class is responsible for expanding configuration options to file names
class ContentPathSetup {
public:
    explicit ContentPathSetup(
        std::shared_ptr<Config> config,
        std::shared_ptr<Database> database,
        const std::shared_ptr<ConfigDefinition>& definition,
        ConfigVal fileListOption,
        ConfigVal dirListOption);
    std::vector<fs::path> getContentPath(
        const std::shared_ptr<CdsObject>& obj,
        const std::string& setting,
        fs::path folder = "") const;

private:
    std::shared_ptr<Config> config;
    std::shared_ptr<Database> database;
    std::vector<std::string> names;
    std::vector<std::vector<std::pair<std::string, std::string>>> patterns;
    std::shared_ptr<DirectoryConfigList> allTweaks;
    static std::string expandName(const std::string& name, const std::shared_ptr<CdsObject>& obj);
    bool caseSensitive;
    std::shared_ptr<ConfigDefinition> definition;
};

/// @brief This class is responsible for populating filesystem based metadata
class MetacontentHandler : public MetadataHandler {
public:
    explicit MetacontentHandler(const std::shared_ptr<Context>& context);
    ~MetacontentHandler() override;

protected:
    const std::shared_ptr<StringConverter> f2i;
    std::shared_ptr<ConfigDefinition> definition;
    std::shared_ptr<Database> database;
};

/// @brief This class is responsible for populating filesystem based album and fan art
class FanArtHandler : public MetacontentHandler {
public:
    explicit FanArtHandler(const std::shared_ptr<Context>& context);

    bool isSupported(const std::string& contentType,
        bool isOggTheora,
        const std::string& mimeType,
        ObjectType mediaType) override;
    bool fillMetadata(const std::shared_ptr<CdsObject>& obj) override;
    std::unique_ptr<IOHandler> serveContent(
        const std::shared_ptr<CdsObject>& obj,
        const std::shared_ptr<CdsResource>& resource) override;

private:
    static std::unique_ptr<ContentPathSetup> setup;
};

/// @brief This class is responsible for populating filesystem based album and fan art
class ContainerArtHandler : public MetacontentHandler {
public:
    explicit ContainerArtHandler(const std::shared_ptr<Context>& context);
    bool fillMetadata(const std::shared_ptr<CdsObject>& obj) override;
    std::unique_ptr<IOHandler> serveContent(
        const std::shared_ptr<CdsObject>& obj,
        const std::shared_ptr<CdsResource>& resource) override;

private:
    static std::unique_ptr<ContentPathSetup> setup;
};

/// @brief This class is responsible for populating filesystem based subtitles
class SubtitleHandler : public MetacontentHandler {
public:
    explicit SubtitleHandler(const std::shared_ptr<Context>& context);

    bool isSupported(const std::string& contentType,
        bool isOggTheora,
        const std::string& mimeType,
        ObjectType mediaType) override;
    bool fillMetadata(const std::shared_ptr<CdsObject>& obj) override;
    std::unique_ptr<IOHandler> serveContent(
        const std::shared_ptr<CdsObject>& obj,
        const std::shared_ptr<CdsResource>& resource) override;

private:
    static std::unique_ptr<ContentPathSetup> setup;
};

/// @brief This class is responsible for populating metadata from additional files
class MetafileHandler : public MetacontentHandler {
public:
    explicit MetafileHandler(const std::shared_ptr<Context>& context, std::shared_ptr<Content> content);
    bool fillMetadata(const std::shared_ptr<CdsObject>& obj) override;
    std::unique_ptr<IOHandler> serveContent(
        const std::shared_ptr<CdsObject>& obj,
        const std::shared_ptr<CdsResource>& resource) override;

private:
    static std::unique_ptr<ContentPathSetup> setup;
    std::shared_ptr<Content> content;
};

/// @brief This class is responsible for reverse mapping filesystem based resources
class ResourceHandler : public MetacontentHandler {
public:
    explicit ResourceHandler(const std::shared_ptr<Context>& context);
    bool fillMetadata(const std::shared_ptr<CdsObject>& obj) override;
    std::unique_ptr<IOHandler> serveContent(
        const std::shared_ptr<CdsObject>& obj,
        const std::shared_ptr<CdsResource>& resource) override;

private:
    static std::unique_ptr<ContentPathSetup> setup;
};

#endif // __METADATA_CONTENT_H__
