/*MT*

    MediaTomb - http://www.mediatomb.cc/

    io_handler_buffer_helper.h - this file is part of MediaTomb.

    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>

    Copyright (C) 2006-2010 Gena Batyan <bgeradz@mediatomb.cc>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>,
                            Leonhard Wimmer <leo@mediatomb.cc>

    Copyright (C) 2016-2024 Gerbera Contributors

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

/// \file io_handler_buffer_helper.h

#ifndef __IO_HANDLER_BUFFER_HELPER_H__
#define __IO_HANDLER_BUFFER_HELPER_H__

#include "io_handler.h" // Base

#include "util/thread_runner.h"

#include <upnp.h>

class Config;

/// \brief a IOHandler with buffer support
/// the buffer is only for read(). write() is not supported
/// the public functions of this class are *not* thread safe!
class IOHandlerBufferHelper : public IOHandler {
public:
    /// \brief get an instance of a IOHandlerBufferHelper
    /// \param config Access to the configuration
    /// \param bufSize the size of the buffer in bytes
    /// \param initialFillSize the number of bytes which have to be in the buffer
    /// before the first read at the very beginning or after a seek returns;
    /// 0 disables the delay
    IOHandlerBufferHelper(std::shared_ptr<Config> config, std::size_t bufSize, std::size_t initialFillSize);
    ~IOHandlerBufferHelper() noexcept override;

    IOHandlerBufferHelper(const IOHandlerBufferHelper&) = delete;
    IOHandlerBufferHelper& operator=(const IOHandlerBufferHelper&) = delete;

    // inherited from IOHandler
    void open(enum UpnpOpenFileMode mode) override;
    std::size_t read(std::byte* buf, std::size_t length) override;
    void seek(off_t offset, int whence) override;
    void close() override;

protected:
    std::shared_ptr<Config> config;
    std::size_t bufSize;
    std::size_t initialFillSize;
    std::byte* buffer {};
    bool isOpen {};
    bool eof {};
    bool readError {};
    bool waitForInitialFillSize;
    bool signalAfterEveryRead {};
    bool checkSocket {};

    // buffer stuff..
    bool empty { true };
    std::size_t a {};
    std::size_t b {};
    off_t posRead {};

    // seek stuff...
    bool seekEnabled {};
    bool doSeek {};
    off_t seekOffset {};
    int seekWhence {};

    // thread stuff..
    void startBufferThread();
    void stopBufferThread();
    virtual void threadProc() = 0;

    std::unique_ptr<StdThreadRunner> threadRunner;
    bool threadShutdown {};
};

#endif // __IO_HANDLER_BUFFER_HELPER_H__
