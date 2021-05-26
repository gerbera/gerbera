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

#include <sys/stat.h>

#include "cds_objects.h"
#include "config/config.h"
#include "config/directory_tweak.h"
#include "iohandler/file_io_handler.h"
#include "util/mime.h"
#include "util/tools.h"

MetacontentHandler::MetacontentHandler(const std::shared_ptr<Context>& context)
    : MetadataHandler(context)
{
}

fs::path MetacontentHandler::getContentPath(const std::vector<std::string>& names, const std::shared_ptr<CdsObject>& obj, bool isCaseSensitive, fs::path folder)
{
    if (!names.empty()) {
        if (folder.empty()) {
            folder = (obj->isContainer()) ? obj->getLocation() : obj->getLocation().parent_path();
        }
        log_debug("Folder name: {}", folder.c_str());

        if (isCaseSensitive) {
            for (auto&& name : names) {
                auto found = folder / expandName(name, obj);
                std::error_code ec;
                bool exists = isRegularFile(found, ec); // no error throwing, please
                if (!exists)
                    continue;

                log_debug("{}: found", found.c_str());
                return found;
            }
        } else {
            auto fileNames = std::map<std::string, fs::path>();
            std::error_code ec;
            for (auto&& p : fs::directory_iterator(folder, ec))
                if (isRegularFile(p, ec))
                    fileNames[toLower(p.path().filename())] = p;

            for (auto&& name : names) {
                auto fileName = toLower(expandName(name, obj));
                for (auto&& [f, s] : fileNames) {
                    if (f == fileName) {
                        log_debug("{}: found", f.c_str());
                        return s;
                    }
                }
            }
        }
    }
    return "";
}

static constexpr std::array<std::pair<std::string_view, metadata_fields_t>, 5> metaTags { {
    { "%album%", M_ALBUM },
    { "%albumArtist%", M_ALBUMARTIST },
    { "%artist%", M_ARTIST },
    { "%genre%", M_GENRE },
    { "%title%", M_TITLE },
} };
bool MetacontentHandler::caseSensitive = true;

std::string MetacontentHandler::expandName(const std::string& name, const std::shared_ptr<CdsObject>& obj)
{
    std::string copy(name);

    for (auto&& [key, val] : metaTags)
        replaceString(copy, key, obj->getMetadata(val));

    if (obj->isItem()) {
        fs::path location = obj->getLocation();
        replaceString(copy, "%filename%", location.stem());
    }
    if (obj->isContainer()) {
        auto title = obj->getTitle();
        if (!title.empty())
            replaceString(copy, "%filename%", title);
        fs::path location = obj->getLocation();
        replaceString(copy, "%filename%", location.filename());
    }
    return copy;
}

std::vector<std::string> FanArtHandler::names = {
    //    "%title%.jpg",
    //    "%filename%.jpg",
    //    "folder.jpg",
    //    "poster.jpg",
    //    "cover.jpg",
    //    "albumartsmall.jpg",
    //    "%album%.jpg",
};
bool FanArtHandler::initDone = false;

FanArtHandler::FanArtHandler(const std::shared_ptr<Context>& context)
    : MetacontentHandler(context)
{
    if (!initDone) {
        std::vector<std::string> files = this->config->getArrayOption(CFG_IMPORT_RESOURCES_FANART_FILE_LIST);
        names.insert(names.end(), files.begin(), files.end());
        caseSensitive = this->config->getBoolOption(CFG_IMPORT_RESOURCES_CASE_SENSITIVE);
        initDone = true;
    }
}

