/*GRB*

Gerbera - https://gerbera.io/

    ffmpeg_thumbnailer_handler.cc - this file is part of Gerbera.

    Copyright (C) 2022-2025 Gerbera Contributors

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
#define GRB_LOG_FAC GrbLogFacility::ffmpeg

#include "ffmpeg_thumbnailer_handler.h" // API

#include "cds/cds_item.h"
#include "config/config.h"
#include "config/config_val.h"
#include "iohandler/mem_io_handler.h"
#include "resolution.h"
#include "util/tools.h"

#include <libffmpegthumbnailer/filmstripfilter.h>
#include <libffmpegthumbnailer/videothumbnailer.h>

static void ffmpegThLogger(ThumbnailerLogLevel logLevel, const std::string message)
{
    switch (logLevel) {
    case ThumbnailerLogLevelError:
        log_error("FfmpegThumbnailerHandler: {}", message);
        break;
    case ThumbnailerLogLevelInfo:
    default:
        log_debug("FfmpegThumbnailerHandler: {}", message);
        break;
    }
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
    if (!isEnabled)
        return nullptr;

    auto item = std::dynamic_pointer_cast<CdsItem>(obj);
    if (!item)
        return nullptr;

    auto cacheEnabled = config->getBoolOption(ConfigVal::SERVER_EXTOPTS_FFMPEGTHUMBNAILER_CACHE_DIR_ENABLED);
    if (cacheEnabled) {
        auto data = readThumbnailCacheFile(item->getLocation());
        if (data) {
            log_debug("Returning cached thumbnail for file: {}", item->getLocation().c_str());
            return std::make_unique<MemIOHandler>(data->data(), data->size());
        }
    }

    try {
        auto thumbLock = std::scoped_lock<std::mutex>(thumb_mutex);

        auto th = ffmpegthumbnailer::VideoThumbnailer(config->getIntOption(ConfigVal::SERVER_EXTOPTS_FFMPEGTHUMBNAILER_THUMBSIZE), false, true, config->getIntOption(ConfigVal::SERVER_EXTOPTS_FFMPEGTHUMBNAILER_IMAGE_QUALITY), false);

        th.setLogCallback(ffmpegThLogger);
        th.setSeekPercentage(config->getIntOption(ConfigVal::SERVER_EXTOPTS_FFMPEGTHUMBNAILER_SEEK_PERCENTAGE));
        if (config->getBoolOption(ConfigVal::SERVER_EXTOPTS_FFMPEGTHUMBNAILER_FILMSTRIP_OVERLAY))
            th.addFilter(new ffmpegthumbnailer::FilmStripFilter());

        log_debug("Generating thumbnail for file: {}", item->getLocation().c_str());

        std::vector<uint8_t> img;
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

bool FfmpegThumbnailerHandler::fillMetadata(const std::shared_ptr<CdsObject>& obj)
{
    if (!isEnabled)
        return false;

    auto item = std::dynamic_pointer_cast<CdsItem>(obj);
    if (!item)
        return false;

    try {
        std::string videoResolution = item->getResource(ContentHandler::DEFAULT)->getAttribute(ResourceAttribute::RESOLUTION);
        if (videoResolution.empty())
            return false;
        auto resolution = Resolution(videoResolution);
        std::string thumbMimetype = getValueOrDefault(mimeContentTypeMappings, CONTENT_TYPE_JPG, "image/jpeg");

        auto thumbResource = std::make_shared<CdsResource>(ContentHandler::FFTH, ResourcePurpose::Thumbnail);
        thumbResource->addAttribute(ResourceAttribute::PROTOCOLINFO, renderProtocolInfo(thumbMimetype));

        auto y = config->getIntOption(ConfigVal::SERVER_EXTOPTS_FFMPEGTHUMBNAILER_THUMBSIZE) * resolution.y() / resolution.x();
        auto x = config->getIntOption(ConfigVal::SERVER_EXTOPTS_FFMPEGTHUMBNAILER_THUMBSIZE);
        thumbResource->addAttribute(ResourceAttribute::RESOLUTION, fmt::format("{}x{}", x, y));
        item->addResource(thumbResource);
        log_debug("Adding resource for {} thumbnail", EnumMapper::mapObjectType(mediaType));
        return true;

    } catch (const std::runtime_error& e) {
        log_warning("Failed to generate resource for thumbnail: {} {}", item->getLocation().c_str(), e.what());
    }
    return false;
}

FfmpegThumbnailerHandler::FfmpegThumbnailerHandler(
    const std::shared_ptr<Context>& context,
    ConfigVal checkOption,
    ObjectType mediaType)
    : MediaMetadataHandler(context, ConfigVal::SERVER_EXTOPTS_FFMPEGTHUMBNAILER_ENABLED)
    , mediaType(mediaType)
{
    isEnabled = this->isEnabled && config->getBoolOption(checkOption);
    if (isEnabled) {
        auto configuredDir = config->getOption(ConfigVal::SERVER_EXTOPTS_FFMPEGTHUMBNAILER_CACHE_DIR);
        if (!configuredDir.empty()) {
            cachePath = configuredDir;
        } else {
            auto home = config->getOption(ConfigVal::SERVER_HOME);
            cachePath = fs::path(home) / "cache-dir";
        }
    }
}

bool FfmpegThumbnailerHandler::isSupported(
    const std::string& contentType,
    bool isOggTheora,
    const std::string& mimeType,
    ObjectType mediaType)
{
    return (this->mediaType == ObjectType::Unknown && (mediaType == ObjectType::Video || mediaType == ObjectType::Image)) || this->mediaType == mediaType;
}

#endif
