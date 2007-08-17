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

using namespace zmm;

BufferedIOHandler::BufferedIOHandler(Ref<IOHandler> underlyingHandler, size_t bufSize, size_t maxChunkSize) : IOHandler()
{
    if (bufSize <=0)
        throw _Exception(_("bufSize must be positive"));
    if (maxChunkSize <= 0 || maxChunkSize > bufSize)
        throw _Exception(_("maxChunkSize must be positive and not be greater than bufSize"));
    if (underlyingHandler == nil)
        throw _Exception(_("underlyingHandler must not be nil"));
    
    mutex = Ref<Mutex>(new Mutex());
    cond = Ref<Cond>(new Cond(mutex));
    
    this->underlyingHandler = underlyingHandler;
    this->bufSize = bufSize;
    this->maxChunkSize = maxChunkSize;
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
    buffer = (char *)MALLOC(bufSize);
    underlyingHandler->open(mode);
    startBufferThread();
    isOpen = true;
}

int BufferedIOHandler::read(OUT char *buf, IN size_t length)
{
    //log_debug("buffered read\n");
    if (! isOpen)
        throw _Exception(_("read on closed BufferedIOHandler"));
    if (length <= 0)
        throw _Exception(_("length must be positive"));
    
    AUTOLOCK(mutex);
    while (empty && ! (eof || readError))
        cond->wait();
    
    if (readError)
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
    if (a == bufSize)
        a = 0;
    if (a == b)
    {
        empty = true;
        
        // start at the beginning of the buffer
        // disabled because it needs to by synced with threadProc
        // which thinks that b won't be changed by anyone else
        //a = b = 0;
    }
    
    if (wasFull)
        cond->signal();
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
    AUTOLOCK(mutex);
    do
    {
        size_t aLocal = a;
        maxWrite = (empty ? bufSize : (aLocal < b ? bufSize - b : aLocal - b));
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
                if (b == bufSize)
                    b = 0;
                if (empty)
                {
                    empty = false;
                    cond->signal();
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
}

/*
int BufferedIOHandler::getBufferFillsize(int a, int b)
{
    if (empty)
        return 0;
    int size = b - a;
    if (size <= 0)
        size += bufSize;
    return size;
}
*/
