/*MT*

    MediaTomb - http://www.mediatomb.cc/

    buffered_io_handler.cc - this file is part of MediaTomb.

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

/// \file buffered_io_handler.cc
#define LOG_FAC log_facility_t::iohandler

#include "buffered_io_handler.h" // API

#include "util/grb_time.h"

class Config;

BufferedIOHandler::BufferedIOHandler(const std::shared_ptr<Config>& config, std::unique_ptr<IOHandler> underlyingHandler, std::size_t bufSize, std::size_t maxChunkSize, std::size_t initialFillSize)
    : IOHandlerBufferHelper(config, bufSize, initialFillSize)
{
    if (!underlyingHandler)
        throw_std_runtime_error("underlyingHandler must not be nullptr");
    if (maxChunkSize == 0)
        throw_std_runtime_error("maxChunkSize must be greater than 0");
    this->underlyingHandler = std::move(underlyingHandler);
    this->maxChunkSize = maxChunkSize;

    // test it first!
    // seekEnabled = true;
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
    std::size_t maxWrite;
    StdThreadRunner::waitFor("BufferedIOHandler", [this] { return threadRunner != nullptr; });

#ifdef TOMBDEBUG
    std::chrono::milliseconds lastLog;
    bool firstLog = true;
#endif

    auto lock = threadRunner->uniqueLock();
    do {
#ifdef TOMBDEBUG
        if (firstLog || getDeltaMillis(lastLog) > std::chrono::milliseconds(100)) {
            if (firstLog)
                firstLog = false;
            lastLog = currentTimeMS();
            [[maybe_unused]] float percentFillLevel = 0;
            if (!empty) {
                auto currentFillSize = static_cast<int>(b - a);
                if (currentFillSize <= 0)
                    currentFillSize += bufSize;
                percentFillLevel = (static_cast<float>(currentFillSize) / static_cast<float>(bufSize)) * 100;
            }
            log_debug("buffer fill level: {:03.2f}%  (bufSize: {}; a: {}; b: {})", percentFillLevel, bufSize, a, b);
        }
#endif
        if (empty)
            a = b = 0;

        if (doSeek && !empty && (seekWhence == SEEK_SET || (seekWhence == SEEK_CUR && seekOffset > 0))) {
            auto currentFillSize = static_cast<int>(b - a);
            if (currentFillSize <= 0)
                currentFillSize += bufSize;

            auto relSeek = static_cast<int>(seekOffset);
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
            std::size_t chunkSize = (maxChunkSize > maxWrite ? maxWrite : maxChunkSize);
            readBytes = underlyingHandler->read(buffer + static_cast<int>(b), chunkSize);
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
                    auto currentFillSize = static_cast<int>(b - a);
                    if (currentFillSize <= 0)
                        currentFillSize += bufSize;
                    if (static_cast<std::size_t>(currentFillSize) >= initialFillSize) {
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
