/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    io_handler_chainer.cc - this file is part of MediaTomb.
    
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

/// \file io_handler_chainer.cc

#ifdef HAVE_CONFIG_H
    #include "autoconfig.h"
#endif

#include "io_handler_chainer.h"

using namespace zmm;

IOHandlerChainer::IOHandlerChainer(Ref<IOHandler> readFrom, Ref<IOHandler> writeTo, int chunkSize) : ThreadExecutor()
{
    if (chunkSize <=0)
        throw _Exception(_("chunkSize must be positive"));
    if (readFrom == NULL || writeTo == NULL)
        throw _Exception(_("readFrom and writeTo need to be set"));
    status = 0;
    this->chunkSize = chunkSize;
    this->readFrom = readFrom;
    this->writeTo = writeTo;
    readFrom->open(UPNP_READ);
    writeTo->open(UPNP_WRITE);
    buf = (char *)MALLOC(chunkSize);
    startThread();
}

void IOHandlerChainer::threadProc()
{
    try
    {
        bool stopLoop = false;
        while (! threadShutdownCheck() && ! stopLoop)
        {
            int numRead = readFrom->read(buf, chunkSize);
            if (numRead == 0)
            {
                status = IOHC_NORMAL_SHUTDOWN;
                stopLoop = true;
            }
            else if (numRead < 0)
            {
                status = IOHC_READ_ERROR;
                stopLoop = true;
            }
            else
            {
                if (! threadShutdownCheck())
                {
                    int numWritten = writeTo->write(buf, numRead);
                    if (numWritten != numRead)
                    {
                        status = IOHC_WRITE_ERROR;
                        stopLoop = true;
                    }
                }
            }
        }
    }
    catch (Exception)
    {
        status = IOHC_EXCEPTION;
    }
    if (threadShutdownCheck())
        status = IOHC_FORCED_SHUTDOWN;
    readFrom->close();
    writeTo->close();
}
