/*GRB*

Gerbera - https://gerbera.io/

    ffmpeg_thumbnailer_handler.cc - this file is part of Gerbera.

    Copyright (C) 2022-2026 Gerbera Contributors

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

/// @file metadata/ffmpeg_thumbnailer_handler.cc

#ifdef HAVE_FFMPEGTHUMBNAILER
#define GRB_LOG_FAC GrbLogFacility::thumbnailer

#include "ffmpeg_thumbnailer_handler.h" // API

#include "cds/cds_item.h"
#include "config/config.h"
#include "config/config_val.h"
#include "iohandler/mem_io_handler.h"
#include "resolution.h"
#include "util/tools.h"

#include <libffmpegthumbnailer/filmstripfilter.h>
#include <libffmpegthumbnailer/videothumbnailer.h>

/// @brief logger function for thumbnailer issues
static void ffmpegThLogger(ThumbnailerLogLevel logLevel, const std::string& message)
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

/// @brief filter implementation to rotate image
class RotationFilter : public ffmpegthumbnailer::IFilter {
protected:
    double degrees;

    /// @brief image size after rotation
    static std::pair<int, int> computeRotatedSize(int width, int height, double radians)
    {
        double abs_cos = std::abs(std::cos(radians));
        double abs_sin = std::abs(std::sin(radians));
        int outWidth = static_cast<int>(std::ceil(width * abs_cos - height * abs_sin));
        int outHeight = static_cast<int>(std::ceil(width * abs_sin + height * abs_cos));
        return { outWidth, outHeight };
    }

public:
    /// @brief create filter
    /// @param degrees rotation in degree (0 to 360)
    RotationFilter(double degrees)
        : degrees(degrees)
    {
    }

    RotationFilter(const RotationFilter&) = delete;
    RotationFilter& operator=(const RotationFilter&) = delete;

    /// @brief implementation of filter logic
    void process(ffmpegthumbnailer::VideoFrame& videoFrame)
    {
        double radians = (degrees * M_PI) / 180;
        double sinf = std::sin(radians);
        double cosf = std::cos(radians);

        auto [height, width] = computeRotatedSize(videoFrame.width, videoFrame.height, -radians);

        double centerXOrig = 0.5 * (videoFrame.width - 1); // point to rotate about
        double centerYOrig = 0.5 * (videoFrame.height - 1); // center of image

        double centerX = 0.5 * (width - 1); // point to rotate about
        double centerY = 0.5 * (height - 1); // center of image

        int lineSize = ((width * 3) % 32) > 0 ? ((width * 3) / 32 + 1) * 32 : (width * 3);
        std::vector<uint8_t> frameData(lineSize * height, 0);

        log_debug("rotate from {}: {}x{}={} -> {}x{}={} ", degrees, videoFrame.width, videoFrame.height, videoFrame.lineSize, width, height, lineSize);
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                long double x1 = x - centerX;
                long double y1 = y - centerY;
                long long xx = static_cast<long long>(std::round(x1 * cosf - y1 * sinf + centerXOrig));
                long long yy = static_cast<long long>(std::round(x1 * sinf + y1 * cosf + centerYOrig));

                long long pixelIndexOrig = yy * videoFrame.lineSize + xx * 3;
                long long pixelIndex = y * lineSize + x * 3;

                if (pixelIndex < 0 || pixelIndex > static_cast<long long>(frameData.size())) {
                    log_debug("rotate out of sight {}", pixelIndex);
                    continue;
                }

                if (xx >= 0 && xx < videoFrame.width && yy >= 0 && yy < videoFrame.height) {
                    frameData[pixelIndex + 0] = videoFrame.frameData.at(pixelIndexOrig + 0);
                    frameData[pixelIndex + 1] = videoFrame.frameData.at(pixelIndexOrig + 1);
                    frameData[pixelIndex + 2] = videoFrame.frameData.at(pixelIndexOrig + 2);
                } else {
                    frameData[pixelIndex + 0] = 0;
                    frameData[pixelIndex + 1] = 0;
                    frameData[pixelIndex + 2] = 0;
                }
            }
        }
        videoFrame.frameData = frameData;
        videoFrame.height = height;
        videoFrame.width = width;
        videoFrame.lineSize = lineSize;
    }
};

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

void FfmpegThumbnailerHandler::writeThumbnailCacheFile(
    const fs::path& movieFilename,
    const std::byte* data,
    std::size_t size) const
{
    try {
        auto path = getThumbnailCachePath(getThumbnailCacheBasePath(), movieFilename);
        fs::create_directories(path.parent_path());
        GrbFile(path).writeBinaryFile(data, size);
    } catch (const std::runtime_error& e) {
        log_error("Failed to write thumbnail cache: {}", e.what());
    }
}

std::unique_ptr<IOHandler> FfmpegThumbnailerHandler::serveContent(
    const std::shared_ptr<CdsObject>& obj,
    const std::shared_ptr<CdsResource>& resource)
{
    if (!enabled)
        return nullptr;

    auto item = std::dynamic_pointer_cast<CdsItem>(obj);
    if (!item)
        return nullptr;
    auto itemLocation = item->getLocation();
    if (itemLocation.empty()) {
        log_warning("Missing location for {}", item->getTitle());
        return nullptr;
    }

    if (cacheEnabled) {
        auto data = readThumbnailCacheFile(itemLocation);
        if (data) {
            log_debug("Returning cached thumbnail for file: {}", itemLocation.c_str());
            return std::make_unique<MemIOHandler>(data->data(), data->size());
        }
    }

    std::unique_ptr<ffmpegthumbnailer::FilmStripFilter> filmStripFilter;
    std::unique_ptr<RotationFilter> rotationFilter;
    try {
        auto thumbLock = std::scoped_lock<std::mutex>(thumb_mutex);

        auto th = ffmpegthumbnailer::VideoThumbnailer(thumbSize, false, true, imageQuality, false);

        th.setLogCallback(ffmpegThLogger);
        th.setSeekPercentage(seekPercentage);
        if (doRotate && resource) {
            // 1: Normal (0° rotation)
            // 3: Upside-down (180° rotation)
            // 6: Rotated 90° counterclockwise (270° clockwise)
            // 8: Rotated 90° clockwise (270° counterclockwise)
            int orientation = stoiString(resource->getAttribute(ResourceAttribute::ORIENTATION));
            double rotation = 0;
            if (orientation == 6) {
                rotation = 90;
            } else if (orientation == 8) {
                rotation = -90;
            } else if (orientation == 3) {
                rotation = 180;
            }
            if (rotation != 0) {
                rotationFilter = std::make_unique<RotationFilter>(rotation);
                th.addFilter(rotationFilter.get());
            }
        }
        if (stripOverlay) {
            filmStripFilter = std::make_unique<ffmpegthumbnailer::FilmStripFilter>();
            th.addFilter(filmStripFilter.get());
        }

        log_debug("Generating thumbnail for file: {}", itemLocation.c_str());

        std::vector<uint8_t> img;
        th.generateThumbnail(itemLocation, Jpeg, img);
        if (cacheEnabled) {
            writeThumbnailCacheFile(itemLocation, reinterpret_cast<std::byte*>(img.data()), img.size());
        }

        return std::make_unique<MemIOHandler>(img.data(), img.size());
    } catch (const std::logic_error& e) {
        log_warning("Thumbnail generation failed for file {}: {}", itemLocation.c_str(), e.what());
        return nullptr;
    }
}

bool FfmpegThumbnailerHandler::fillMetadata(const std::shared_ptr<CdsObject>& obj)
{
    if (!enabled)
        return false;

    auto item = std::dynamic_pointer_cast<CdsItem>(obj);
    if (!item)
        return false;

    auto itemLocation = item->getLocation();
    if (itemLocation.empty()) {
        log_warning("Missing location for {}", item->getTitle());
        return false;
    }
    try {
        std::string videoResolution = item->getResource(ContentHandler::DEFAULT)->getAttribute(ResourceAttribute::RESOLUTION);
        if (videoResolution.empty())
            return false;
        auto resolution = Resolution(videoResolution);
        std::string thumbMimetype = getValueOrDefault(mimeContentTypeMappings, CONTENT_TYPE_JPG, "image/jpeg");

        auto thumbResource = std::make_shared<CdsResource>(ContentHandler::FFTH, ResourcePurpose::Thumbnail);
        thumbResource->addAttribute(ResourceAttribute::PROTOCOLINFO, renderProtocolInfo(thumbMimetype));

        auto y = thumbSize * resolution.y() / resolution.x();
        auto x = thumbSize;
        thumbResource->addAttribute(ResourceAttribute::RESOLUTION, fmt::format("{}x{}", x, y));
        item->addResource(thumbResource);
        log_debug("Adding resource to {} for {} thumbnail", itemLocation.c_str(), EnumMapper::mapObjectType(mediaType));
        return true;

    } catch (const std::runtime_error& e) {
        log_warning("Failed to generate resource for thumbnail: {} {}", itemLocation.c_str(), e.what());
    }
    return false;
}

FfmpegThumbnailerHandler::FfmpegThumbnailerHandler(
    const std::shared_ptr<Context>& context,
    ConfigVal checkOption,
    ObjectType mediaType)
    : MediaMetadataHandler(context, ConfigVal::SERVER_EXTOPTS_FFMPEGTHUMBNAILER_ENABLED)
    , mediaType(mediaType)
    , thumbSize(config->getIntOption(ConfigVal::SERVER_EXTOPTS_FFMPEGTHUMBNAILER_THUMBSIZE))
    , seekPercentage(config->getIntOption(ConfigVal::SERVER_EXTOPTS_FFMPEGTHUMBNAILER_SEEK_PERCENTAGE))
    , imageQuality(config->getIntOption(ConfigVal::SERVER_EXTOPTS_FFMPEGTHUMBNAILER_IMAGE_QUALITY))
    , cacheEnabled(config->getBoolOption(ConfigVal::SERVER_EXTOPTS_FFMPEGTHUMBNAILER_CACHE_DIR_ENABLED))
    , stripOverlay(config->getBoolOption(ConfigVal::SERVER_EXTOPTS_FFMPEGTHUMBNAILER_FILMSTRIP_OVERLAY))
    , doRotate(config->getBoolOption(ConfigVal::SERVER_EXTOPTS_FFMPEGTHUMBNAILER_ROTATE))
{
    enabled = this->enabled && config->getBoolOption(checkOption);

    if (enabled) {
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
