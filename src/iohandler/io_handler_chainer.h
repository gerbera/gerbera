/*MT*

    MediaTomb - http://www.mediatomb.cc/

    io_handler_chainer.h - this file is part of MediaTomb.

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

/// \file io_handler_chainer.h

#ifndef __IO_HANDLER_CHAINER_H__
#define __IO_HANDLER_CHAINER_H__

#define IOHC_NORMAL_SHUTDOWN 1
#define IOHC_FORCED_SHUTDOWN 2
#define IOHC_READ_ERROR 3
#define IOHC_WRITE_ERROR 4
#define IOHC_EXCEPTION 5

#include "io_handler.h"
#include "util/thread_executor.h"

/// \brief gets two IOHandler, starts a thread which reads from one IOHandler
/// and writes the data to the other IOHandler
class IOHandlerChainer : public ThreadExecutor {
public:
    /// \brief initialize the IOHandlerChainer
    /// \param readFrom the IOHandler to read from
    /// \param writeTo the IOHandler to write to
    /// \param chunkSize the amount of bytes to read/write at once. a buffer of
    /// this size will be allocated
    IOHandlerChainer(std::unique_ptr<IOHandler> readFrom, std::unique_ptr<IOHandler> writeTo, int chunkSize);
    ~IOHandlerChainer() override
    {
        if (buf) {
            delete[] buf;
            buf = nullptr;
        }
    }

    IOHandlerChainer(const IOHandlerChainer&) = delete;
    IOHandlerChainer& operator=(const IOHandlerChainer&) = delete;

    int getStatus() override { return status; }

protected:
    void threadProc() override;

private:
    int status;
    std::byte* buf;
    int chunkSize;
    std::unique_ptr<IOHandler> readFrom;
    std::unique_ptr<IOHandler> writeTo;
};

#endif // __IO_HANDLER_CHAINER_H__
