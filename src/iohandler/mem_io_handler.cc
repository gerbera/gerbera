/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    mem_io_handler.cc - this file is part of MediaTomb.
    
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

/// \file mem_io_handler.cc

#include "mem_io_handler.h" // API

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>

#include <sys/stat.h>
#include <unistd.h>

MemIOHandler::MemIOHandler(const void* buffer, int length)
    : buffer(new char[length])
    , length(length)
    , pos(-1)
{
    memcpy(this->buffer, buffer, length);
}

MemIOHandler::MemIOHandler(const std::string& str)
    : buffer(new char[str.length()])
    , length(str.length())
    , pos(-1)
{
    memcpy(this->buffer, str.c_str(), length);
}

MemIOHandler::~MemIOHandler()
{
    delete[] buffer;
}

void MemIOHandler::open(enum UpnpOpenFileMode mode)
{
    pos = 0;
}

size_t MemIOHandler::read(char* buf, size_t length)
{
    size_t ret = 0;

    // we indicate EOF by setting pos to -1
    if (pos == -1) {
        return 0;
    }

    off_t rest = this->length - pos;
    if (length > size_t(rest))
        length = rest;

    memcpy(buf, buffer + pos, length);
    pos = pos + length;
    ret = int(length);

    if (pos >= this->length) {
        pos = -1;
    }

    return ret;
}

void MemIOHandler::seek(off_t offset, int whence)
{
    if (whence == SEEK_SET) {
        // offset must be positive when SEEK_SET is used
        if (offset < 0) {
            throw_std_runtime_error("seek failed: SEEK_SET used with negative offset");
        }

        if (offset > length) {
            throw_std_runtime_error("seek failed: trying to seek past the end of file");
        }

        pos = offset;
    } else if (whence == SEEK_CUR) {
        off_t temp = (pos == -1) ? length : pos;
        //  (((temp + offset) > length) || ((temp + offset) < 0))
        if (offset > 0 && (length < offset || temp > length - offset)) {
            throw_std_runtime_error("seek failed: trying to seek past end of file");
        }

        if (offset < 0 && temp < -offset) {
            throw_std_runtime_error("seek failed: trying to seek before the beginning of file");
        }

        pos = temp + offset;
    } else if (whence == SEEK_END) {
        off_t temp = length;
        //  (((temp + offset) > length) || ((temp + offset) < 0))
        if (offset > 0 && (length < offset || temp > length - offset)) {
            throw_std_runtime_error("seek failed: trying to seek past end of file");
        }

        if (offset < 0 && temp < -offset) {
            throw_std_runtime_error("seek failed: trying to seek before the beginning of file");
        }

        pos = temp + offset;
    } else {
        throw_std_runtime_error("seek failed: unrecognized whence");
    }
}

off_t MemIOHandler::tell()
{
    return pos;
}

void MemIOHandler::close()
{
    pos = -1;
}
