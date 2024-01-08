/*GRB*

Gerbera - https://gerbera.io/

    ffmpeg_thumbnailer_handler.cc - this file is part of Gerbera.

    Copyright (C) 2022-2024 Gerbera Contributors

    Gerbera is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.

    Gerbera is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Gerbera.  If not, see <http://www.gnu.org/licenses/>.
*/

/// \file ffmpeg_thumbnailer_handler.cc

#ifdef HAVE_FFMPEGTHUMBNAILER
#define LOG_FAC log_facility_t::ffmpeg

#include "ffmpeg_thumbnailer_handler.h" // API

#include <libffmpegthumbnailer/filmstripfilter.h>
#include <libffmpegthumbnailer/videothumbnailer.h>

#include "cds/cds_item.h"
#include "iohandler/mem_io_handler.h"
#include "resolution.h"
#include "util/tools.h"

fs::path FfmpegThumbnailerHandler::getThumbnailCacheBasePath() const
{
    auto configuredDir = config->getOption(CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_CACHE_DIR);
    if (!configuredDir.empty())
        return configuredDir;

    auto home = config->getOption(CFG_SERVER_HOME);
    return fs::path(home) / "cache-dir";
}

fs::path FfmpegThumbnailerHandler::getThumbnailCachePath(const fs::path& base, const fs::path& movie)
{
    assert(movie.is_absolute());

    auto path = base / movie.relative_path();
    path += "-thumb.jpg";
    return path;
}

std::optional<std::vector<std::byte>> FfmpegThumbnailerHandler::readThumbnailCacheFile(const fs::path& movieFilename) const
{
    auto path = getThumbnailCachePath(getThumbnailCacheBasePath(), movieFilename);
    return GrbFile(path).readBinaryFile();
}

void FfmpegThumbnailerHandler::writeThumbnailCacheFile(const fs::path& movieFilename, const std::byte* data, std::size_t size) const
{
    try {
        auto path = getThumbnailCachePath(getThumbnailCacheBasePath(), movieFilename);
        fs::create_directories(path.parent_path());
        GrbFile(path).writeBinaryFile(data, size);
    } catch (const std::runtime_error& e) {
        log_error("Failed to write thumbnail cache: {}", e.what());
    }
}

std::unique_ptr<IOHandler> FfmpegThumbnailerHandler::serveContent(const std::shared_ptr<CdsObject>& obj, const std::shared_ptr<CdsResource>& resource)
{
    if (!enabled)
        return nullptr;

    auto item = std::dynamic_pointer_cast<CdsItem>(obj);
    if (!item)
        return nullptr;

    auto cacheEnabled = config->getBoolOption(CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_CACHE_DIR_ENABLED);
    if (cacheEnabled) {
        auto data = readThumbnailCacheFile(item->getLocation());
        if (data) {
            log_debug("Returning cached thumbnail for file: {}", item->getLocation().c_str());
            return std::make_unique<MemIOHandler>(data->data(), data->size());
        }
    }

    try {
        auto thumbLock = std::scoped_lock<std::mutex>(thumb_mutex);

        auto th = ffmpegthumbnailer::VideoThumbnailer(config->getIntOption(CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_THUMBSIZE), false, true, config->getIntOption(CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_IMAGE_QUALITY), false);
        std::vector<uint8_t> img;

        th.setSeekPercentage(config->getIntOption(CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_SEEK_PERCENTAGE));
        if (config->getBoolOption(CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_FILMSTRIP_OVERLAY))
            th.addFilter(new ffmpegthumbnailer::FilmStripFilter());

        log_debug("Generating thumbnail for file: {}", item->getLocation().c_str());

        th.generateThumbnail(item->getLocation().c_str(), Jpeg, img);
        if (cacheEnabled) {
            writeThumbnailCacheFile(item->getLocation(), reinterpret_cast<std::byte*>(img.data()), img.size());
        }

        return std::make_unique<MemIOHandler>(img.data(), img.size());
    } catch (const std::logic_error& e) {
        log_warning("Thumbnail generation failed for file {}: {}", item->getLocation().c_str(), e.what());
        return nullptr;
    }
}

void FfmpegThumbnailerHandler::fillMetadata(const std::shared_ptr<CdsObject>& obj)
{
    if (!enabled)
        return;

    auto item = std::dynamic_pointer_cast<CdsItem>(obj);
    if (!item)
        return;

    try {
        std::string videoResolution = item->getResource(ContentHandler::DEFAULT)->getAttribute(CdsResource::Attribute::RESOLUTION);
        if (videoResolution.empty())
            return;
        auto resolution = Resolution(videoResolution);
        auto mappings = config->getDictionaryOption(CFG_IMPORT_MAPPINGS_MIMETYPE_TO_CONTENTTYPE_LIST);

        auto it = mappings.find(CONTENT_TYPE_JPG);
        std::string thumbMimetype = it != mappings.end() && !it->second.empty() ? it->second : "image/jpeg";

        auto thumbResource = std::make_shared<CdsResource>(ContentHandler::FFTH, CdsResource::Purpose::Thumbnail);
        thumbResource->addAttribute(CdsResource::Attribute::PROTOCOLINFO, renderProtocolInfo(thumbMimetype));

        auto y = config->getIntOption(CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_THUMBSIZE) * resolution.y() / resolution.x();
        auto x = config->getIntOption(CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_THUMBSIZE);
        thumbResource->addAttribute(CdsResource::Attribute::RESOLUTION, fmt::format("{}x{}", x, y));
        item->addResource(thumbResource);
        log_debug("Adding resource for video thumbnail");

    } catch (const std::runtime_error& e) {
        log_warning("Failed to generate resource for thumbnail: {} {}", item->getLocation().c_str(), e.what());
    }
}

FfmpegThumbnailerHandler::FfmpegThumbnailerHandler(const std::shared_ptr<Context>& context, config_option_t checkOption)
    : MetadataHandler(context)
    , enabled(config->getBoolOption(CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_ENABLED) && config->getBoolOption(checkOption))
{
}

#endif
