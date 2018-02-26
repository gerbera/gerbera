/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    file_io_handler.cc - this file is part of MediaTomb.
    
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

/// \file file_io_handler.cc

#include <cstdio>
#include <ixml.h>

#include "cds_objects.h"
#include "file_io_handler.h"
#include "server.h"

using namespace zmm;
using namespace mxml;

FileIOHandler::FileIOHandler(String filename)
    : filename(filename)
    , f(nullptr)
{
}

void FileIOHandler::open(IN enum UpnpOpenFileMode mode)
{
    if (mode == UPNP_READ) {
        f = fopen(filename.c_str(), "rb");
    } else if (mode == UPNP_WRITE) {
        throw _Exception(_("FileIOHandler::open: Write mode not supported"));
        ;
    } else {
        throw _Exception(_("FileIOHandler::open: invalid UpnpOpenFileMode mode"));
    }

    if (f == nullptr) {
        throw _Exception(_("FileIOHandler::open: failed to open: ") + filename.c_str());
    }
}

size_t FileIOHandler::read(OUT char* buf, IN size_t length)
{
    size_t ret = 0;

    ret = fread(buf, sizeof(char), length, f);

    if (ret <= 0) {
        if (feof(f))
            return 0;
        if (ferror(f))
            return -1;
    }

    return ret;
}

size_t FileIOHandler::write(IN char* buf, IN size_t length)
{
    size_t ret = 0;

    ret = fwrite(buf, sizeof(char), length, f);

    return ret;
}

void FileIOHandler::seek(IN off_t offset, IN int whence)
{
    if (fseeko(f, offset, whence) != 0) {
        throw _Exception(_("fseek failed"));
    }
}

void FileIOHandler::close()
{
    if (fclose(f) != 0) {
        throw _Exception(_("fclose failed"));
    }
    f = nullptr;
}
