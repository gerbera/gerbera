/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    mpegremux_processor.h - this file is part of MediaTomb.
    
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

/// \file mpegremux_processor.h

#ifndef __MPEGREMUX_PROCESSOR_H__
#define __MPEGREMUX_PROCESSOR_H__

#define IOHC_NORMAL_SHUTDOWN 1
#define IOHC_FORCED_SHUTDOWN 2
#define IOHC_READ_ERROR 3
#define IOHC_WRITE_ERROR 4
#define IOHC_EXCEPTION 5


#include "thread_executor.h"

/// \brief gets a reader and writer fd, where input is assumed to be an
/// an MPEG PS stream, strips out unwanted streams and writes the remuxed
/// result to the output fd. Uses blocking read/write operations.

class MPEGRemuxProcessor : public ThreadExecutor
{
public:
    MPEGRemuxProcessor(int in_fd, int out_fd, unsigned char keep_audio_id);
    virtual int getStatus() { return status; }
protected:
    virtual void threadProc();
private:
    int status;
    int in_fd;
    int out_fd;
    unsigned char keep_audio_id;
};

#endif // __MPEGREMUX_PROCESSOR_H__
