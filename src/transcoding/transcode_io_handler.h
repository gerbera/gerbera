/*GRB*

    Gerbera - https://gerbera.io/

    transcode_io_handler.h - this file is part of Gerbera.

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
/// @file transcoding/transcode_io_handler.h
#ifndef __TRANSCODE_IO_HANDLER_H__
#define __TRANSCODE_IO_HANDLER_H__
#ifdef HAVE_FFMPEG

#include "iohandler/io_handler.h"
#include "util/grb_fs.h"

#include <map>
#include <memory>
#include <mutex>
#include <vector>

extern "C" {
struct AVFormatContext;
struct AVPacket;
}

class FilteringContext;
class CdsObject;
class StreamContext;
class TranscodingProfile;

/// @brief Convert input to output file, applying some hard-coded filter-graph
///        on both audio and video streams.
class TranscodeInternalIOHandler : public IOHandler {
protected:
    /// @brief context for input file
    AVFormatContext* inFormatContext = nullptr;
    /// @brief context for output file
    AVFormatContext* outFormatContext = nullptr;
    std::map<std::size_t, std::shared_ptr<FilteringContext>> filteringCtx;
    std::map<std::size_t, std::shared_ptr<StreamContext>> streamCtx;
    /// @brief storage for active packet
    AVPacket* packet = nullptr;
    /// @brief output of transcoding not yet read by client
    std::vector<std::byte> extraData;

    /// @brief Name of the file.
    GrbFile file;
    std::shared_ptr<TranscodingProfile> profile;
    std::shared_ptr<CdsObject> obj;

    /// @brief open file with offset
    off_t offset;

    /// brief remember end of input if still data to read
    bool isEOF { false };

    /// @brief lock against multiple access from client
    std::mutex mutex;

    int open_input_file();
    int open_output_file(const std::string& filename);
    int init_filters(void);
    int flush_encoder(std::size_t stream_index);
    /// @brief convert input packet to output stream
    int filterPacket();
    /// @brief read from output stream
    std::vector<std::byte> readBuffer();
    int closeOutput();

public:
    /// @brief Sets the filename to work with.
    explicit TranscodeInternalIOHandler(
        const fs::path& filename,
        const std::shared_ptr<TranscodingProfile>& profile,
        const std::shared_ptr<CdsObject>& obj,
        off_t offset = 0);
    ~TranscodeInternalIOHandler() override;

    /// @brief Opens file for reading (writing is not supported)
    void open(enum UpnpOpenFileMode mode) override;

    /// @brief Reads a previously opened file sequentially.
    /// @param buf Data from the file will be copied into this buffer.
    /// @param length Number of bytes to be copied into the buffer.
    grb_read_t read(std::byte* buf, std::size_t length) override;

    /// @brief Performs seek on an open file.
    /// @param offset Number of bytes to move in the file. For seeking forwards
    /// positive values are used, for seeking backwards - negative. Offset must
    /// be positive if origin is set to SEEK_SET
    /// @param whence The position to move relative to. SEEK_CUR to move relative
    /// to current position, SEEK_END to move relative to the end of file,
    /// SEEK_SET to specify an absolute offset.
    void seek(off_t offset, int whence) override;

    /// @brief Return the current stream position.
    off_t tell() override;

    /// @brief Close a previously opened file.
    void close() override;

    /// @brief Run Transcoding to create new file.
    int run(const std::string& outputFileName);
};

#endif // HAVE_FFMPEG
#endif // __TRANSCODE_IO_HANDLER_H__
