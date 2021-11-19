/*MT*

    MediaTomb - http://www.mediatomb.cc/

    ffmpeg_handler.h - this file is part of MediaTomb.

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
/*
    This code was contributed by
    Copyright (C) 2007 Ingo Preiml <ipreiml@edu.uni-klu.ac.at>
*/

/// \file ffmpeg_handler.h
/// \brief Definition of the FfmpegHandler class - getting metadata via
/// ffmpeg library calls.

#ifdef HAVE_FFMPEG
#ifndef __FFMPEG_HANDLER_H__
#define __FFMPEG_HANDLER_H__

#include <optional>

#include "iohandler/io_handler.h"
#include "metadata_handler.h"

// forward declaration
struct AVFormatContext;

/// \brief This class is responsible for reading id3 tags metadata
class FfmpegHandler : public MetadataHandler {
public:
    explicit FfmpegHandler(const std::shared_ptr<Context>& context);
    void fillMetadata(const std::shared_ptr<CdsObject>& obj) override;
    std::unique_ptr<IOHandler> serveContent(const std::shared_ptr<CdsObject>& obj, int resNum) override;
    std::string getMimeType() const override;

private:
    // The ffmpegthumbnailer code (ffmpeg?) is not threading safe.
    // Add a lock around the usage to avoid crashing randomly.
    std::mutex thumb_mutex;
    std::map<std::string, std::string> specialPropertyMap;

    void addFfmpegAuxdataFields(const std::shared_ptr<CdsItem>& item, AVFormatContext* pFormatCtx) const;
    void addFfmpegMetadataFields(const std::shared_ptr<CdsItem>& item, AVFormatContext* pFormatCtx) const;
    void addFfmpegResourceFields(const std::shared_ptr<CdsItem>& item, AVFormatContext* pFormatCtx) const;
    std::optional<std::vector<std::byte>> readThumbnailCacheFile(const fs::path& movieFilename) const;
    void writeThumbnailCacheFile(const fs::path& movieFilename, const std::byte* data, std::size_t size) const;
};

fs::path getThumbnailCacheBasePath(const Config& config);
fs::path getThumbnailCachePath(const fs::path& base, const fs::path& movie);

#endif //__FFMPEG_HANDLER_H__
#endif // HAVE_FFMPEG
