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
#include <utility>

#include "cds_objects.h"
#include "config/config.h"
#include "config/directory_tweak.h"
#include "iohandler/file_io_handler.h"
#include "util/tools.h"

MetacontentHandler::MetacontentHandler(std::shared_ptr<Config> config, std::shared_ptr<Mime> mime)
    : MetadataHandler(std::move(config), std::move(mime))
{
}

fs::path MetacontentHandler::getContentPath(const std::vector<std::string>& names, const std::shared_ptr<CdsItem>& item, bool isCaseSensitive)
{
    if (!names.empty()) {
        auto folder = item->getLocation().parent_path();
        log_debug("Folder name: {}", folder.c_str());

        if (isCaseSensitive) {
            for (const auto& name : names) {
                auto found = folder / expandName(name, item);
                std::error_code ec;
                bool exists = isRegularFile(found, ec); // no error throwing, please
                if (!exists)
                    continue;

                log_debug("{}: found", found.c_str());
                return found;
            }
        } else {
            auto fileNames = std::map<std::string, fs::path>();
            for (const auto& p : fs::directory_iterator(folder))
                if (p.is_regular_file())
                    fileNames[toLower(p.path().filename())] = p;

            for (const auto& name : names) {
                auto it = std::find_if(fileNames.begin(), fileNames.end(), [fileName = toLower(expandName(name, item))](const auto& testFile) { return testFile.first == fileName; });
                if (it != fileNames.end()) {
                    log_debug("{}: found", it->first.c_str());
                    return it->second;
                }
            }
        }
    }
    return "";
}

static constexpr std::array<std::pair<std::string_view, metadata_fields_t>, 2> metaTags { {
    { "%album%", M_ALBUM },
    { "%title%", M_TITLE },
} };
bool MetacontentHandler::caseSensitive = true;

