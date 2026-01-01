/*GRB*

Gerbera - https://gerbera.io/

    ffmpeg_thumbnailer_handler.h - this file is part of Gerbera.

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

/// @file metadata/ffmpeg_thumbnailer_handler.h

#ifndef __FFMPEG_THUMBNAILER_HANDLER_H__
#define __FFMPEG_THUMBNAILER_HANDLER_H__
#ifdef HAVE_FFMPEGTHUMBNAILER

#include "metadata_handler.h"

#include "util/grb_fs.h"

#include <memory>
#include <mutex>

class CdsObject;
class IOHandler;
enum class ObjectType;

/// @brief class to treat virtual resource for videos and images to generate thumbnails
class FfmpegThumbnailerHandler : public MediaMetadataHandler {
public:
    explicit FfmpegThumbnailerHandler(
        const std::shared_ptr<Context>& context,
        ConfigVal checkOption,
        ObjectType mediaType);

    bool isSupported(const std::string& contentType,
        bool isOggTheora,
        const std::string& mimeType,
        ObjectType mediaType) override;

    bool fillMetadata(const std::shared_ptr<CdsObject>& obj) override;

    std::unique_ptr<IOHandler> serveContent(
        const std::shared_ptr<CdsObject>& obj,
        const std::shared_ptr<CdsResource>& resource) override;

protected:
    // Needed in tests
    const fs::path& getThumbnailCacheBasePath() const { return cachePath; }
    static fs::path getThumbnailCachePath(const fs::path& base, const fs::path& movie);

private:
    /// @brief The ffmpegthumbnailer code (ffmpeg?) is not threading safe.
    /// Add a lock around the usage to avoid crashing randomly.
    mutable std::mutex thumb_mutex;
    /// @brief location of cached thumbnails
    fs::path cachePath;
    /// @brief distinguish video or image
    ObjectType mediaType;

    /// @brief read cached thumbnail
    std::optional<std::vector<std::byte>> readThumbnailCacheFile(const fs::path& movieFilename) const;

    /// @brief cache generated thumb
    void writeThumbnailCacheFile(
        const fs::path& movieFilename,
        const std::byte* data,
        std::size_t size) const;
};

#endif // HAVE_FFMPEGTHUMBNAILER
#endif // __FFMPEG_THUMBNAILER_HANDLER_H__
