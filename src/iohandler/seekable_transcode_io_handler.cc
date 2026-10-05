/*GRB*

    Gerbera - https://gerbera.io/

    iohandler/seekable_transcode_io_handler.cc - this file is part of Gerbera.

    Copyright (C) 2026 Gerbera Contributors

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

/// @file iohandler/seekable_transcode_io_handler.cc
#define GRB_LOG_FAC GrbLogFacility::iohandler

#include "seekable_transcode_io_handler.h" // API

#include "config/result/transcoding.h"
#include "exceptions.h"
#include "transcoding/transcode_dispatcher.h"
#include "util/logger.h"

SeekableTranscodeIOHandler::SeekableTranscodeIOHandler(
    std::shared_ptr<Content> content,
    std::shared_ptr<TranscodingProfile> profile,
    std::string path,
    std::shared_ptr<CdsObject> obj,
    std::string group)
    : content(std::move(content))
    , profile(std::move(profile))
    , path(std::move(path))
    , obj(std::move(obj))
    , group(std::move(group))
{
}

void SeekableTranscodeIOHandler::open(enum UpnpOpenFileMode mode)
{
    this->mode = mode;
}

grb_read_t SeekableTranscodeIOHandler::read(std::byte* buf, std::size_t length)
{
    // Server::ReadCallback does not catch exceptions
    try {
        if (!transcoder) {
            // also 0, so that the agent can always pass it on, e.g. to ffmpeg's -ss
            std::string range = "0";
            if (offset > 0)
                range = fmt::format("{:.3f}", static_cast<double>(offset) * 8.0 / static_cast<double>(profile->getSeekBitrate()));
            log_debug("Transcoding {} from byte {}, start time '{}'", path, offset, range);
            auto transcodeDispatcher = std::make_unique<TranscodeDispatcher>(content);
            transcoder = transcodeDispatcher->serveContent(profile, path, obj, group, range);
            transcoder->open(mode);
        }
        auto ret = transcoder->read(buf, length);
        if (ret > 0)
            position += ret;
        return ret;
    } catch (const std::exception& e) {
        log_error("Transcoding {} failed: {}", path, e.what());
        return GRB_READ_ERROR;
    }
}

void SeekableTranscodeIOHandler::seek(off_t seekOffset, int whence)
{
    if (transcoder)
        throw_std_runtime_error("Seek in a running transcoding of {}", path);
    if (whence == SEEK_SET)
        offset = seekOffset;
    else if (whence == SEEK_CUR)
        offset += seekOffset;
    else
        throw_std_runtime_error("Seek from the end of a transcoding of {}", path);
    if (offset < 0)
        offset = 0;
    position = offset;
}

off_t SeekableTranscodeIOHandler::tell()
{
    return position;
}

void SeekableTranscodeIOHandler::close()
{
    if (transcoder)
        transcoder->close();
}