std::string MetacontentHandler::expandName(const std::string& name, const std::shared_ptr<CdsItem>& item)
{
    std::string copy(name);

    for (const auto& [key, val] : metaTags)
        replaceString(copy, key, item->getMetadata(val));

    fs::path location = item->getLocation();
    replaceString(copy, "%filename%", location.stem());
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

FanArtHandler::FanArtHandler(std::shared_ptr<Config> config, std::shared_ptr<Mime> mime)
    : MetacontentHandler(std::move(config), std::move(mime))
{
    if (!initDone) {
        std::vector<std::string> files = this->config->getArrayOption(CFG_IMPORT_RESOURCES_FANART_FILE_LIST);
        names.insert(names.end(), files.begin(), files.end());
        caseSensitive = this->config->getBoolOption(CFG_IMPORT_RESOURCES_CASE_SENSITIVE);
        initDone = true;
    }
}

void FanArtHandler::fillMetadata(std::shared_ptr<CdsItem> item)
{
    log_debug("Running fanart handler on {}", item->getLocation().c_str());
    auto tweak = config->getDirectoryTweakOption(CFG_IMPORT_DIRECTORIES_LIST)->get(item->getLocation());
    auto path = getContentPath(tweak == nullptr || !tweak->hasFanArtFile() ? names : std::vector<std::string> { tweak->getFanArtFile() }, item, tweak != nullptr && tweak->hasCaseSensitive() ? tweak->getCaseSensitive() : caseSensitive);

    if (!path.empty()) {
        auto resource = std::make_shared<CdsResource>(CH_FANART);
        resource->addAttribute(R_PROTOCOLINFO, renderProtocolInfo("jpg"));
        resource->addAttribute(R_RESOURCE_FILE, path.c_str());
        resource->addParameter(RESOURCE_CONTENT_TYPE, ID3_ALBUM_ART);
        item->addResource(resource);
    } else {
        item->removeResource(CH_FANART);
    }
}

std::unique_ptr<IOHandler> FanArtHandler::serveContent(std::shared_ptr<CdsItem> item, int resNum)
{
    fs::path path = item->getResource(resNum)->getAttribute(R_RESOURCE_FILE);
    if (path.empty()) {
        auto tweak = config->getDirectoryTweakOption(CFG_IMPORT_DIRECTORIES_LIST)->get(item->getLocation());
        path = getContentPath(tweak == nullptr && !tweak->hasFanArtFile() ? names : std::vector<std::string> { tweak->getFanArtFile() }, item, tweak != nullptr && tweak->hasCaseSensitive() ? tweak->getCaseSensitive() : caseSensitive);
    }
    log_debug("FanArt: Opening name: {}", path.c_str());
    struct stat statbuf;
    int ret = stat(path.c_str(), &statbuf);
    if (ret != 0) {
        log_warning("File does not exist: {} ({})", path.c_str(), strerror(errno));
        return nullptr;
    }
    auto io_handler = std::make_unique<FileIOHandler>(path);
    return io_handler;
}

std::vector<std::string> SubtitleHandler::names = {
    //    "%title%.srt",
    //    "%filename%.srt"
};
bool SubtitleHandler::initDone = false;

SubtitleHandler::SubtitleHandler(std::shared_ptr<Config> config, std::shared_ptr<Mime> mime)
    : MetacontentHandler(std::move(config), std::move(mime))
{
    if (!initDone) {
        std::vector<std::string> files = this->config->getArrayOption(CFG_IMPORT_RESOURCES_SUBTITLE_FILE_LIST);
        names.insert(names.end(), files.begin(), files.end());
        initDone = true;
    }
}

void SubtitleHandler::fillMetadata(std::shared_ptr<CdsItem> item)
{
    auto tweak = config->getDirectoryTweakOption(CFG_IMPORT_DIRECTORIES_LIST)->get(item->getLocation());
    auto path = getContentPath(tweak == nullptr || !tweak->hasSubTitleFile() ? names : std::vector<std::string> { tweak->getSubTitleFile() }, item, tweak != nullptr && tweak->hasCaseSensitive() ? tweak->getCaseSensitive() : caseSensitive);
    log_debug("Running subtitle handler on {} -> {}", item->getLocation().c_str(), path.c_str());

    if (!path.empty()) {
        auto resource = std::make_shared<CdsResource>(CH_SUBTITLE);
        std::string type = path.extension().string().substr(1);
        resource->addAttribute(R_PROTOCOLINFO, renderProtocolInfo(type));
        resource->addAttribute(R_RESOURCE_FILE, path.c_str());
        resource->addParameter(RESOURCE_CONTENT_TYPE, VIDEO_SUB);
        resource->addParameter("type", type);
        item->addResource(resource);
    } else {
        item->removeResource(CH_SUBTITLE);
    }
}

std::unique_ptr<IOHandler> SubtitleHandler::serveContent(std::shared_ptr<CdsItem> item, int resNum)
{
    fs::path path = item->getResource(resNum)->getAttribute(R_RESOURCE_FILE);
    if (path.empty()) {
        auto tweak = config->getDirectoryTweakOption(CFG_IMPORT_DIRECTORIES_LIST)->get(item->getLocation());
        path = getContentPath(tweak == nullptr || !tweak->hasSubTitleFile() ? names : std::vector<std::string> { tweak->getSubTitleFile() }, item, tweak != nullptr && tweak->hasCaseSensitive() ? tweak->getCaseSensitive() : caseSensitive);
    }
    log_debug("Subtitle: Opening name: {}", path.c_str());
    struct stat statbuf;
    int ret = stat(path.c_str(), &statbuf);
    if (ret != 0) {
        log_warning("File does not exist: {} ({})", path.c_str(), strerror(errno));
        return nullptr;
    }
    auto io_handler = std::make_unique<FileIOHandler>(path);
    return io_handler;
}

std::vector<std::string> ResourceHandler::names = {
    //    "%filename%.srt",
    //    "cover.jpg",
    //    "%album%.jpg",
};
bool ResourceHandler::initDone = false;

ResourceHandler::ResourceHandler(std::shared_ptr<Config> config, std::shared_ptr<Mime> mime)
    : MetacontentHandler(std::move(config), std::move(mime))
{
    if (!initDone) {
        std::vector<std::string> files = this->config->getArrayOption(CFG_IMPORT_RESOURCES_RESOURCE_FILE_LIST);
        names.insert(names.end(), files.begin(), files.end());
        initDone = true;
    }
}

void ResourceHandler::fillMetadata(std::shared_ptr<CdsItem> item)
{
    auto tweak = config->getDirectoryTweakOption(CFG_IMPORT_DIRECTORIES_LIST)->get(item->getLocation());
    auto path = getContentPath(tweak == nullptr || !tweak->hasResourceFile() ? names : std::vector<std::string> { tweak->getResourceFile() }, item, tweak != nullptr && tweak->hasCaseSensitive() ? tweak->getCaseSensitive() : caseSensitive);
    log_debug("Running resource handler check on {} -> {}", item->getLocation().c_str(), path.c_str());

    if (!path.empty() && toLower(path.c_str()) == toLower(item->getLocation().c_str())) {
        auto resource = std::make_shared<CdsResource>(CH_RESOURCE);
        resource->addAttribute(R_PROTOCOLINFO, renderProtocolInfo("res"));
        resource->addAttribute(R_RESOURCE_FILE, path.c_str());
        item->addResource(resource);
    } else {
        item->removeResource(CH_RESOURCE);
    }
}

std::unique_ptr<IOHandler> ResourceHandler::serveContent(std::shared_ptr<CdsItem> item, int resNum)
{
    fs::path path = item->getResource(resNum)->getAttribute(R_RESOURCE_FILE);
    if (path.empty()) {
        auto tweak = config->getDirectoryTweakOption(CFG_IMPORT_DIRECTORIES_LIST)->get(item->getLocation());
        path = getContentPath(tweak == nullptr || !tweak->hasResourceFile() ? names : std::vector<std::string> { tweak->getResourceFile() }, item, tweak != nullptr && tweak->hasCaseSensitive() ? tweak->getCaseSensitive() : caseSensitive);
    }
    log_debug("Resource: Opening name: {}", path.c_str());
    struct stat statbuf;
    int ret = stat(path.c_str(), &statbuf);
    if (ret != 0) {
        log_warning("File does not exist: {} ({})", path.c_str(), strerror(errno));
        return nullptr;
    }
    auto io_handler = std::make_unique<FileIOHandler>(path);
    return io_handler;
}
