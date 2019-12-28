/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    buffered_io_handler.h - this file is part of MediaTomb.
    
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

/// \file buffered_io_handler.h

#ifndef __BUFFERED_IO_HANDLER_H__
#define __BUFFERED_IO_HANDLER_H__

#include <upnp.h>

#include "common.h"
#include "io_handler_buffer_helper.h"

/// \brief a IOHandler with buffer support
/// the buffer is only for read(). write() is not supported
/// the public functions of this class are *not* thread safe!
class BufferedIOHandler : public IOHandlerBufferHelper {
public:
    /// \brief get an instance of a BufferedIOHandler
    /// \param underlyingHandler the IOHandler from which the buffer should read
    /// \param bufSize the size of the buffer in bytes
    /// \param maxChunkSize the maximum size of the chunks which are read by the buffer
    /// \param initialFillSize the number of bytes which have to be in the buffer
    /// before the first read at the very beginning or after a seek returns;
    /// 0 disables the delay
    BufferedIOHandler(zmm::Ref<IOHandler> underlyingHandler, size_t bufSize, size_t maxChunkSize, size_t initialFillSize);

    virtual void open(enum UpnpOpenFileMode mode);
    virtual void close();

private:
    zmm::Ref<IOHandler> underlyingHandler;
    size_t maxChunkSize;

    virtual void threadProc();
};

#endif // __BUFFERED_IO_HANDLER_H__
