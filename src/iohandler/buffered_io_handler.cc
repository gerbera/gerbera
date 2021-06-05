/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    buffered_io_handler.cc - this file is part of MediaTomb.
    
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

/// \file buffered_io_handler.cc

#include "buffered_io_handler.h" // API

#include <cassert>

#include "util/tools.h"

class Config;

BufferedIOHandler::BufferedIOHandler(std::shared_ptr<Config> config, std::unique_ptr<IOHandler> underlyingHandler, size_t bufSize, size_t maxChunkSize, size_t initialFillSize)
    : IOHandlerBufferHelper(std::move(config), bufSize, initialFillSize)
{
    if (underlyingHandler == nullptr)
        throw_std_runtime_error("underlyingHandler must not be nullptr");
    if (maxChunkSize == 0)
        throw_std_runtime_error("maxChunkSize must be greater than 0");
    this->underlyingHandler = std::move(underlyingHandler);
    this->maxChunkSize = maxChunkSize;

    // test it first!
    //seekEnabled = true;
}

void BufferedIOHandler::open(enum UpnpOpenFileMode mode)
{
    // do the open here instead of threadProc() because it may throw an exception
    underlyingHandler->open(mode);
    IOHandlerBufferHelper::open(mode);
}

void BufferedIOHandler::close()
{
    IOHandlerBufferHelper::close();
    // do the close here instead of threadProc() because it may throw an exception
    underlyingHandler->close();
}

void BufferedIOHandler::threadProc()
{
    int readBytes = 0;
    size_t maxWrite;

#ifdef TOMBDEBUG
    std::chrono::milliseconds last_log;
    bool first_log = true;
#endif

    auto lock = threadRunner->uniqueLock();
    do {

#ifdef TOMBDEBUG
        if (first_log || getDeltaMillis(last_log) > std::chrono::seconds(1)) {
            if (first_log)
                first_log = false;
            last_log = currentTimeMS();
            float percentFillLevel = 0;
            if (!empty) {
                auto currentFillSize = int(b - a);
                if (currentFillSize <= 0)
                    currentFillSize += bufSize;
                percentFillLevel = (float(currentFillSize) / float(bufSize)) * 100;
            }
            log_debug("buffer fill level: {:03.2f}%  (bufSize: {}; a: {}; b: {})", percentFillLevel, bufSize, a, b);
        }
#endif
        if (empty)
            a = b = 0;

        if (doSeek && !empty && (seekWhence == SEEK_SET || (seekWhence == SEEK_CUR && seekOffset > 0))) {
            auto currentFillSize = int(b - a);
            if (currentFillSize <= 0)
                currentFillSize += bufSize;

            auto relSeek = int(seekOffset);
            if (seekWhence == SEEK_SET)
                relSeek -= posRead;

            if (relSeek <= currentFillSize) { // we have everything we need in the buffer already
                a += relSeek;
                posRead += relSeek;
                if (a >= bufSize)
                    a -= bufSize;
                if (a == b) {
                    empty = true;
                    a = b = 0;
                }

                /// \todo do we need to wait for initialFillSize again?

                doSeek = false;
                threadRunner->notify();
            }
        }

        // note: seeking could be optimized some more (backward seeking)
        // but this should suffice for now

        if (doSeek) { // seek not been processed yet
            try {
                underlyingHandler->seek(seekOffset, seekWhence);
                empty = true;
                a = b = 0;
            } catch (const std::runtime_error& e) {
                log_error("Error while seeking in buffer: {}", e.what());
            }

            /// \todo should we do that?
            waitForInitialFillSize = (initialFillSize > 0);

            doSeek = false;
            threadRunner->notify();
        }

        maxWrite = (empty ? bufSize : (a < b ? bufSize - b : a - b));
        if (maxWrite == 0) {
            threadRunner->wait(lock);
        } else {
            lock.unlock();
            size_t chunkSize = (maxChunkSize > maxWrite ? maxWrite : maxChunkSize);
            readBytes = underlyingHandler->read(buffer + b, chunkSize);
            lock.lock();
            if (readBytes > 0) {
                b += readBytes;
                assert(b <= bufSize);
                if (b == bufSize)
                    b = 0;
                if (empty) {
                    empty = false;
                    threadRunner->notify();
                }
                if (waitForInitialFillSize) {
                    auto currentFillSize = int(b - a);
                    if (currentFillSize <= 0)
                        currentFillSize += bufSize;
                    if (size_t(currentFillSize) >= initialFillSize) {
                        log_debug("buffer: initial fillsize reached");
                        waitForInitialFillSize = false;
                        threadRunner->notify();
                    }
                }
            } else if (readBytes == CHECK_SOCKET) {
                checkSocket = true;
                threadRunner->notify();
            }
        }
    } while ((maxWrite == 0 || readBytes > 0 || readBytes == CHECK_SOCKET) && !threadShutdown);
    if (!threadShutdown) {
        if (readBytes == 0)
            eof = true;
        if (readBytes < 0)
            readError = true;
    }
    // ensure that read() doesn't wait for me to fill the buffer
    threadRunner->notify();
}
