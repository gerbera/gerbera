/*GRB*

    Gerbera - https://gerbera.io/

    metacontent_handler.cc - this file is part of Gerbera.

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

/// \file metacontent_handler.cc
#define GRB_LOG_FAC GrbLogFacility::metadata

#include "metacontent_handler.h" // API

#include "cds/cds_objects.h"
#include "config/config.h"
#include "config/config_definition.h"
#include "config/config_val.h"
#include "config/result/directory_tweak.h"
#include "content/content.h"
#include "context.h"
#include "iohandler/file_io_handler.h"
#include "util/mime.h"
#include "util/string_converter.h"
#include "util/tools.h"

#include <array>
#include <regex>
#include <sys/stat.h>

ContentPathSetup::ContentPathSetup(std::shared_ptr<Config> config, const std::shared_ptr<ConfigDefinition>& definition, ConfigVal fileListOption, ConfigVal dirListOption)
    : config(std::move(config))
    , names(this->config->getArrayOption(fileListOption))
    , patterns(this->config->getVectorOption(dirListOption))
    , allTweaks(this->config->getDirectoryTweakOption(ConfigVal::IMPORT_DIRECTORIES_LIST))
    , caseSensitive(this->config->getBoolOption(ConfigVal::IMPORT_RESOURCES_CASE_SENSITIVE))
    , definition(definition)
{
}

std::vector<fs::path> ContentPathSetup::getContentPath(const std::shared_ptr<CdsObject>& obj, const std::string& setting, fs::path folder) const
{
    auto objLocation = obj->getLocation();
    auto tweak = allTweaks->getKey(objLocation);
    auto files = !tweak || !tweak->hasSetting(setting) ? this->names : std::vector<std::string> { tweak->getSetting(setting) };
    auto isCaseSensitive = tweak && tweak->hasCaseSensitive() ? tweak->getCaseSensitive() : this->caseSensitive;

    static auto nameOption = definition->removeAttribute(ConfigVal::A_IMPORT_RESOURCES_NAME);
    static auto extOption = definition->removeAttribute(ConfigVal::A_IMPORT_RESOURCES_EXT);
    static auto pttOption = definition->removeAttribute(ConfigVal::A_IMPORT_RESOURCES_PTT);

    std::vector<fs::path> result;

    if (!files.empty()) {
        if (folder.empty()) {
            folder = (obj->isContainer()) ? objLocation : objLocation.parent_path();
        }
        log_debug("Folder name: {}", folder.c_str());

        if (isCaseSensitive) {
            // directly search files using name
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
            // get all files in lowercase
            for (auto&& p : fs::directory_iterator(folder, ec))
                if (isRegularFile(p, ec) && p.path() != objLocation)
                    fileNames[toLower(p.path().filename().string())] = p;

            // filter files matching filenames
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
            // filter files matching patterns
            for (auto&& pattern : patterns) {
                std::string dir;
                std::string ext;
                std::string ptt;
                for (auto&& [key, val] : pattern) {
                    if (key == nameOption)
                        dir = val;
                    else if (key == extOption)
                        ext = val;
                    else if (key == pttOption)
                        ptt = val;
                }
                auto contentPath = fs::path(expandName(dir, obj));
                auto extn = fs::path(expandName(ext, obj));
                ptt = expandName(ptt, obj);
                auto stem = isCaseSensitive ? extn.stem().string() : toLower(extn.stem().string());
                if (extn.has_extension()) {
                    extn = isCaseSensitive ? extn.extension().string() : toLower(extn.extension().string());
                } else {
                    extn = fmt::format(".{}", isCaseSensitive ? ext : toLower(ext));
                    stem.clear();
                }
                if (!ptt.empty()) {
                    stem = fmt::format("{}{}", ptt, stem);
                }
                std::error_code ec;
                if (contentPath.is_relative()) {
                    contentPath = fs::weakly_canonical(folder / contentPath);
                }
                if (!fs::is_directory(contentPath)) {
                    log_debug("{}: not a directory", contentPath.string());
                    continue;
                }
                // Check files using patterns
                for (auto&& contentFile : fs::directory_iterator(contentPath, ec)) {
                    if (isRegularFile(contentFile, ec)
                        && (ext.empty() || (isCaseSensitive && contentFile.path().extension() == extn) || (!isCaseSensitive && toLower(contentFile.path().extension().string()) == extn))
                        && contentFile.path() != objLocation) {
                        if (!stem.empty()) {
                            replaceAllString(stem, "?", ".");
                            replaceAllString(stem, "*", ".*");
                            auto re = std::regex(fmt::format("^{}$", stem));
                            if (std::regex_match(contentFile.path().stem().string(), re)) {
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

static constexpr std::array metaTags {
    std::pair("%album%", MetadataFields::M_ALBUM),
    std::pair("%albumArtist%", MetadataFields::M_ALBUMARTIST),
    std::pair("%artist%", MetadataFields::M_ARTIST),
    std::pair("%genre%", MetadataFields::M_GENRE),
    std::pair("%title%", MetadataFields::M_TITLE),
    std::pair("%composer%", MetadataFields::M_COMPOSER),
};

std::string ContentPathSetup::expandName(const std::string& name, const std::shared_ptr<CdsObject>& obj)
{
    std::string copy(name);
    auto path = (obj->isItem()) ? obj->getLocation().parent_path().string() : obj->getLocation().string();
    if (name.empty())
        return path;
    else if (name.at(0) == '.') {
        return copy.replace(0, 1, path);
    }

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

MetacontentHandler::MetacontentHandler(const std::shared_ptr<Context>& context)
    : MetadataHandler(context)
    , f2i(context->getConverterManager()->f2i())
    , definition(context->getDefinition())
{
}

FanArtHandler::FanArtHandler(const std::shared_ptr<Context>& context)
    : MetacontentHandler(context)
{
    if (!setup) {
        setup = std::make_unique<ContentPathSetup>(config, definition, ConfigVal::IMPORT_RESOURCES_FANART_FILE_LIST, ConfigVal::IMPORT_RESOURCES_FANART_DIR_LIST);
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
            auto resource = std::make_shared<CdsResource>(ContentHandler::FANART, ResourcePurpose::Thumbnail);
            std::string type = path.extension().string().substr(1);
            auto mimeType = std::get<1>(mime->getMimeType(path, fmt::format("image/{}", type)));

            if (!mimeType.empty()) {
                resource->addAttribute(ResourceAttribute::PROTOCOLINFO, renderProtocolInfo(mimeType));
                resource->addAttribute(ResourceAttribute::RESOURCE_FILE, path.string());
            }
            obj->addResource(resource);
        }
    }
}

std::unique_ptr<IOHandler> FanArtHandler::serveContent(const std::shared_ptr<CdsObject>& obj, const std::shared_ptr<CdsResource>& resource)
{
    fs::path path = resource->getAttribute(ResourceAttribute::RESOURCE_FILE);
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
        setup = std::make_unique<ContentPathSetup>(config, definition, ConfigVal::IMPORT_RESOURCES_CONTAINERART_FILE_LIST, ConfigVal::IMPORT_RESOURCES_CONTAINERART_DIR_LIST);
    }
}

void ContainerArtHandler::fillMetadata(const std::shared_ptr<CdsObject>& obj)
{
    auto pathList = setup->getContentPath(obj, SETTING_CONTAINERART, config->getOption(ConfigVal::IMPORT_RESOURCES_CONTAINERART_LOCATION));
    if (pathList.empty() || pathList[0].empty()) {
        pathList = setup->getContentPath(obj, SETTING_CONTAINERART);
    }

    if (pathList.empty() || pathList[0].empty())
        obj->removeResource(ContentHandler::CONTAINERART);

    for (auto&& path : pathList) {
        if (!path.empty()) {
            log_debug("Running ContainerArt handler on {}", !path.empty() ? path.c_str() : obj->getLocation().c_str());
            auto resource = std::make_shared<CdsResource>(ContentHandler::CONTAINERART, ResourcePurpose::Thumbnail);
            std::string type = path.extension().string().substr(1);
            auto mimeType = std::get<1>(mime->getMimeType(path, fmt::format("image/{}", type)));
            if (!mimeType.empty()) {
                resource->addAttribute(ResourceAttribute::PROTOCOLINFO, renderProtocolInfo(mimeType));
                auto [val, err] = f2i->convert(path.string());
                if (!err.empty()) {
                    log_warning("{}: {}", path.string(), err);
                }
                resource->addAttribute(ResourceAttribute::RESOURCE_FILE, val);
            }
            obj->addResource(resource);
        }
    }
}

std::unique_ptr<IOHandler> ContainerArtHandler::serveContent(const std::shared_ptr<CdsObject>& obj, const std::shared_ptr<CdsResource>& resource)
{
    fs::path path = resource->getAttribute(ResourceAttribute::RESOURCE_FILE);
    if (path.empty()) {
        path = setup->getContentPath(obj, SETTING_CONTAINERART, config->getOption(ConfigVal::IMPORT_RESOURCES_CONTAINERART_LOCATION))[0];
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
        setup = std::make_unique<ContentPathSetup>(config, definition, ConfigVal::IMPORT_RESOURCES_SUBTITLE_FILE_LIST, ConfigVal::IMPORT_RESOURCES_SUBTITLE_DIR_LIST);
    }
}

void SubtitleHandler::fillMetadata(const std::shared_ptr<CdsObject>& obj)
{
    auto pathList = setup->getContentPath(obj, SETTING_SUBTITLE);
    auto objFilename = obj->getLocation().filename().stem().string();

    if (pathList.empty() || pathList[0].empty())
        obj->removeResource(ContentHandler::SUBTITLE);

    for (auto&& path : pathList) {
        if (!path.empty()) {
            log_debug("Running subtitle handler on {} -> {}", obj->getLocation().c_str(), path.c_str());
            auto resource = std::make_shared<CdsResource>(ContentHandler::SUBTITLE, ResourcePurpose::Subtitle);
            std::string type = path.extension().string().substr(1);

            auto mimeType = std::get<1>(mime->getMimeType(path, fmt::format("text/{}", type)));
            auto pos = mimeType.find("plain");
            if (pos != std::string::npos) {
                mimeType = fmt::format("{}{}", mimeType.substr(0, pos), type);
            }

            resource->addAttribute(ResourceAttribute::PROTOCOLINFO, renderProtocolInfo(mimeType));
            auto [mval, err] = f2i->convert(path.string());
            if (!err.empty()) {
                log_warning("{}: {}", path.string(), err);
            }
            resource->addAttribute(ResourceAttribute::RESOURCE_FILE, mval);
            resource->addAttribute(ResourceAttribute::TYPE, type);
            auto lang = path.stem().string();
            if (startswith(lang, objFilename)) {
                replaceAllString(lang, objFilename, ""); // remove starting part with filename
            }
            while (!lang.empty() && lang[0] == '.') {
                lang = lang.substr(1); // remove starting dot
            }
            if (!lang.empty())
                resource->addAttribute(ResourceAttribute::LANGUAGE, lang); // assume file name is related to some language
            resource->addParameter("type", type);
            obj->addResource(resource);
        }
    }
}

std::unique_ptr<IOHandler> SubtitleHandler::serveContent(const std::shared_ptr<CdsObject>& obj, const std::shared_ptr<CdsResource>& resource)
{
    fs::path path = resource->getAttribute(ResourceAttribute::RESOURCE_FILE);
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

MetafileHandler::MetafileHandler(const std::shared_ptr<Context>& context, std::shared_ptr<Content> content)
    : MetacontentHandler(context)
    , content(std::move(content))
{
    if (!setup) {
        setup = std::make_unique<ContentPathSetup>(config, definition, ConfigVal::IMPORT_RESOURCES_METAFILE_FILE_LIST, ConfigVal::IMPORT_RESOURCES_METAFILE_DIR_LIST);
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

std::unique_ptr<IOHandler> MetafileHandler::serveContent(const std::shared_ptr<CdsObject>& obj, const std::shared_ptr<CdsResource>& resource)
{
    return {};
}

std::unique_ptr<ContentPathSetup> ResourceHandler::setup {};

ResourceHandler::ResourceHandler(const std::shared_ptr<Context>& context)
    : MetacontentHandler(context)
{
    if (!setup) {
        setup = std::make_unique<ContentPathSetup>(config, definition, ConfigVal::IMPORT_RESOURCES_RESOURCE_FILE_LIST, ConfigVal::IMPORT_RESOURCES_RESOURCE_DIR_LIST);
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
            auto resource = std::make_shared<CdsResource>(ContentHandler::RESOURCE, ResourcePurpose::Thumbnail);
            resource->addAttribute(ResourceAttribute::PROTOCOLINFO, renderProtocolInfo("res"));
            auto [mval, err] = f2i->convert(path.string());
            if (!err.empty()) {
                log_warning("{}: {}", path.string(), err);
            }
            resource->addAttribute(ResourceAttribute::RESOURCE_FILE, mval);
            obj->addResource(resource);
        }
    }
}

std::unique_ptr<IOHandler> ResourceHandler::serveContent(const std::shared_ptr<CdsObject>& obj, const std::shared_ptr<CdsResource>& resource)
{
    fs::path path = resource->getAttribute(ResourceAttribute::RESOURCE_FILE);
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
