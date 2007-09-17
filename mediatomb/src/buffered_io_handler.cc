/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    buffered_io_handler.cc - this file is part of MediaTomb.
    
    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>
    
    Copyright (C) 2006-2007 Gena Batyan <bgeradz@mediatomb.cc>,
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

#ifdef HAVE_CONFIG_H
    #include "autoconfig.h"
#endif

#include "buffered_io_handler.h"
#include "tools.h"

using namespace zmm;

BufferedIOHandler::BufferedIOHandler(Ref<IOHandler> underlyingHandler, size_t bufSize, size_t maxChunkSize, size_t initialFillSize) : IOHandler()
{
    if (bufSize <=0)
        throw _Exception(_("bufSize must be positive"));
    if (maxChunkSize <= 0)
        throw _Exception(_("maxChunkSize must be positive"));
    if (initialFillSize < 0 || initialFillSize > bufSize)
        throw _Exception(_("initialFillSize must be non-negative and must be lesser than or equal to the size of the buffer"));
    if (underlyingHandler == nil)
        throw _Exception(_("underlyingHandler must not be nil"));
    
    mutex = Ref<Mutex>(new Mutex());
    cond = Ref<Cond>(new Cond(mutex));
    
    this->underlyingHandler = underlyingHandler;
    this->bufSize = bufSize;
    this->maxChunkSize = maxChunkSize;
    this->initialFillSize = initialFillSize;
    waitForInitialFillSize = (initialFillSize > 0);
    buffer = NULL;
    isOpen = false;
    threadShutdown = false;
    eof = false;
    readError = false;
    a = b = 0;
    empty = true;
}

void BufferedIOHandler::open(IN enum UpnpOpenFileMode mode)
{
    if (isOpen)
        throw _Exception(_("tried to reopen an open BufferedIOHandler"));
    buffer = (char *)MALLOC(bufSize);
    if (buffer == NULL)
        throw _Exception(_("Failed to allocate memory for transcoding buffer!"));

    underlyingHandler->open(mode);
    startBufferThread();
    isOpen = true;
}

BufferedIOHandler::~BufferedIOHandler()
{
    if (isOpen)
        close();
}

int BufferedIOHandler::read(OUT char *buf, IN size_t length)
{
    if (! isOpen)
        throw _Exception(_("read on closed BufferedIOHandler"));
    if (length <= 0)
        throw _Exception(_("length must be positive"));
    
    AUTOLOCK(mutex);
    while ((empty || waitForInitialFillSize) && ! (threadShutdown || eof || readError))
        cond->wait();
    
    if (readError || threadShutdown)
        return -1;
    if (empty && eof)
        return 0;
    
    size_t bLocal = b;
    AUTOUNLOCK();
    
    // we ensured with the while above that the buffer isn't empty
    size_t maxRead = (a < bLocal ? bLocal - a : bufSize - a);
    size_t doRead = (maxRead > length ? length : maxRead);
    memcpy(buf, buffer + a, doRead);
    
    AUTORELOCK();
    bool wasFull = (a == bLocal);
    a += doRead;
    assert(a <= bufSize);
    if (a == bufSize)
        a = 0;
    if (a == b)
    {
        empty = true;
        cond->signal();
        // start at the beginning of the buffer
        // disabled because it needs to by synced with threadProc
        // which thinks that b won't be changed by anyone else
        //a = b = 0;
    }
    
    if (wasFull)
        cond->signal();
    //log_debug("read on buffer - requested %d; sent %d\n", length, doRead);
    return doRead;
}

void BufferedIOHandler::seek(IN off_t offset, IN int whence)
{
    throw _Exception(_("seek currently unimplemented for BufferedIOHandler"));
}

void BufferedIOHandler::close()
{
    if (! isOpen)
        throw _Exception(_("close called on closed BufferedIOHandler"));
    isOpen = false;
    stopBufferThread();
    FREE(buffer);
    buffer = NULL;
    underlyingHandler->close();
}

// thread stuff...

void BufferedIOHandler::startBufferThread()
{
    pthread_create(
        &bufferThread,
        NULL, // attr
        BufferedIOHandler::staticThreadProc,
        this
    );
}

void BufferedIOHandler::stopBufferThread()
{
    AUTOLOCK(mutex);
    threadShutdown = true;
    cond->signal();
    AUTOUNLOCK();
    if (bufferThread)
        pthread_join(bufferThread, NULL);
    bufferThread = 0;
}

void *BufferedIOHandler::staticThreadProc(void *arg)
{
    log_debug("starting buffer thread... thread: %d\n", pthread_self());
    BufferedIOHandler *inst = (BufferedIOHandler *)arg;
    inst->threadProc();
    log_debug("buffer thread shut down. thread: %d\n", pthread_self());
    pthread_exit(NULL);
    return NULL;
}

void BufferedIOHandler::threadProc()
{
    int readBytes;
    size_t maxWrite;
    
#ifdef LOG_TOMBDEBUG
    struct timespec last_log;
    bool first_log = true;
#endif
    
    AUTOLOCK(mutex);
    do
    {
        
#ifdef LOG_TOMBDEBUG
        if (first_log || getDeltaMillis(&last_log) > 1000)
        {
            if (first_log)
                first_log = false;
            getTimespecNow(&last_log);
            float percentFillLevel = 0;
            if (! empty)
            {
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
        maxWrite = (empty ? bufSize: (a < b ? bufSize - b : a - b));
        if (maxWrite == 0)
        {
            cond->wait();
        }
        else
        {
            AUTOUNLOCK();
            size_t chunkSize = (maxChunkSize > maxWrite ? maxWrite : maxChunkSize);
            readBytes = underlyingHandler->read(buffer + b, chunkSize);
            AUTORELOCK();
            if (readBytes > 0)
            {
                b += readBytes;
                assert(b <= bufSize);
                if (b == bufSize)
                    b = 0;
                if (empty)
                {
                    empty = false;
                    cond->signal();
                }
                if (waitForInitialFillSize)
                {
                    size_t currentFillSize = b - a;
                    if (currentFillSize <= 0)
                        currentFillSize += bufSize;
                    if (currentFillSize >= initialFillSize)
                    {
                        log_debug("buffer: initial fillsize reached\n");
                        waitForInitialFillSize = false;
                        cond->signal();
                    }
                }
            }
        }
    }
    while((maxWrite == 0 || readBytes > 0) && ! threadShutdown);
    if (! threadShutdown)
    {
        if (readBytes == 0)
            eof = true;
        if (readBytes < 0)
            readError = true;
    }
    // ensure that read() doesn't wait for me to fill the buffer
    cond->signal();
}
