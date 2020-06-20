/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    io_handler_buffer_helper.cc - this file is part of MediaTomb.
    
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

/// \file io_handler_buffer_helper.cc

#include "io_handler_buffer_helper.h" // API

#include <cstdlib>

#include "config/config_manager.h"

IOHandlerBufferHelper::IOHandlerBufferHelper(size_t bufSize, size_t initialFillSize)
{
    if (bufSize == 0)
        throw_std_runtime_error("bufSize must be greater than 0");
    if (initialFillSize > bufSize)
        throw_std_runtime_error("initialFillSize must be lesser than or equal to the size of the buffer");

    this->bufSize = bufSize;
    this->initialFillSize = initialFillSize;
    waitForInitialFillSize = (initialFillSize > 0);
    buffer = nullptr;
    isOpen = false;
    threadShutdown = false;
    eof = false;
    readError = false;
    a = b = posRead = 0;
    empty = true;
    signalAfterEveryRead = false;
    checkSocket = false;

    seekEnabled = false;
    doSeek = false;
}

void IOHandlerBufferHelper::open(enum UpnpOpenFileMode mode)
{
    if (isOpen)
        throw_std_runtime_error("tried to reopen an open IOHandlerBufferHelper");
    buffer = static_cast<char*>(malloc(bufSize));
    if (buffer == nullptr)
        throw_std_runtime_error("Failed to allocate memory for transcoding buffer");

    startBufferThread();
    isOpen = true;
}

IOHandlerBufferHelper::~IOHandlerBufferHelper() noexcept
{
    if (isOpen)
        close();
}

size_t IOHandlerBufferHelper::read(char* buf, size_t length)
{
    // check read on closed BufferedIOHandler
    assert(isOpen);
    // length must be positive
    assert(length > 0);

    std::unique_lock<std::mutex> lock(mutex);

    while ((empty || waitForInitialFillSize) && !(threadShutdown || eof || readError)) {
        if (checkSocket) {
            checkSocket = false;
            return CHECK_SOCKET;
        }
        cond.wait(lock);
    }

    if (readError || threadShutdown)
        return -1;
    if (empty && eof)
        return 0;

    size_t bLocal = b;
    lock.unlock();

    // we ensured with the while above that the buffer isn't empty
    int currentFillSize = bLocal - a;
    if (currentFillSize <= 0)
        currentFillSize += bufSize;
    size_t maxRead1 = (a < bLocal ? bLocal - a : bufSize - a);
    size_t read1 = (maxRead1 > length ? length : maxRead1);
    size_t maxRead2 = currentFillSize - read1;
    size_t read2 = (read1 < length ? length - read1 : 0);
    if (read2 > maxRead2)
        read2 = maxRead2;

    memcpy(buf, buffer + a, read1);
    if (read2)
        memcpy(buf + read1, buffer, read2);

    size_t didRead = read1 + read2;

    lock.lock();

    bool signalled = false;
    // was the buffer full or became it "full" while we read?
    if (signalAfterEveryRead || a == b) {
        cond.notify_one();
        signalled = true;
    }

    a += didRead;
    if (a >= bufSize)
        a -= bufSize;
    if (a == b) {
        empty = true;
        if (!signalled)
            cond.notify_one();
    }

    posRead += didRead;
    return didRead;
}

void IOHandlerBufferHelper::seek(off_t offset, int whence)
{
    log_debug("seek called: %lld {}", offset, whence);
    if (!seekEnabled)
        throw_std_runtime_error("seek currently disabled in this IOHandlerBufferHelper");

    assert(isOpen);

    // check for valid input
    assert(whence == SEEK_SET || whence == SEEK_CUR || whence == SEEK_END);
    assert(whence != SEEK_SET || offset >= 0);
    assert(whence != SEEK_END || offset <= 0);

    // do nothing in this case
    if (whence == SEEK_CUR && offset == 0)
        return;

    std::unique_lock<std::mutex> lock(mutex);

    // if another seek isn't processed yet - well we don't care as this new seek
    // will change the position anyway
    doSeek = true;
    seekOffset = offset;
    seekWhence = whence;

    // tell the probably sleeping thread to process our seek
    cond.notify_one();

    // wait until the seek has been processed
    cond.wait(lock, [&]() {
        return !doSeek || threadShutdown || eof || readError;
    });
}

void IOHandlerBufferHelper::close()
{
    if (!isOpen)
        throw_std_runtime_error("close called on closed IOHandlerBufferHelper");
    isOpen = false;
    stopBufferThread();
    free(buffer);
    buffer = nullptr;
}

// thread stuff...

void IOHandlerBufferHelper::startBufferThread()
{
    pthread_create(
        &bufferThread,
        nullptr, // attr
        IOHandlerBufferHelper::staticThreadProc,
        this);
}

void IOHandlerBufferHelper::stopBufferThread()
{
    std::unique_lock<std::mutex> lock(mutex);
    threadShutdown = true;
    cond.notify_one();
    lock.unlock();

    if (bufferThread)
        pthread_join(bufferThread, nullptr);
    bufferThread = 0;
}

void* IOHandlerBufferHelper::staticThreadProc(void* arg)
{
    auto inst = static_cast<IOHandlerBufferHelper*>(arg);
    inst->threadProc();
    pthread_exit(nullptr);
}
