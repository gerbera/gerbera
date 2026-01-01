/*MT*

    MediaTomb - http://www.mediatomb.cc/

    io_handler.cc - this file is part of MediaTomb.

    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>

    Copyright (C) 2006-2010 Gena Batyan <bgeradz@mediatomb.cc>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>,
                            Leonhard Wimmer <leo@mediatomb.cc>

    Copyright (C) 2016-2026 Gerbera Contributors

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

/// @file iohandler/io_handler.cc
#define GRB_LOG_FAC GrbLogFacility::iohandler

#include "io_handler.h" // API

/// @brief Opens a file for the web server.
/// @param mode in which the data will be opened (we only support UPNP_READ)
void IOHandler::open(enum UpnpOpenFileMode mode)
{
}

/// @brief Reads a previously opened file sequentially.
/// @param buf  This buffer will be filled by fread.
/// @param length Number of bytes to read.
///
/// This function is called by the web server to perform a sequential
/// read from an open file. It copies \b length bytes from the file
/// into the buffer.
///
/// \retval 0   EOF encountered.
/// \retval -1  Error.
grb_read_t IOHandler::read(std::byte* buf, std::size_t length)
{
    return -1;
}

/// @brief Writes to a previously opened file sequentially.
/// @param buf This buffer will be filled by fwrite.
/// @param length Number of bytes to fwrite.
///
/// This function is called by the web server to perform a sequential
/// write to an open file. It copies \b length bytes into the file
/// from the buffer. It should return the actual number of bytes
/// written, in case of a write error this might be less than
/// \b length.
///
/// \retval Actual number of bytes written.
std::size_t IOHandler::write(std::byte* buf, std::size_t length)
{
    return 0;
}

/// @brief Performs a seek on an open file.
/// @param offset Number of bytes to move in the file. For seeking forwards
/// positive values are used, for seeking backwards - negative. \b Offset must
/// be positive if \b origin is set to \b SEEK_SET
/// @param whence The position to move relative to. SEEK_CUR to move relative
/// to current position, SEEK_END to move relative to the end of file,
/// SEEK_SET to specify an absolute offset.
///
/// This function is called by the web server to perform seek on an a file.
void IOHandler::seek(off_t offset, int whence)
{
}

/// @brief Return the current stream position.
off_t IOHandler::tell()
{
    return -1;
}

/// @brief Closes a previously opened file.
///
/// Same as fclose()
void IOHandler::close()
{
}
