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

#include <cassert>

#include "buffered_io_handler.h"
#include "tools.h"

using namespace zmm;
using namespace std;

BufferedIOHandler::BufferedIOHandler(Ref<IOHandler> underlyingHandler, size_t bufSize, size_t maxChunkSize, size_t initialFillSize)
    : IOHandlerBufferHelper(bufSize, initialFillSize)
{
    if (underlyingHandler == nullptr)
        throw _Exception(_("underlyingHandler must not be nullptr"));
    if (maxChunkSize <= 0)
        throw _Exception(_("maxChunkSize must be positive"));
    this->underlyingHandler = underlyingHandler;
    this->maxChunkSize = maxChunkSize;

    // test it first!
    //seekEnabled = true;
}

void BufferedIOHandler::open(IN enum UpnpOpenFileMode mode)
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
    int readBytes;
    size_t maxWrite;

#ifdef TOMBDEBUG
    struct timespec last_log;
    bool first_log = true;
#endif

    unique_lock<std::mutex> lock(mutex);
    do {

#ifdef TOMBDEBUG
        if (first_log || getDeltaMillis(&last_log) > 1000) {
            if (first_log)
                first_log = false;
            getTimespecNow(&last_log);
            float percentFillLevel = 0;
            if (!empty) {
                int currentFillSize = b - a;
                if (currentFillSize <= 0)
                    currentFillSize += bufSize;
                percentFillLevel = ((float)currentFillSize / (float)bufSize) * 100;
            }
            log_debug("buffer fill level: %3.2f%%  (bufSize: %d; a: %d; b: %d)\n", percentFillLevel, bufSize, a, b);
        }
#endif
        if (empty)
            a = b = 0;

        if (doSeek && !empty && (seekWhence == SEEK_SET || (seekWhence == SEEK_CUR && seekOffset > 0))) {
            int currentFillSize = b - a;
            if (currentFillSize <= 0)
                currentFillSize += bufSize;

            int relSeek = seekOffset;
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
                cond.notify_one();
            }
        }

        // note: seeking could be optimized some more (backward seeking)
        // but this should suffice for now

        if (doSeek) { // seek not been processed yet
            try {
                underlyingHandler->seek(seekOffset, seekWhence);
                empty = true;
                a = b = 0;
            } catch (const Exception& e) {
                log_error("Error while seeking in buffer: %s\n", e.getMessage().c_str());
                e.printStackTrace();
            }

            /// \todo should we do that?
            waitForInitialFillSize = (initialFillSize > 0);

            doSeek = false;
            cond.notify_one();
        }

        maxWrite = (empty ? bufSize : (a < b ? bufSize - b : a - b));
        if (maxWrite == 0) {
            cond.wait(lock);
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
                    cond.notify_one();
                }
                if (waitForInitialFillSize) {
                    int currentFillSize = b - a;
                    if (currentFillSize <= 0)
                        currentFillSize += bufSize;
                    if ((size_t)currentFillSize >= initialFillSize) {
                        log_debug("buffer: initial fillsize reached\n");
                        waitForInitialFillSize = false;
                        cond.notify_one();
                    }
                }
            } else if (readBytes == CHECK_SOCKET) {
                checkSocket = true;
                cond.notify_one();
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
    cond.notify_one();
}
