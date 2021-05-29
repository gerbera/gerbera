/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    process_io_handler.h - this file is part of MediaTomb.
    
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

/// \file process_io_handler.h
/// \brief Definition of the ProcessIOHandler class.
#ifndef __PROCESS_IO_HANDLER_H__
#define __PROCESS_IO_HANDLER_H__

#include <filesystem>
#include <memory>
namespace fs = std::filesystem;

#include "common.h"
#include "io_handler.h"
#include "util/executor.h"

// forward declaration
class ContentManager;

#define FIFO_READ_TIMEOUT 2
#define FIFO_WRITE_TIMEOUT 2

class ProcListItem {
public:
    explicit ProcListItem(std::shared_ptr<Executor> exec, bool abortOnDeath = false);
    std::shared_ptr<Executor> getExecutor();
    bool abortOnDeath() const;

protected:
    std::shared_ptr<Executor> executor;
    bool abort;
};

/// \brief Allows the web server to read from a fifo.
class ProcessIOHandler : public IOHandler {
public:
    /// \brief Sets the filename to work with.
    /// \param filename to read the data from
    /// \param procList associated processes that will be terminated once
    /// they are no longer needed
    ProcessIOHandler(std::shared_ptr<ContentManager> content,
        fs::path filename, const std::shared_ptr<Executor>& mainProc,
        std::vector<std::shared_ptr<ProcListItem>> procList = std::vector<std::shared_ptr<ProcListItem>>(),
        bool ignoreSeek = false);
    ~ProcessIOHandler() override;

    ProcessIOHandler(const ProcessIOHandler&) = delete;
    ProcessIOHandler& operator=(const ProcessIOHandler&) = delete;

    /// \brief Opens file for reading (writing is not supported)
    void open(enum UpnpOpenFileMode mode) override;

    /// \brief Reads a previously opened file sequentially.
    /// \param buf Data from the file will be copied into this buffer.
    /// \param length Number of bytes to be copied into the buffer.
    size_t read(char* buf, size_t length) override;

    /// \brief Writes to a previously opened file.
    /// \param buf Data from the buffer will be written to the file.
    /// \param length Number of bytes to be written from the buffer.
    /// \return number of bytes written.
    size_t write(char* buf, size_t length) override;

    /// \brief Performs seek on an open file.
    /// \param offset Number of bytes to move in the file. For seeking forwards
    /// positive values are used, for seeking backwards - negative. Offset must
    /// be positive if origin is set to SEEK_SET
    /// \param whence The position to move relative to. SEEK_CUR to move relative
    /// to current position, SEEK_END to move relative to the end of file,
    /// SEEK_SET to specify an absolute offset.
    void seek(off_t offset, int whence) override;

    /// \brief Close a previously opened file and kills the kill_pid process
    void close() override;

protected:
    std::shared_ptr<ContentManager> content;

    /// \brief List of associated processes.
    std::vector<std::shared_ptr<ProcListItem>> procList;

    /// \brief Main process used for reading
    std::shared_ptr<Executor> mainProc;

    /// \brief name of the file or fifo to read the data from
    fs::path filename;

    /// \brief file descriptor
    int fd {};

    /// \brief if this flag is set seek on a fifo will not return an error
    bool ignoreSeek;

    bool abort() const;
    void killAll() const;
    void registerAll();
    void unregisterAll();
};

#endif // __PROCESS_IO_HANDLER_H__
