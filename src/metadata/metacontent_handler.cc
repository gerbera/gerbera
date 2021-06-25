/*GRB*

    Gerbera - https://gerbera.io/

    metacontent_handler.cc - this file is part of Gerbera.

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

/// \file metacontent_handler.cc

#include "metacontent_handler.h" // API

#include <regex>
#include <sys/stat.h>

#include "cds_objects.h"
#include "config/config.h"
#include "config/directory_tweak.h"
#include "iohandler/file_io_handler.h"
#include "util/mime.h"
#include "util/tools.h"

ContentPathSetup::ContentPathSetup(std::shared_ptr<Config> config, config_option_t fileListOption, config_option_t dirListOption)
    : config(std::move(config))
    , names(this->config->getArrayOption(fileListOption))
    , patterns(this->config->getDictionaryOption(dirListOption))
    , allTweaks(this->config->getDirectoryTweakOption(CFG_IMPORT_DIRECTORIES_LIST))
    , caseSensitive(this->config->getBoolOption(CFG_IMPORT_RESOURCES_CASE_SENSITIVE))
{
}

std::vector<fs::path> ContentPathSetup::getContentPath(const std::shared_ptr<CdsObject>& obj, const std::string& setting, fs::path folder)
{
    auto tweak = allTweaks->get(obj->getLocation());
    auto files = tweak == nullptr || !tweak->hasSetting(setting) ? this->names : std::vector<std::string> { tweak->getSetting(setting) };
    auto isCaseSensitive = tweak != nullptr && tweak->hasCaseSensitive() ? tweak->getCaseSensitive() : this->caseSensitive;

    std::vector<fs::path> result;

    if (!files.empty()) {
        if (folder.empty()) {
            folder = (obj->isContainer()) ? obj->getLocation() : obj->getLocation().parent_path();
        }
        log_debug("Folder name: {}", folder.c_str());

        if (isCaseSensitive) {
            for (auto&& name : files) {
                auto found = folder / expandName(name, obj);
                std::error_code ec;
                bool exists = isRegularFile(found, ec); // no error throwing, please
                if (!exists)
                    continue;

                log_debug("{}: found", found.c_str());
                result.push_back(found);
            }
        } else {
            std::map<std::string, fs::path> fileNames;
            std::error_code ec;
            for (auto&& p : fs::directory_iterator(folder, ec))
                if (isRegularFile(p, ec))
                    fileNames[toLower(p.path().filename().string())] = p;

            for (auto&& name : files) {
                auto fileName = toLower(expandName(name, obj));
                for (auto&& [f, s] : fileNames) {
                    if (f == fileName) {
                        log_debug("{}: found", f.c_str());
                        result.push_back(s);
                    }
                }
            }
        }
        if (!patterns.empty()) {
            for (auto&& [dir, ext] : patterns) {
                auto found = fs::path(expandName(dir, obj));
                auto extn = fs::path(expandName(ext, obj));
                auto stem = isCaseSensitive ? extn.stem().string() : toLower(extn.stem().string());
                if (extn.has_extension()) {
                    extn = isCaseSensitive ? extn.extension().string() : toLower(extn.extension().string());
                } else {
                    extn = isCaseSensitive ? fmt::format(".{}", ext) : toLower(fmt::format(".{}", ext));
                    stem = "";
                }
                std::error_code ec;
                if (found.is_relative()) {
                    found = folder / found;
                }
                if (!fs::is_directory(found)) {
                    log_debug("{}: not a directory", found.string());
                    continue;
                }
                for (auto&& p : fs::directory_iterator(found, ec)) {
                    if (isRegularFile(p, ec) && ((isCaseSensitive && p.path().extension() == extn) || (!isCaseSensitive && toLower(p.path().extension().string()) == extn))) {
                        if (!stem.empty()) {
                            replaceAllString(stem, "*", ".*");
                            replaceAllString(stem, ".", "?");
                            std::regex re(fmt::format("^{}$", stem));
                            std::cmatch m;
                            if (std::regex_match(p.path().stem().string().c_str(), m, re)) {
                                log_debug("{}: found", p.path().string());
                                result.push_back(p.path());
                            }
                        } else {
                            log_debug("{}: found", p.path().string());
                            result.push_back(p.path());
                        }
                    }
                }
            }
        }
    }
    if (result.empty())
        result.push_back("");
    return result;
}

static constexpr std::array<std::pair<std::string_view, metadata_fields_t>, 5> metaTags { {
    { "%album%", M_ALBUM },
    { "%albumArtist%", M_ALBUMARTIST },
    { "%artist%", M_ARTIST },
    { "%genre%", M_GENRE },
    { "%title%", M_TITLE },
} };

std::string ContentPathSetup::expandName(const std::string& name, const std::shared_ptr<CdsObject>& obj)
{
    std::string copy(name);

    for (auto&& [key, val] : metaTags)
        replaceString(copy, key, obj->getMetadata(val));

    if (obj->isItem()) {
        fs::path location = obj->getLocation();
        replaceString(copy, "%filename%", location.stem().string().c_str());
    }
    if (obj->isContainer()) {
        auto title = obj->getTitle();
        if (!title.empty())
            replaceString(copy, "%filename%", title);
        fs::path location = obj->getLocation();
        replaceString(copy, "%filename%", location.filename().string().c_str());
    }
    return copy;
}

MetacontentHandler::MetacontentHandler(const std::shared_ptr<Context>& context)
    : MetadataHandler(context)
{
}

std::unique_ptr<ContentPathSetup> FanArtHandler::setup {};

FanArtHandler::FanArtHandler(const std::shared_ptr<Context>& context)
    : MetacontentHandler(context)
{
    if (!setup) {
        setup = std::make_unique<ContentPathSetup>(config, CFG_IMPORT_RESOURCES_FANART_FILE_LIST, CFG_IMPORT_RESOURCES_FANART_DIR_LIST);
    }
}

void FanArtHandler::fillMetadata(std::shared_ptr<CdsObject> obj)
{
    log_debug("Running fanart handler on {}", obj->getLocation().c_str());
    auto pathList = setup->getContentPath(obj, SETTING_FANART);

    if (pathList.empty() || pathList[0].empty())
        obj->removeResource(CH_FANART);

    for (auto&& path : pathList) {
        if (!path.empty()) {
            auto resource = std::make_shared<CdsResource>(CH_FANART);
            std::string type = path.extension().string().substr(1);
            std::string mimeType = mime->getMimeType(path, fmt::format("image/{}", type));

            resource->addAttribute(R_PROTOCOLINFO, renderProtocolInfo(mimeType));
            resource->addAttribute(R_RESOURCE_FILE, path.string());
            resource->addParameter(RESOURCE_CONTENT_TYPE, ID3_ALBUM_ART);
            obj->addResource(resource);
        }
    }
}

std::unique_ptr<IOHandler> FanArtHandler::serveContent(std::shared_ptr<CdsObject> obj, int resNum)
{
    fs::path path = obj->getResource(resNum)->getAttribute(R_RESOURCE_FILE);
    if (path.empty()) {
        path = setup->getContentPath(obj, SETTING_FANART)[0];
    }
    log_debug("FanArt: Opening name: {}", path.c_str());
    struct stat statbuf;
    int ret = stat(path.c_str(), &statbuf);
    if (ret != 0) {
        log_warning("File does not exist: {} ({})", path.c_str(), std::strerror(errno));
        return nullptr;
    }
    return std::make_unique<FileIOHandler>(path);
}

std::unique_ptr<ContentPathSetup> ContainerArtHandler::setup {};

ContainerArtHandler::ContainerArtHandler(const std::shared_ptr<Context>& context)
    : MetacontentHandler(context)
{
    if (!setup) {
        setup = std::make_unique<ContentPathSetup>(config, CFG_IMPORT_RESOURCES_CONTAINERART_FILE_LIST, CFG_IMPORT_RESOURCES_CONTAINERART_DIR_LIST);
    }
}

void ContainerArtHandler::fillMetadata(std::shared_ptr<CdsObject> obj)
{
    auto pathList = setup->getContentPath(obj, SETTING_CONTAINERART, config->getOption(CFG_IMPORT_RESOURCES_CONTAINERART_LOCATION));
    if (pathList.empty() || pathList[0].empty()) {
        pathList = setup->getContentPath(obj, SETTING_CONTAINERART);
    }

    if (pathList.empty() || pathList[0].empty())
        obj->removeResource(CH_CONTAINERART);

    for (auto&& path : pathList) {
        log_debug("Running ContainerArt handler on {}", !path.empty() ? path.c_str() : obj->getLocation().c_str());
        if (!path.empty()) {
            auto resource = std::make_shared<CdsResource>(CH_CONTAINERART);
            std::string type = path.extension().string().substr(1);
            std::string mimeType = mime->getMimeType(path, fmt::format("image/{}", type));

            resource->addAttribute(R_PROTOCOLINFO, renderProtocolInfo(mimeType));
            resource->addAttribute(R_RESOURCE_FILE, path.string());
            resource->addParameter(RESOURCE_CONTENT_TYPE, ID3_ALBUM_ART);
            obj->addResource(resource);
        }
    }
}

std::unique_ptr<IOHandler> ContainerArtHandler::serveContent(std::shared_ptr<CdsObject> obj, int resNum)
{
    fs::path path = obj->getResource(resNum)->getAttribute(R_RESOURCE_FILE);
    if (path.empty()) {
        path = setup->getContentPath(obj, SETTING_CONTAINERART, config->getOption(CFG_IMPORT_RESOURCES_CONTAINERART_LOCATION))[0];
        if (path.empty()) {
            path = setup->getContentPath(obj, SETTING_CONTAINERART)[0];
        }
    }
    log_debug("ContainerArt: Opening name: {}", path.c_str());
    struct stat statbuf;
    int ret = stat(path.c_str(), &statbuf);
    if (ret != 0) {
        log_warning("File does not exist: {} ({})", path.c_str(), std::strerror(errno));
        return nullptr;
    }
    return std::make_unique<FileIOHandler>(path);
}

std::unique_ptr<ContentPathSetup> SubtitleHandler::setup {};

SubtitleHandler::SubtitleHandler(const std::shared_ptr<Context>& context)
    : MetacontentHandler(context)
{
    if (!setup) {
        setup = std::make_unique<ContentPathSetup>(config, CFG_IMPORT_RESOURCES_SUBTITLE_FILE_LIST, CFG_IMPORT_RESOURCES_SUBTITLE_DIR_LIST);
    }
}

void SubtitleHandler::fillMetadata(std::shared_ptr<CdsObject> obj)
{
    auto pathList = setup->getContentPath(obj, SETTING_SUBTITLE);

    if (pathList.empty() || pathList[0].empty())
        obj->removeResource(CH_SUBTITLE);

    for (auto&& path : pathList) {
        log_debug("Running subtitle handler on {} -> {}", obj->getLocation().c_str(), path.c_str());
        if (!path.empty()) {
            auto resource = std::make_shared<CdsResource>(CH_SUBTITLE);
            std::string type = path.extension().string().substr(1);

            std::string mimeType = mime->getMimeType(path, fmt::format("text/{}", type));
            auto pos = mimeType.find("plain");
            if (pos != std::string::npos) {
                mimeType = fmt::format("{}{}", mimeType.substr(0, pos), type);
            }

            resource->addAttribute(R_PROTOCOLINFO, renderProtocolInfo(mimeType));
            resource->addAttribute(R_RESOURCE_FILE, path.string());
            resource->addAttribute(R_LANGUAGE, path.stem().string()); // assume file name is related to some language
            resource->addParameter(RESOURCE_CONTENT_TYPE, VIDEO_SUB);
            resource->addParameter("type", type);
            obj->addResource(resource);
        }
    }
}

std::unique_ptr<IOHandler> SubtitleHandler::serveContent(std::shared_ptr<CdsObject> obj, int resNum)
{
    fs::path path = obj->getResource(resNum)->getAttribute(R_RESOURCE_FILE);
    if (path.empty()) {
        path = setup->getContentPath(obj, SETTING_SUBTITLE)[0];
    }
    log_debug("Subtitle: Opening name: {}", path.c_str());
    struct stat statbuf;
    int ret = stat(path.c_str(), &statbuf);
    if (ret != 0) {
        log_warning("File does not exist: {} ({})", path.c_str(), std::strerror(errno));
        return nullptr;
    }
    return std::make_unique<FileIOHandler>(path);
}

std::unique_ptr<ContentPathSetup> ResourceHandler::setup {};

ResourceHandler::ResourceHandler(const std::shared_ptr<Context>& context)
    : MetacontentHandler(context)
{
    if (!setup) {
        setup = std::make_unique<ContentPathSetup>(config, CFG_IMPORT_RESOURCES_RESOURCE_FILE_LIST, CFG_IMPORT_RESOURCES_RESOURCE_DIR_LIST);
    }
}

void ResourceHandler::fillMetadata(std::shared_ptr<CdsObject> obj)
{
    auto pathList = setup->getContentPath(obj, SETTING_RESOURCE);

    if (pathList.empty() || pathList[0].empty())
        obj->removeResource(CH_RESOURCE);

    for (auto&& path : pathList) {
        log_debug("Running resource handler check on {} -> {}", obj->getLocation().string().c_str(), path.string().c_str());

        if (!path.empty() && toLower(path.string()) == toLower(obj->getLocation().string())) {
            auto resource = std::make_shared<CdsResource>(CH_RESOURCE);
            resource->addAttribute(R_PROTOCOLINFO, renderProtocolInfo("res"));
            resource->addAttribute(R_RESOURCE_FILE, path.string());
            obj->addResource(resource);
        }
    }
}

std::unique_ptr<IOHandler> ResourceHandler::serveContent(std::shared_ptr<CdsObject> obj, int resNum)
{
    fs::path path = obj->getResource(resNum)->getAttribute(R_RESOURCE_FILE);
    if (path.empty()) {
        path = setup->getContentPath(obj, SETTING_RESOURCE)[0];
    }
    log_debug("Resource: Opening name: {}", path.c_str());
    struct stat statbuf;
    int ret = stat(path.c_str(), &statbuf);
    if (ret != 0) {
        log_warning("File does not exist: {} ({})", path.c_str(), std::strerror(errno));
        return nullptr;
    }
    return std::make_unique<FileIOHandler>(path);
}
