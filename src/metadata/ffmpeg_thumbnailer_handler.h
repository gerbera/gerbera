/*GRB*

Gerbera - https://gerbera.io/

    ffmpeg_thumbnailer_handler.h - this file is part of Gerbera.

    Copyright (C) 2022 Gerbera Contributors

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

/// \file ffmpeg_thumbnailer_handler.h

#ifndef GERBERA_FFMPEG_THUMBNAILER_HANDLER_H
#define GERBERA_FFMPEG_THUMBNAILER_HANDLER_H

#include <memory>

#include "config/config.h"
#include "metadata_handler.h"

class CdsObject;
class IOHandler;

class FfmpegThumbnailerHandler : public MetadataHandler {
public:
    explicit FfmpegThumbnailerHandler(const std::shared_ptr<Context>& context, config_option_t checkOption);
    void fillMetadata(const std::shared_ptr<CdsObject>& obj) override;
    std::unique_ptr<IOHandler> serveContent(const std::shared_ptr<CdsObject>& obj, int resNum) override;

    // fixme: these should be private
    fs::path getThumbnailCacheBasePath() const;
    static fs::path getThumbnailCachePath(const fs::path& base, const fs::path& movie);

private:
    // The ffmpegthumbnailer code (ffmpeg?) is not threading safe.
    // Add a lock around the usage to avoid crashing randomly.
    mutable std::mutex thumb_mutex;

    bool enabled {};
    std::optional<std::vector<std::byte>> readThumbnailCacheFile(const fs::path& movieFilename) const;
    void writeThumbnailCacheFile(const fs::path& movieFilename, const std::byte* data, std::size_t size) const;
};

#endif // GERBERA_FFMPEG_THUMBNAILER_HANDLER_H
