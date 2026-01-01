/*MT*

    MediaTomb - http://www.mediatomb.cc/

    io_handler_buffer_helper.cc - this file is part of MediaTomb.

    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>

    Copyright (C) 2006-2010 Gena Batyan <bgeradz@mediatomb.cc>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>,
                            Leonhard Wimmer <leo@mediatomb.cc>

    Copyright (C) 2016-2026 Gerbera Contributors

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

/// @file iohandler/io_handler_buffer_helper.cc
#define GRB_LOG_FAC GrbLogFacility::iohandler

#include "io_handler_buffer_helper.h" // API

#include "exceptions.h"

#include <algorithm>

IOHandlerBufferHelper::IOHandlerBufferHelper(std::shared_ptr<Config> config, std::size_t bufSize, std::size_t initialFillSize)
    : config(std::move(config))
    , bufSize(bufSize)
    , initialFillSize(initialFillSize)
    , waitForInitialFillSize(initialFillSize > 0)
{
    if (bufSize == 0)
        throw_std_runtime_error("bufSize must be greater than 0");
    if (initialFillSize > bufSize)
        throw_std_runtime_error("initialFillSize {} must be lesser than or equal to the size of the buffer {}", initialFillSize, bufSize);
}

void IOHandlerBufferHelper::open(enum UpnpOpenFileMode mode)
{
    if (isOpen)
        throw_std_runtime_error("tried to reopen an open IOHandlerBufferHelper");

    buffer = new std::byte[bufSize];
    startBufferThread();
    isOpen = true;
}

IOHandlerBufferHelper::~IOHandlerBufferHelper() noexcept
{
    if (isOpen)
        IOHandlerBufferHelper::close();
}

grb_read_t IOHandlerBufferHelper::read(std::byte* buf, std::size_t length)
{
    // check read on closed BufferedIOHandler
    assert(isOpen);
    // length must be positive
    assert(length > 0);

    // Lovely hack to ensure constuction of the child is complete before we try to do anything
    StdThreadRunner::waitFor(
        "IOHandlerBufferHelper", [this] { return threadRunner != nullptr; }, 100);
    auto lock = threadRunner->uniqueLock();

    while ((empty || waitForInitialFillSize) && !threadShutdown && !eof && !readError) {
        if (checkSocket) {
            checkSocket = false;
            return CHECK_SOCKET;
        }
        threadRunner->wait(lock);
    }

    if (readError || threadShutdown)
        return -1;
    if (empty && eof)
        return 0;

    std::size_t bLocal = b;
    lock.unlock();

    // we ensured with the while above that the buffer isn't empty
    auto currentFillSize = static_cast<int>(bLocal - a);
    if (currentFillSize <= 0)
        currentFillSize += bufSize;

    auto maxRead1 = (a < bLocal) ? bLocal - a : bufSize - a;
    auto read1 = std::min(length, maxRead1);

    auto remaining = (length > read1) ? length - read1 : 0;
    auto read2 = std::min(remaining, static_cast<std::size_t>(currentFillSize - read1));

    std::copy_n(buffer + a, read1, buf);
    if (read2)
        std::copy_n(buffer, read2, buf + read1);

    std::size_t didRead = read1 + read2;

    lock.lock();

    bool signalled = false;
    // was the buffer full or became it "full" while we read?
    if (signalAfterEveryRead || a == b) {
        threadRunner->notify();
        signalled = true;
    }

    a += didRead;
    if (a >= bufSize)
        a -= bufSize;
    if (a == b) {
        empty = true;
        if (!signalled)
            threadRunner->notify();
    }

    posRead += didRead;
    return didRead;
}

void IOHandlerBufferHelper::seek(off_t offset, int whence)
{
    log_debug("seek called: {} {}", offset, whence);
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

    auto lock = threadRunner->uniqueLock();

    // if another seek isn't processed yet - well we don't care as this new seek
    // will change the position anyway
    doSeek = true;
    seekOffset = offset;
    seekWhence = whence;

    // tell the probably sleeping thread to process our seek
    threadRunner->notify();

    // wait until the seek has been processed
    threadRunner->wait(lock, [this] { return !doSeek || threadShutdown || eof || readError; });
}

void IOHandlerBufferHelper::close()
{
    if (!isOpen)
        log_error("close called on closed IOHandlerBufferHelper");
    isOpen = false;
    stopBufferThread();
    delete[] buffer;
    buffer = nullptr;
}

// thread stuff...

void IOHandlerBufferHelper::startBufferThread()
{
    threadRunner = std::make_unique<StdThreadRunner>(
        "BufferHelperThread", [](void* arg) {
            auto inst = static_cast<IOHandlerBufferHelper*>(arg);
            inst->threadProc();
        },
        this);
}

void IOHandlerBufferHelper::stopBufferThread()
{
    auto lock = threadRunner->uniqueLock();
    threadShutdown = true;
    threadRunner->notify();
    lock.unlock();

    threadRunner->join();
    threadRunner = nullptr;
}
