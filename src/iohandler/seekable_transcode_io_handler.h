/*GRB*

    Gerbera - https://gerbera.io/

    iohandler/seekable_transcode_io_handler.h - this file is part of Gerbera.

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

/// @file iohandler/seekable_transcode_io_handler.h

#ifndef __SEEKABLE_TRANSCODE_IO_HANDLER_H__
#define __SEEKABLE_TRANSCODE_IO_HANDLER_H__

#include "io_handler.h" // Base

#include <memory>
#include <string>

class CdsObject;
class Content;
class TranscodingProfile;

/// @brief A transcoded stream served like a file.
///
/// With a constant bit rate the stream carries bitrate / 8 bytes per second,
/// so a byte offset is a point in time. For a range request libupnp calls seek()
/// right after open() and before any read() (httpreadwrite.c, http_SendMessage):
/// the agent is started at the first read, from the time that matches the offset,
/// which it gets in seconds through %range, 0 for a request from the beginning.
class SeekableTranscodeIOHandler : public IOHandler {
public:
    SeekableTranscodeIOHandler(
        std::shared_ptr<Content> content,
        std::shared_ptr<TranscodingProfile> profile,
        std::string path,
        std::shared_ptr<CdsObject> obj,
        std::string group);

    void open(enum UpnpOpenFileMode mode) override;
    grb_read_t read(std::byte* buf, std::size_t length) override;
    void seek(off_t seekOffset, int whence) override;
    off_t tell() override;
    void close() override;

private:
    std::shared_ptr<Content> content;
    std::shared_ptr<TranscodingProfile> profile;
    std::string path;
    std::shared_ptr<CdsObject> obj;
    std::string group;
    enum UpnpOpenFileMode mode { UPNP_READ };
    off_t offset {};
    off_t position {};
    std::unique_ptr<IOHandler> transcoder;
};

#endif // __SEEKABLE_TRANSCODE_IO_HANDLER_H__
