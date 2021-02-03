/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    io_handler_chainer.cc - this file is part of MediaTomb.
    
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

/// \file io_handler_chainer.cc

#include "io_handler_chainer.h" // API

#include <thread>

#include "exceptions.h"

IOHandlerChainer::IOHandlerChainer(std::unique_ptr<IOHandler>& readFrom, std::unique_ptr<IOHandler>& writeTo, int chunkSize)
{
    if (chunkSize <= 0)
        throw_std_runtime_error("chunkSize must be positive");
    if (readFrom == nullptr || writeTo == nullptr)
        throw_std_runtime_error("readFrom and writeTo need to be set");
    status = 0;
    readFrom->open(UPNP_READ);
    this->chunkSize = chunkSize;
    this->readFrom = std::move(readFrom);
    this->writeTo = std::move(writeTo);
    buf = new char[chunkSize];
    startThread();
}

void IOHandlerChainer::threadProc()
{
    try {
        bool again = false;
        do {
            try {
                writeTo->open(UPNP_WRITE);
                again = false;
            } catch (const TryAgainException& e) {
                again = true;
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        } while (!threadShutdownCheck() && again);

        bool stopLoop = false;
        while (!threadShutdownCheck() && !stopLoop) {
            int numRead = readFrom->read(buf, chunkSize);
            if (numRead == 0) {
                status = IOHC_NORMAL_SHUTDOWN;
                stopLoop = true;
            } else if (numRead < 0) {
                status = IOHC_READ_ERROR;
                stopLoop = true;
            } else {
                int numWritten = 0;
                while (!threadShutdownCheck() && numWritten == 0 && !stopLoop) {
                    numWritten = writeTo->write(buf, numRead);
                    if (numWritten != 0 && numWritten != numRead) {
                        status = IOHC_WRITE_ERROR;
                        stopLoop = true;
                    }
                }
            }
        }
    } catch (const std::runtime_error& e) {
        log_debug("{}", e.what());
        status = IOHC_EXCEPTION;
    }
    try {
        if (threadShutdownCheck())
            status = IOHC_FORCED_SHUTDOWN;
        readFrom->close();
        writeTo->close();
    } catch (const std::runtime_error& e) {
        log_debug("{}", e.what());
        status = IOHC_EXCEPTION;
    }
}
