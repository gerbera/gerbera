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

#include "io_handler_buffer_helper.h"
#include "config_manager.h"

using namespace zmm;

IOHandlerBufferHelper::IOHandlerBufferHelper(size_t bufSize, size_t initialFillSize) : IOHandler()
{
    if (bufSize <=0)
        throw _Exception(_("bufSize must be positive"));
    if (initialFillSize < 0 || initialFillSize > bufSize)
        throw _Exception(_("initialFillSize must be non-negative and must be lesser than or equal to the size of the buffer"));
    
    mutex = Ref<Mutex>(new Mutex());
    cond = Ref<Cond>(new Cond(mutex));
    
    this->bufSize = bufSize;
    this->initialFillSize = initialFillSize;
    waitForInitialFillSize = (initialFillSize > 0);
    buffer = NULL;
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

void IOHandlerBufferHelper::open(IN enum UpnpOpenFileMode mode)
{
    if (isOpen)
        throw _Exception(_("tried to reopen an open IOHandlerBufferHelper"));
    buffer = (char *)MALLOC(bufSize);
    if (buffer == NULL)
        throw _Exception(_("Failed to allocate memory for transcoding buffer!"));

    startBufferThread();
    isOpen = true;
}

IOHandlerBufferHelper::~IOHandlerBufferHelper()
{
    if (isOpen)
        close();
}

int IOHandlerBufferHelper::read(OUT char *buf, IN size_t length)
{
    // check read on closed BufferedIOHandler
    assert(isOpen);
    // length must be positive
    assert(length > 0);
    
    AUTOLOCK(mutex);
    
    while ((empty || waitForInitialFillSize) && ! (threadShutdown || eof || readError))
    {
        if (checkSocket)
        {
            checkSocket = false;
            return CHECK_SOCKET;
        }
        else
            cond->wait();
    }
    
    if (readError || threadShutdown)
        return -1;
    if (empty && eof)
        return 0;
    
    size_t bLocal = b;
    AUTOUNLOCK();
    
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
    
    size_t didRead = read1+read2;
    
    AUTORELOCK();
    
    bool signalled = false;
    // was the buffer full or became it "full" while we read?
    if (signalAfterEveryRead || a == b)
    {
        cond->signal();
        signalled = true;
    }
    
    a += didRead;
    if (a >= bufSize)
        a -= bufSize;
    if (a == b)
    {
        empty = true;
        if (! signalled)
            cond->signal();
    }
    
    posRead += didRead;
    return didRead;
}

void IOHandlerBufferHelper::seek(IN off_t offset, IN int whence)
{
    log_debug("seek called: %lld %d\n", offset, whence);
    if (! seekEnabled)
        throw _Exception(_("seek currently disabled in this IOHandlerBufferHelper"));
    
    assert(isOpen);
    
    // check for valid input
    assert(whence == SEEK_SET || whence == SEEK_CUR || whence == SEEK_END);
    assert(whence != SEEK_SET || offset >= 0);
    assert(whence != SEEK_END || offset <= 0);
    
    // do nothing in this case
    if (whence == SEEK_CUR && offset == 0)
        return;
    
    AUTOLOCK(mutex);
    
    // if another seek isn't processed yet - well we don't care as this new seek
    // will change the position anyway
    doSeek = true;
    seekOffset = offset;
    seekWhence = whence;
    
    // tell the probably sleeping thread to process our seek
    cond->signal();
    
    // wait until the seek has been processed
    while(doSeek && ! (threadShutdown || eof || readError))
        cond->wait();
}

void IOHandlerBufferHelper::close()
{
    if (! isOpen)
        throw _Exception(_("close called on closed IOHandlerBufferHelper"));
    isOpen = false;
    stopBufferThread();
    FREE(buffer);
    buffer = NULL;
}

// thread stuff...

void IOHandlerBufferHelper::startBufferThread()
{
    pthread_create(
        &bufferThread,
        NULL, // attr
        IOHandlerBufferHelper::staticThreadProc,
        this
    );
}

void IOHandlerBufferHelper::stopBufferThread()
{
    AUTOLOCK(mutex);
    threadShutdown = true;
    cond->signal();
    AUTOUNLOCK();
    if (bufferThread)
        pthread_join(bufferThread, NULL);
    bufferThread = 0;
}

void *IOHandlerBufferHelper::staticThreadProc(void *arg)
{
    log_debug("starting buffer thread... thread: %d\n", pthread_self());
    IOHandlerBufferHelper *inst = (IOHandlerBufferHelper *)arg;
    inst->threadProc();
    log_debug("buffer thread shut down. thread: %d\n", pthread_self());
    pthread_exit(NULL);
    return NULL;
}
