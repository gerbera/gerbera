/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    fd_io_handler.cc - this file is part of MediaTomb.
    
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

/// \file fd_io_handler.cc

#include "fd_io_handler.h"
#include "common.h"
#include "server.h"
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

using namespace zmm;
using namespace mxml;

FDIOHandler::FDIOHandler(String filename)
    : IOHandler()
{
    this->filename = filename;
    this->fd = -1;
    this->other = nullptr;
    this->reference_list = Ref<Array<Object>>(new Array<Object>(4));
    this->closed = false;
}

FDIOHandler::FDIOHandler(int fd)
    : IOHandler()
{
    this->filename = nullptr;
    this->fd = fd;
    this->other = nullptr;
    this->reference_list = Ref<Array<Object>>(new Array<Object>(4));
    this->closed = false;
}

void FDIOHandler::addReference(Ref<Object> reference)
{
    reference_list->append(reference);
}

void FDIOHandler::closeOther(Ref<IOHandler> other)
{
    this->other = other;
}

void FDIOHandler::open(IN enum UpnpOpenFileMode mode)
{

    if (fd != -1) {
        log_debug("Assuming valid fd %d\n", fd);
        return;
    }

    if (!string_ok(filename))
        throw _Exception(_("Missing filename!"));

    if (mode == UPNP_READ) {
        fd = ::open(filename.c_str(), O_RDONLY);
    } else if (mode == UPNP_WRITE) {
        fd = ::open(filename.c_str(), O_WRONLY | O_CREAT, S_IWUSR | S_IRUSR);
    } else {
        throw _Exception(_("FDIOHandler::open: invdalid read/write mode"));
    }

    if (fd == -1) {
        throw _Exception(_("FDIOHandler::open: failed to open: ") + filename.c_str());
    }
}

size_t FDIOHandler::read(OUT char* buf, IN size_t length)
{
    size_t ret = 0;

    ret = ::read(fd, buf, length);

    return ret;
}

size_t FDIOHandler::write(IN char* buf, IN size_t length)
{
    size_t ret = 0;

    ret = ::write(fd, buf, length);

    return ret;
}

void FDIOHandler::seek(IN off_t offset, IN int whence)
{
    if (lseek(fd, offset, whence) != 0) {
        throw _Exception(_("fseek failed"));
    }
}

void FDIOHandler::close()
{

    if (closed)
        return;

    log_debug("Closing...\n");
    try {
        if (other != nullptr)
            other->close();
    } catch (const Exception& ex) {
        log_debug("Error closing \"other\" handler: %s\n", ex.getMessage().c_str());
    }

    // protect from multiple close calls
    if (::close(fd) != 0) {
        throw _Exception(_("fclose failed"));
    }
    fd = -1;
    closed = true;
}
