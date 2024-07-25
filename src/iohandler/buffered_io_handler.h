/*MT*

    MediaTomb - http://www.mediatomb.cc/

    buffered_io_handler.h - this file is part of MediaTomb.

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

/// \file buffered_io_handler.h

#ifndef __BUFFERED_IO_HANDLER_H__
#define __BUFFERED_IO_HANDLER_H__

#include "io_handler_buffer_helper.h"

#include <memory>
#include <upnp.h>

/// \brief a IOHandler with buffer support
/// the buffer is only for read(). write() is not supported
/// the public functions of this class are *not* thread safe!
class BufferedIOHandler : public IOHandlerBufferHelper {
public:
    /// \brief get an instance of a BufferedIOHandler
    /// \param config Access configuration
    /// \param underlyingHandler the IOHandler from which the buffer should read
    /// \param bufSize the size of the buffer in bytes
    /// \param maxChunkSize the maximum size of the chunks which are read by the buffer
    /// \param initialFillSize the number of bytes which have to be in the buffer
    /// before the first read at the very beginning or after a seek returns;
    /// 0 disables the delay
    BufferedIOHandler(const std::shared_ptr<Config>& config, std::unique_ptr<IOHandler> underlyingHandler, std::size_t bufSize, std::size_t maxChunkSize, std::size_t initialFillSize);

    void open(enum UpnpOpenFileMode mode) override;
    void close() override;

private:
    std::unique_ptr<IOHandler> underlyingHandler;
    std::size_t maxChunkSize;

    void threadProc() override;
};

#endif // __BUFFERED_IO_HANDLER_H__
