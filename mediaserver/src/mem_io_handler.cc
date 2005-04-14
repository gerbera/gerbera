/*  mem_io_handler.cc - this file is part of MediaTomb.
                                                                                
    Copyright (C) 2005 Gena Batyan <bgeradz@deadlock.dhs.org>,
                       Sergey Bostandzhyan <jin@deadlock.dhs.org>
                                                                                
    MediaTomb is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
                                                                                
    MediaTomb is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
                                                                                
    You should have received a copy of the GNU General Public License
    along with MediaTomb; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

/// \file mem_io_handler.cc
#include "server.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <upnp/ixml.h>
#include <time.h>
#include "common.h"
#include "storage.h"
#include "cds_objects.h"
#include "process.h"
#include "update_manager.h"
#include "mem_io_handler.h"
#include "dictionary.h"

using namespace zmm;
using namespace mxml;

MemIOHandler::MemIOHandler(String buffer) : IOHandler()
{
    this->buffer = buffer;
}

void MemIOHandler::open(IN enum UpnpOpenFileMode mode)
{
    pos = 0;
}

int MemIOHandler::read(OUT char *buf, IN size_t length)
{
    int ret = 0;
    void *p;

    // we indicate EOF by setting pos to -1
    if (pos == -1)
    {
        return 0;
    }
    
    p = memccpy(buf, buffer.c_str() + pos, '\0', length);
    if (p == NULL)
    {
        pos = pos + length;
        ret = (int) length;

        if (pos > buffer.length())
        {
            pos = -1;
        }
    }
    else
    {
        pos = pos + strlen(buf);
        ret = strlen(buf);
        if (pos > buffer.length())
        {
            pos = -1;
        }
    }
   
    return ret; 
 }
                                                                                                                                                                         
void MemIOHandler::seek(IN long offset, IN int whence)
{
    if (whence == SEEK_SET)
    {
        // offset must be positive when SEEK_SET is used
        if (offset < 0) 
        {
            throw Exception("MemIOHandler seek failed: SEEK_SET used with negative offset");
        }

        if (offset > buffer.length())
        {
            throw Exception("MemIOHandler seek failed: trying to seek past the end of file");
        }

        pos = offset;
    }
    else if (whence == SEEK_CUR)
    {
        long temp;

        if (pos == -1) 
        {
            temp = buffer.length();
        }
        else
        {
            temp = pos;
        }
        
        if (((temp + offset) > buffer.length()) ||
            ((temp + offset) < 0))
        {
            throw Exception("MemIOHandler seek failed: trying to seek before the beginning/past end of file");
        }

        pos = temp + offset;
    }
    else if (whence == SEEK_END)
    {
        long temp = buffer.length();
        if (((temp + offset) > buffer.length()) ||
            ((temp + offset) < 0))
        {
            throw Exception("MemIOHandler seek failed: trying to seek before the beginning/past end of file");
        }

        pos = temp + offset;
    }
    else
    {
        throw Exception("MemIOHandler seek failed: unrecognized whence");
    }
}