void FanArtHandler::fillMetadata(std::shared_ptr<CdsObject> obj)
{
    log_debug("Running fanart handler on {}", obj->getLocation().c_str());
    auto tweak = config->getDirectoryTweakOption(CFG_IMPORT_DIRECTORIES_LIST)->get(obj->getLocation());
    auto path = getContentPath(tweak == nullptr || !tweak->hasFanArtFile() ? names : std::vector<std::string> { tweak->getFanArtFile() }, obj, tweak != nullptr && tweak->hasCaseSensitive() ? tweak->getCaseSensitive() : caseSensitive);

    if (!path.empty()) {
        auto resource = std::make_shared<CdsResource>(CH_FANART);
        std::string type = path.extension().string().substr(1);
        std::string mimeType = mime->getMimeType(path, fmt::format("image/{}", type));

        resource->addAttribute(R_PROTOCOLINFO, renderProtocolInfo(mimeType));
        resource->addAttribute(R_RESOURCE_FILE, path.string());
        resource->addParameter(RESOURCE_CONTENT_TYPE, ID3_ALBUM_ART);
        obj->addResource(resource);
    } else {
        obj->removeResource(CH_FANART);
    }
}

std::unique_ptr<IOHandler> FanArtHandler::serveContent(std::shared_ptr<CdsObject> obj, int resNum)
{
    fs::path path = obj->getResource(resNum)->getAttribute(R_RESOURCE_FILE);
    if (path.empty()) {
        auto tweak = config->getDirectoryTweakOption(CFG_IMPORT_DIRECTORIES_LIST)->get(obj->getLocation());
        path = getContentPath(tweak == nullptr || !tweak->hasFanArtFile() ? names : std::vector<std::string> { tweak->getFanArtFile() }, obj, tweak != nullptr && tweak->hasCaseSensitive() ? tweak->getCaseSensitive() : caseSensitive);
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

std::vector<std::string> ContainerArtHandler::names = {
    //    "%title%.jpg",
    //    "%filename%.jpg",
    //    "folder.jpg",
    //    "poster.jpg",
    //    "cover.jpg",
    //    "albumartsmall.jpg",
    //    "%album%.jpg",
};
bool ContainerArtHandler::initDone = false;

ContainerArtHandler::ContainerArtHandler(const std::shared_ptr<Context>& context)
    : MetacontentHandler(context)
{
    if (!initDone) {
        std::vector<std::string> files = this->config->getArrayOption(CFG_IMPORT_RESOURCES_CONTAINERART_FILE_LIST);
        names.insert(names.end(), files.begin(), files.end());
        caseSensitive = this->config->getBoolOption(CFG_IMPORT_RESOURCES_CASE_SENSITIVE);
        initDone = true;
    }
}

void ContainerArtHandler::fillMetadata(std::shared_ptr<CdsObject> obj)
{
    auto path = getContentPath(names, obj, caseSensitive, config->getOption(CFG_IMPORT_RESOURCES_CONTAINERART_LOCATION));
    if (path.empty()) {
        path = getContentPath(names, obj, caseSensitive);
    }
    log_debug("Running ContainerArt handler on {}", !path.empty() ? path.c_str() : obj->getLocation().c_str());

    if (!path.empty()) {
        auto resource = std::make_shared<CdsResource>(CH_CONTAINERART);
        std::string type = path.extension().string().substr(1);
        std::string mimeType = mime->getMimeType(path, fmt::format("image/{}", type));

        resource->addAttribute(R_PROTOCOLINFO, renderProtocolInfo(mimeType));
        resource->addAttribute(R_RESOURCE_FILE, path.string());
        resource->addParameter(RESOURCE_CONTENT_TYPE, ID3_ALBUM_ART);
        obj->addResource(resource);
    } else {
        obj->removeResource(CH_CONTAINERART);
    }
}

std::unique_ptr<IOHandler> ContainerArtHandler::serveContent(std::shared_ptr<CdsObject> obj, int resNum)
{
    fs::path path = obj->getResource(resNum)->getAttribute(R_RESOURCE_FILE);
    if (path.empty()) {
        path = getContentPath(names, obj, caseSensitive, config->getOption(CFG_IMPORT_RESOURCES_CONTAINERART_LOCATION));
        if (path.empty()) {
            path = getContentPath(names, obj, caseSensitive);
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

std::vector<std::string> SubtitleHandler::names = {
    //    "%title%.srt",
    //    "%filename%.srt"
};
bool SubtitleHandler::initDone = false;

SubtitleHandler::SubtitleHandler(const std::shared_ptr<Context>& context)
    : MetacontentHandler(context)
{
    if (!initDone) {
        std::vector<std::string> files = this->config->getArrayOption(CFG_IMPORT_RESOURCES_SUBTITLE_FILE_LIST);
        names.insert(names.end(), files.begin(), files.end());
        initDone = true;
    }
}

void SubtitleHandler::fillMetadata(std::shared_ptr<CdsObject> obj)
{
    auto tweak = config->getDirectoryTweakOption(CFG_IMPORT_DIRECTORIES_LIST)->get(obj->getLocation());
    auto path = getContentPath(tweak == nullptr || !tweak->hasSubTitleFile() ? names : std::vector<std::string> { tweak->getSubTitleFile() }, obj, tweak != nullptr && tweak->hasCaseSensitive() ? tweak->getCaseSensitive() : caseSensitive);
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
        resource->addParameter(RESOURCE_CONTENT_TYPE, VIDEO_SUB);
        resource->addParameter("type", type);
        obj->addResource(resource);
    } else {
        obj->removeResource(CH_SUBTITLE);
    }
}

std::unique_ptr<IOHandler> SubtitleHandler::serveContent(std::shared_ptr<CdsObject> obj, int resNum)
{
    fs::path path = obj->getResource(resNum)->getAttribute(R_RESOURCE_FILE);
    if (path.empty()) {
        auto tweak = config->getDirectoryTweakOption(CFG_IMPORT_DIRECTORIES_LIST)->get(obj->getLocation());
        path = getContentPath(tweak == nullptr || !tweak->hasSubTitleFile() ? names : std::vector<std::string> { tweak->getSubTitleFile() }, obj, tweak != nullptr && tweak->hasCaseSensitive() ? tweak->getCaseSensitive() : caseSensitive);
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

std::vector<std::string> ResourceHandler::names = {
    //    "%filename%.srt",
    //    "cover.jpg",
    //    "%album%.jpg",
};
bool ResourceHandler::initDone = false;

ResourceHandler::ResourceHandler(const std::shared_ptr<Context>& context)
    : MetacontentHandler(context)
{
    if (!initDone) {
        std::vector<std::string> files = this->config->getArrayOption(CFG_IMPORT_RESOURCES_RESOURCE_FILE_LIST);
        names.insert(names.end(), files.begin(), files.end());
        initDone = true;
    }
}

void ResourceHandler::fillMetadata(std::shared_ptr<CdsObject> obj)
{
    auto tweak = config->getDirectoryTweakOption(CFG_IMPORT_DIRECTORIES_LIST)->get(obj->getLocation());
    auto path = getContentPath(tweak == nullptr || !tweak->hasResourceFile() ? names : std::vector<std::string> { tweak->getResourceFile() }, obj, tweak != nullptr && tweak->hasCaseSensitive() ? tweak->getCaseSensitive() : caseSensitive);
    log_debug("Running resource handler check on {} -> {}", obj->getLocation().string().c_str(), path.string().c_str());

    if (!path.empty() && toLower(path.string()) == toLower(obj->getLocation().string())) {
        auto resource = std::make_shared<CdsResource>(CH_RESOURCE);
        resource->addAttribute(R_PROTOCOLINFO, renderProtocolInfo("res"));
        resource->addAttribute(R_RESOURCE_FILE, path.string());
        obj->addResource(resource);
    } else {
        obj->removeResource(CH_RESOURCE);
    }
}

std::unique_ptr<IOHandler> ResourceHandler::serveContent(std::shared_ptr<CdsObject> obj, int resNum)
{
    fs::path path = obj->getResource(resNum)->getAttribute(R_RESOURCE_FILE);
    if (path.empty()) {
        auto tweak = config->getDirectoryTweakOption(CFG_IMPORT_DIRECTORIES_LIST)->get(obj->getLocation());
        path = getContentPath(tweak == nullptr || !tweak->hasResourceFile() ? names : std::vector<std::string> { tweak->getResourceFile() }, obj, tweak != nullptr && tweak->hasCaseSensitive() ? tweak->getCaseSensitive() : caseSensitive);
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
