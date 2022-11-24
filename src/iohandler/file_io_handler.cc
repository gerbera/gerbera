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
#define LOG_FAC log_facility_t::iohandler

#include "file_io_handler.h" // API

FileIOHandler::FileIOHandler(const fs::path& filename)
    : file(filename)
{
}

void FileIOHandler::open(enum UpnpOpenFileMode mode)
{
    if (mode != UPNP_READ)
        throw_std_runtime_error("open: UpnpOpenFileMode mode not supported");

    f = file.open("rb");
}

std::size_t FileIOHandler::read(std::byte* buf, std::size_t length)
{
    std::size_t ret = std::fread(buf, sizeof(std::byte), length, f);

    if (ret == 0) {
        if (std::feof(f))
            return 0;
        if (std::ferror(f))
            return -1;
    }

    return ret;
}

std::size_t FileIOHandler::write(std::byte* buf, std::size_t length)
{
    return std::fwrite(buf, sizeof(std::byte), length, f);
}

void FileIOHandler::seek(off_t offset, int whence)
{
    if (fseeko(f, offset, whence) != 0) {
        throw_std_runtime_error("fseek failed");
    }
}

off_t FileIOHandler::tell()
{
    return ftello(f);
}

void FileIOHandler::close()
{
}
