/*GRB*

    Gerbera - https://gerbera.io/

    metacontent_handler.h - this file is part of Gerbera.

    Copyright (C) 2020-2021 Gerbera Contributors

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

/// \file metacontent_handler.h
/// \brief Definition of the Metacontent, FanArt and Subtitle classes.

#ifndef __METADATA_CONTENT_H__
#define __METADATA_CONTENT_H__

#include <filesystem>
#include <map>
namespace fs = std::filesystem;

#include "config/config.h"
#include "metadata_handler.h"

class ContentPathSetup {
public:
    explicit ContentPathSetup(std::shared_ptr<Config> config, config_option_t fileListOption, config_option_t dirListOption);
    std::vector<fs::path> getContentPath(const std::shared_ptr<CdsObject>& obj, const std::string& setting, fs::path folder = "");

private:
    const std::shared_ptr<Config> config;
    std::vector<std::string> names;
    std::map<std::string, std::string> patterns;
    std::shared_ptr<DirectoryConfigList> allTweaks;
    static std::string expandName(const std::string& name, const std::shared_ptr<CdsObject>& obj);
    bool caseSensitive;
};

/// \brief This class is responsible for populating filesystem based metadata
class MetacontentHandler : public MetadataHandler {
public:
    explicit MetacontentHandler(const std::shared_ptr<Context>& context);
};

/// \brief This class is responsible for populating filesystem based album and fan art
class FanArtHandler : public MetacontentHandler {
public:
    explicit FanArtHandler(const std::shared_ptr<Context>& context);
    void fillMetadata(std::shared_ptr<CdsObject> obj) override;
    std::unique_ptr<IOHandler> serveContent(std::shared_ptr<CdsObject> obj, int resNum) override;

private:
    static std::unique_ptr<ContentPathSetup> setup;
};

/// \brief This class is responsible for populating filesystem based album and fan art
class ContainerArtHandler : public MetacontentHandler {
public:
    explicit ContainerArtHandler(const std::shared_ptr<Context>& context);
    void fillMetadata(std::shared_ptr<CdsObject> obj) override;
    std::unique_ptr<IOHandler> serveContent(std::shared_ptr<CdsObject> obj, int resNum) override;

private:
    static std::unique_ptr<ContentPathSetup> setup;
};

/// \brief This class is responsible for populating filesystem based subtitles
class SubtitleHandler : public MetacontentHandler {
public:
    explicit SubtitleHandler(const std::shared_ptr<Context>& context);
    void fillMetadata(std::shared_ptr<CdsObject> obj) override;
    std::unique_ptr<IOHandler> serveContent(std::shared_ptr<CdsObject> obj, int resNum) override;

private:
    static std::unique_ptr<ContentPathSetup> setup;
};

/// \brief This class is responsible for reverse mapping filesystem based resources
class ResourceHandler : public MetacontentHandler {
public:
    explicit ResourceHandler(const std::shared_ptr<Context>& context);
    void fillMetadata(std::shared_ptr<CdsObject> obj) override;
    std::unique_ptr<IOHandler> serveContent(std::shared_ptr<CdsObject> obj, int resNum) override;

private:
    static std::unique_ptr<ContentPathSetup> setup;
};

#endif // __METADATA_CONTENT_H__
