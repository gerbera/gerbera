/*GRB*

    Gerbera - https://gerbera.io/

    metacontent_handler.cc - this file is part of Gerbera.

    Copyright (C) 2020-2022 Gerbera Contributors

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

#include <array>
#include <regex>
#include <sys/stat.h>

#include "cds_objects.h"
#include "config/config.h"
#include "config/directory_tweak.h"
#include "content/content_manager.h"
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

std::vector<fs::path> ContentPathSetup::getContentPath(const std::shared_ptr<CdsObject>& obj, const std::string& setting, fs::path folder) const
{
    auto objLocation = obj->getLocation();
    auto tweak = allTweaks->get(objLocation);
    auto files = !tweak || !tweak->hasSetting(setting) ? this->names : std::vector<std::string> { tweak->getSetting(setting) };
    auto isCaseSensitive = tweak && tweak->hasCaseSensitive() ? tweak->getCaseSensitive() : this->caseSensitive;

    std::vector<fs::path> result;

    if (!files.empty()) {
        if (folder.empty()) {
            folder = (obj->isContainer()) ? objLocation : objLocation.parent_path();
        }
        log_debug("Folder name: {}", folder.c_str());

        if (isCaseSensitive) {
            for (auto&& name : files) {
                auto contentFile = folder / expandName(name, obj);
                std::error_code ec;
                if (!isRegularFile(contentFile, ec) || contentFile == objLocation) // no error throwing, please
                    continue;

                log_debug("{}: found", contentFile.c_str());
                result.push_back(std::move(contentFile));
            }
        } else {
            std::map<std::string, fs::path> fileNames;
            std::error_code ec;
            for (auto&& p : fs::directory_iterator(folder, ec))
                if (isRegularFile(p, ec) && p.path() != objLocation)
                    fileNames[toLower(p.path().filename().string())] = p;

            for (auto&& name : files) {
                auto fileName = toLower(expandName(name, obj));
                for (auto&& [f, s] : fileNames) {
                    if (f == fileName) {
                        log_debug("{}: found", f);
                        result.push_back(s);
                    }
                }
            }
        }
        if (!patterns.empty()) {
            for (auto&& [dir, ext] : patterns) {
                auto contentPath = fs::path(expandName(dir, obj));
                auto extn = fs::path(expandName(ext, obj));
                auto stem = isCaseSensitive ? extn.stem().string() : toLower(extn.stem().string());
                if (extn.has_extension()) {
                    extn = isCaseSensitive ? extn.extension().string() : toLower(extn.extension().string());
                } else {
                    extn = fmt::format(".{}", isCaseSensitive ? ext : toLower(ext));
                    stem.clear();
                }
                std::error_code ec;
                if (contentPath.is_relative()) {
                    contentPath = fs::weakly_canonical(folder / contentPath);
                }
                if (!fs::is_directory(contentPath)) {
                    log_debug("{}: not a directory", contentPath.string());
                    continue;
                }
                for (auto&& contentFile : fs::directory_iterator(contentPath, ec)) {
                    if (isRegularFile(contentFile, ec) && ((isCaseSensitive && contentFile.path().extension() == extn) || (!isCaseSensitive && toLower(contentFile.path().extension().string()) == extn)) && contentFile.path() != objLocation) {
                        if (!stem.empty()) {
                            replaceAllString(stem, "*", ".*");
                            replaceAllString(stem, ".", "?");
                            auto re = std::regex(fmt::format("^{}$", stem));
                            std::cmatch m;
                            if (std::regex_match(contentFile.path().stem().string().c_str(), m, re)) {
                                log_debug("{}: found", contentFile.path().string());
                                result.push_back(contentFile.path());
                            }
                        } else {
                            log_debug("{}: found", contentFile.path().string());
                            result.push_back(contentFile.path());
                        }
                    }
                }
            }
        }
    }
    if (result.empty()) // required to detect no match
        result.emplace_back();
    return result;
}

static constexpr auto metaTags = std::array {
    std::pair("%album%", M_ALBUM),
    std::pair("%albumArtist%", M_ALBUMARTIST),
    std::pair("%artist%", M_ARTIST),
    std::pair("%genre%", M_GENRE),
    std::pair("%title%", M_TITLE),
    std::pair("%composer%", M_COMPOSER),
};

std::string ContentPathSetup::expandName(std::string_view name, const std::shared_ptr<CdsObject>& obj)
{
    std::string copy(name);

    for (auto&& [key, val] : metaTags)
        replaceString(copy, key, obj->getMetaData(val));

    if (obj->isItem()) {
        fs::path location = obj->getLocation();
        replaceString(copy, "%filename%", location.stem().string());
    }
    if (obj->isContainer()) {
        auto title = obj->getTitle();
        if (!title.empty())
            replaceString(copy, "%filename%", title);
        fs::path location = obj->getLocation();
        replaceString(copy, "%filename%", location.filename().string());
    }
    return copy;
}

std::unique_ptr<ContentPathSetup> FanArtHandler::setup {};

FanArtHandler::FanArtHandler(const std::shared_ptr<Context>& context)
    : MetacontentHandler(context)
{
    if (!setup) {
        setup = std::make_unique<ContentPathSetup>(config, CFG_IMPORT_RESOURCES_FANART_FILE_LIST, CFG_IMPORT_RESOURCES_FANART_DIR_LIST);
    }
}

void FanArtHandler::fillMetadata(const std::shared_ptr<CdsObject>& obj)
{
    log_debug("Running fanart handler on {}", obj->getLocation().c_str());
    auto pathList = setup->getContentPath(obj, SETTING_FANART);

    if (pathList.empty() || pathList[0].empty())
        obj->removeResource(ContentHandler::FANART);

    for (auto&& path : pathList) {
        if (!path.empty()) {
            auto resource = std::make_shared<CdsResource>(ContentHandler::FANART, CdsResource::Purpose::Thumbnail);
            std::string type = path.extension().string().substr(1);
            std::string mimeType = mime->getMimeType(path, fmt::format("image/{}", type));

            resource->addAttribute(CdsResource::Attribute::PROTOCOLINFO, renderProtocolInfo(mimeType));
            resource->addAttribute(CdsResource::Attribute::RESOURCE_FILE, path.string());
            obj->addResource(resource);
        }
    }
}

std::unique_ptr<IOHandler> FanArtHandler::serveContent(const std::shared_ptr<CdsObject>& obj, int resNum)
{
    fs::path path = obj->getResource(resNum)->getAttribute(CdsResource::Attribute::RESOURCE_FILE);
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

void ContainerArtHandler::fillMetadata(const std::shared_ptr<CdsObject>& obj)
{
    auto pathList = setup->getContentPath(obj, SETTING_CONTAINERART, config->getOption(CFG_IMPORT_RESOURCES_CONTAINERART_LOCATION));
    if (pathList.empty() || pathList[0].empty()) {
        pathList = setup->getContentPath(obj, SETTING_CONTAINERART);
    }

    if (pathList.empty() || pathList[0].empty())
        obj->removeResource(ContentHandler::CONTAINERART);

    for (auto&& path : pathList) {
        if (!path.empty()) {
            log_debug("Running ContainerArt handler on {}", !path.empty() ? path.c_str() : obj->getLocation().c_str());
            auto resource = std::make_shared<CdsResource>(ContentHandler::CONTAINERART, CdsResource::Purpose::Thumbnail);
            std::string type = path.extension().string().substr(1);
            std::string mimeType = mime->getMimeType(path, fmt::format("image/{}", type));

            resource->addAttribute(CdsResource::Attribute::PROTOCOLINFO, renderProtocolInfo(mimeType));
            resource->addAttribute(CdsResource::Attribute::RESOURCE_FILE, path.string());
            obj->addResource(resource);
        }
    }
}

std::unique_ptr<IOHandler> ContainerArtHandler::serveContent(const std::shared_ptr<CdsObject>& obj, int resNum)
{
    fs::path path = obj->getResource(resNum)->getAttribute(CdsResource::Attribute::RESOURCE_FILE);
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

void SubtitleHandler::fillMetadata(const std::shared_ptr<CdsObject>& obj)
{
    auto pathList = setup->getContentPath(obj, SETTING_SUBTITLE);
    auto objFilename = obj->getLocation().filename().string() + ".";

    if (pathList.empty() || pathList[0].empty())
        obj->removeResource(ContentHandler::SUBTITLE);

    for (auto&& path : pathList) {
        if (!path.empty()) {
            log_debug("Running subtitle handler on {} -> {}", obj->getLocation().c_str(), path.c_str());
            auto resource = std::make_shared<CdsResource>(ContentHandler::SUBTITLE, CdsResource::Purpose::Subtitle);
            std::string type = path.extension().string().substr(1);

            std::string mimeType = mime->getMimeType(path, fmt::format("text/{}", type));
            auto pos = mimeType.find("plain");
            if (pos != std::string::npos) {
                mimeType = fmt::format("{}{}", mimeType.substr(0, pos), type);
            }

            resource->addAttribute(CdsResource::Attribute::PROTOCOLINFO, renderProtocolInfo(mimeType));
            resource->addAttribute(CdsResource::Attribute::RESOURCE_FILE, path.string());
            resource->addAttribute(CdsResource::Attribute::TYPE, type);
            auto lang = path.stem().string();
            if (startswith(lang, objFilename)) {
                replaceAllString(lang, objFilename, ""); // remove starting part with filename
            }
            if (!lang.empty())
                resource->addAttribute(CdsResource::Attribute::LANGUAGE, lang); // assume file name is related to some language
            resource->addParameter("type", type);
            obj->addResource(resource);
        }
    }
}

std::unique_ptr<IOHandler> SubtitleHandler::serveContent(const std::shared_ptr<CdsObject>& obj, int resNum)
{
    fs::path path = obj->getResource(resNum)->getAttribute(CdsResource::Attribute::RESOURCE_FILE);
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

std::unique_ptr<ContentPathSetup> MetafileHandler::setup {};

MetafileHandler::MetafileHandler(const std::shared_ptr<Context>& context, std::shared_ptr<ContentManager> content)
    : MetacontentHandler(context)
    , content(std::move(content))
{
    if (!setup) {
        setup = std::make_unique<ContentPathSetup>(config, CFG_IMPORT_RESOURCES_METAFILE_FILE_LIST, CFG_IMPORT_RESOURCES_METAFILE_DIR_LIST);
    }
}

void MetafileHandler::fillMetadata(const std::shared_ptr<CdsObject>& obj)
{
#ifdef HAVE_JS
    auto pathList = setup->getContentPath(obj, SETTING_METAFILE);

    if (pathList.empty() || pathList[0].empty())
        obj->removeResource(ContentHandler::METAFILE);

    for (auto&& path : pathList) {
        if (!path.empty()) {
            log_debug("Running metafile handler on {} -> '{}'", obj->getLocation().c_str(), path.c_str());
            content->parseMetafile(obj, path);
        }
    }
#endif
}

std::unique_ptr<IOHandler> MetafileHandler::serveContent(const std::shared_ptr<CdsObject>& obj, int resNum)
{
    return {};
}

std::unique_ptr<ContentPathSetup> ResourceHandler::setup {};

ResourceHandler::ResourceHandler(const std::shared_ptr<Context>& context)
    : MetacontentHandler(context)
{
    if (!setup) {
        setup = std::make_unique<ContentPathSetup>(config, CFG_IMPORT_RESOURCES_RESOURCE_FILE_LIST, CFG_IMPORT_RESOURCES_RESOURCE_DIR_LIST);
    }
}

void ResourceHandler::fillMetadata(const std::shared_ptr<CdsObject>& obj)
{
    auto pathList = setup->getContentPath(obj, SETTING_RESOURCE);

    if (pathList.empty() || pathList[0].empty())
        obj->removeResource(ContentHandler::RESOURCE);

    for (auto&& path : pathList) {
        if (!path.empty() && toLower(path.string()) == toLower(obj->getLocation().string())) {
            log_debug("Running resource handler check on {} -> {}", obj->getLocation().string(), path.string());
            auto resource = std::make_shared<CdsResource>(ContentHandler::RESOURCE, CdsResource::Purpose::Thumbnail);
            resource->addAttribute(CdsResource::Attribute::PROTOCOLINFO, renderProtocolInfo("res"));
            resource->addAttribute(CdsResource::Attribute::RESOURCE_FILE, path.string());
            obj->addResource(resource);
        }
    }
}

std::unique_ptr<IOHandler> ResourceHandler::serveContent(const std::shared_ptr<CdsObject>& obj, int resNum)
{
    fs::path path = obj->getResource(resNum)->getAttribute(CdsResource::Attribute::RESOURCE_FILE);
    if (path.empty()) {
        path = setup->getContentPath(obj, SETTING_RESOURCE)[0];
    }
    log_debug("Resource: Opening name: {}", path.string());
    struct stat statbuf;
    int ret = stat(path.c_str(), &statbuf);
    if (ret != 0) {
        log_warning("File does not exist: {} ({})", path.string(), std::strerror(errno));
        return nullptr;
    }
    return std::make_unique<FileIOHandler>(path);
}
