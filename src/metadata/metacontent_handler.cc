/*MT*

    MediaTomb - http://www.mediatomb.cc/

    metacontent_handler.cc - this file is part of MediaTomb.

    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>

    Copyright (C) 2006-2010 Gena Batyan <bgeradz@mediatomb.cc>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>,
                            Leonhard Wimmer <leo@mediatomb.cc>

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

/// \file metacontent_handler.cc

#include "metacontent_handler.h" // API

#include <sys/stat.h>
#include <utility>

#include "cds_objects.h"
#include "config/config.h"
#include "iohandler/file_io_handler.h"
#include "util/tools.h"

// These must not have a leading slash, or the "/" operator will produce
// just this, not folder and this
std::vector<std::string> FanArtHandler::names = {
    "%title%.jpg",
    "%filename%.jpg",
    "folder.jpg",
    "poster.jpg",
    "cover.jpg",
    "albumartsmall.jpg",
    "%album%.jpg",
};

std::vector<std::string> SubtitleHandler::names = {
    "%title%.srt",
    "%filename%.srt"
};

std::vector<std::string> ResourceHandler::names = {
    "%filename%.srt",
    "folder.jpg",
    "poster.jpg",
    "cover.jpg",
    "albumartsmall.jpg",
    "%album%.jpg",
};

MetacontentHandler::MetacontentHandler(std::shared_ptr<Config> config)
    : MetadataHandler(std::move(config))
{
}

fs::path MetacontentHandler::getContentPath(std::vector<std::string>& names, const std::shared_ptr<CdsItem>& item)
{
    auto folder = item->getLocation().parent_path();
    log_debug("Folder name: {}", folder.c_str());

    for (const auto& name : names) {
        auto fileName = tolower_string(expandName(name, item));
        for (auto& p : fs::directory_iterator(folder))
            if (p.is_regular_file() && tolower_string(p.path().filename()) == fileName) {
                log_debug("{}: found", p.path().filename().c_str());
                return p.path();
            }
    }
    return "";
}

static std::map<std::string, int> metaTags = {
    { "%album%", M_ALBUM },
    { "%title%", M_TITLE },
};

std::string MetacontentHandler::expandName(const std::string& name, const std::shared_ptr<CdsItem>& item)
{
    std::string copy(name);

    for (const auto& tag : metaTags)
        replace_string(copy, tag.first, item->getMetadata(MT_KEYS[tag.second].upnp));

    fs::path location = item->getLocation();
    replace_string(copy, "%filename%", location.stem());
    return copy;
}

FanArtHandler::FanArtHandler(std::shared_ptr<Config> config)
    : MetacontentHandler(std::move(config))
{
    std::vector<std::string> files = this->config->getArrayOption(CFG_IMPORT_RESOURCES_FANART_FILE_LIST);
    names.insert(names.end(), files.begin(), files.end());
}

void FanArtHandler::fillMetadata(std::shared_ptr<CdsItem> item)
{
    log_debug("Running fanart handler on {}", item->getLocation().c_str());
    auto path = getContentPath(names, item);

    if (!path.empty()) {
        auto resource = std::make_shared<CdsResource>(CH_FANART);
        resource->addAttribute(MetadataHandler::getResAttrName(R_PROTOCOLINFO), renderProtocolInfo("jpg"));
        resource->addParameter(RESOURCE_CONTENT_TYPE, ID3_ALBUM_ART);
        item->addResource(resource);
    }
}

std::unique_ptr<IOHandler> FanArtHandler::serveContent(std::shared_ptr<CdsItem> item, int resNum)
{
    auto path = getContentPath(names, item);
    log_debug("FanArt: Opening name: {}", path.c_str());

    auto io_handler = std::make_unique<FileIOHandler>(path);
    return io_handler;
}

SubtitleHandler::SubtitleHandler(std::shared_ptr<Config> config)
    : MetacontentHandler(std::move(config))
{
    std::vector<std::string> files = this->config->getArrayOption(CFG_IMPORT_RESOURCES_SUBTITLE_FILE_LIST);
    names.insert(names.end(), files.begin(), files.end());
}

void SubtitleHandler::fillMetadata(std::shared_ptr<CdsItem> item)
{
    auto path = getContentPath(names, item);
    log_debug("Running subtitle handler on {} -> {}", item->getLocation().c_str(), path.c_str());

    if (!path.empty()) {
        auto resource = std::make_shared<CdsResource>(CH_SUBTITLE);
        std::string type = path.extension().string().substr(1);
        resource->addAttribute(MetadataHandler::getResAttrName(R_PROTOCOLINFO), renderProtocolInfo(type));
        resource->addParameter(RESOURCE_CONTENT_TYPE, VIDEO_SUB);
        resource->addParameter("type", type.c_str());
        item->addResource(resource);
    }
}

std::unique_ptr<IOHandler> SubtitleHandler::serveContent(std::shared_ptr<CdsItem> item, int resNum)
{
    auto path = getContentPath(names, item);
    log_debug("Subtitle: Opening name: {}", path.c_str());

    auto io_handler = std::make_unique<FileIOHandler>(path);
    return io_handler;
}

ResourceHandler::ResourceHandler(std::shared_ptr<Config> config)
    : MetacontentHandler(std::move(config))
{
    std::vector<std::string> files = this->config->getArrayOption(CFG_IMPORT_RESOURCES_RESOURCE_FILE_LIST);
    names.insert(names.end(), files.begin(), files.end());
}

void ResourceHandler::fillMetadata(std::shared_ptr<CdsItem> item)
{
    auto path = getContentPath(ResourceHandler::names, item);
    log_debug("Running resource handler check on {} -> {}", item->getLocation().c_str(), path.c_str());

    if (!path.empty()) {
        if (tolower_string(path.c_str()) == tolower_string(item->getLocation().c_str())) {
            auto resource = std::make_shared<CdsResource>(CH_RESOURCE);
            resource->addAttribute(MetadataHandler::getResAttrName(R_PROTOCOLINFO), renderProtocolInfo("res"));
            item->addResource(resource);
        }
    }
}

std::unique_ptr<IOHandler> ResourceHandler::serveContent(std::shared_ptr<CdsItem> item, int resNum)
{
    auto path = getContentPath(ResourceHandler::names, item);
    log_debug("Resource: Opening name: {}", path.c_str());

    auto io_handler = std::make_unique<FileIOHandler>(path);
    return io_handler;
}
