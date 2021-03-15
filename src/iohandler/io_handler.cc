/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    io_handler.cc - this file is part of MediaTomb.
    
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

/// \file io_handler.cc

#include "io_handler.h" // API

#include <unistd.h>

/// \fn static UpnpWebFileHandle web_open(const char *filename,
///                                       enum UpnpOpenFileMode mode)
/// \brief Opens a file for the web server.
/// \param filename Name of the file to open.
/// \param mode in which the file will be opened (we only support UPNP_READ)
///
/// This function is called by the web server when it needs to open a file.
///
/// \retval UpnpWebFileHandle A valid file handle.
/// \retval NULL Error.
void IOHandler::open(enum UpnpOpenFileMode mode)
{
}

/// \fn static int web_read (UpnpWebFileHandle f, char *buf,
///                          size_t length)
/// \brief Reads a previously opened file sequentially.
/// \param f Handle of the file.
/// \param buf  This buffer will be filled by fread.
/// \param length Number of bytes to read.
///
/// This function is called by the web server to perform a sequential
/// read from an open file. It copies \b length bytes from the file
/// into the buffer.
///
/// \retval 0   EOF encountered.
/// \retval -1  Error.
size_t IOHandler::read(char* buf, size_t length)
{
    return -1;
}

/// \fn static int web_write (UpnpWebFileHandle f,char *buf,
///                           size_t length)
/// \brief Writes to a previously opened file sequentially.
/// \param f Handle of the file.
/// \param buf This buffer will be filled by fwrite.
/// \param length Number of bytes to fwrite.
///
/// This function is called by the web server to perform a sequential
/// write to an open file. It copies \b length bytes into the file
/// from the buffer. It should return the actual number of bytes
/// written, in case of a write error this might be less than
/// \b length.
///
/// \retval Actual number of bytes written.
///
/// \warning Currently this function is not supported.
size_t IOHandler::write(char* buf, size_t length)
{
    return 0;
}

/// \fn static int web_seek (UpnpWebFileHandle f, long offset,
///                   int origin)
/// \brief Performs a seek on an open file.
/// \param f Handle of the file.
/// \param offset Number of bytes to move in the file. For seeking forwards
/// positive values are used, for seeking backwards - negative. \b Offset must
/// be positive if \b origin is set to \b SEEK_SET
/// \param whence The position to move relative to. SEEK_CUR to move relative
/// to current position, SEEK_END to move relative to the end of file,
/// SEEK_SET to specify an absolute offset.
///
/// This function is called by the web server to perform seek on an a file.
///
/// \retval 0 On success, non-zero value on error.
void IOHandler::seek(off_t offset, int whence)
{
}

/// \brief Return the current stream position.
off_t IOHandler::tell()
{
    return -1;
}

/// \fn static int web_close (UpnpWebFileHandle f)
/// \brief Closes a previously opened file.
/// \param f Handle of the file.
///
/// Same as fclose()
///
/// \retval 0 On success, non-zero on error.
void IOHandler::close()
{
}
