/*MT*
    
    MediaTomb - http://www.mediatomb.org/
    
    file_io_handler.h - this file is part of MediaTomb.
    
    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.org>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.org>
    
    Copyright (C) 2006-2007 Gena Batyan <bgeradz@mediatomb.org>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.org>,
                            Leonhard Wimmer <leo@mediatomb.org>
    
    MediaTomb is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.
    
    MediaTomb is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    version 2 along with MediaTomb; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
    
    $Id$
*/

/// \file file_io_handler.h
/// \brief Definition of the FileIOHandler class.
#ifndef __FILE_IO_HANDLER_H__
#define __FILE_IO_HANDLER_H__

#include "common.h"
#include "io_handler.h"

/// \brief Allows the web server to read from a file.
class FileIOHandler : public IOHandler
{
protected:
    /// \brief Name of the file.
    zmm::String filename;

    /// \brief Handle of the file.
    FILE *f;
public:
    /// \brief Sets the filename to work with.
    FileIOHandler(zmm::String filename);

    /// \brief Opens file for reading (writing is not supported)
    virtual void open(IN enum UpnpOpenFileMode mode);

    /// \brief Reads a previously opened file sequentially.
    /// \param buf Data from the file will be copied into this buffer.
    /// \param length Number of bytes to be copied into the buffer.
    virtual int read(OUT char *buf, IN size_t length);

    /// \brief Performs seek on an open file.
    /// \param offset Number of bytes to move in the file. For seeking forwards
    /// positive values are used, for seeking backwards - negative. Offset must
    /// be positive if origin is set to SEEK_SET
    /// \param whence The position to move relative to. SEEK_CUR to move relative
    /// to current position, SEEK_END to move relative to the end of file,
    /// SEEK_SET to specify an absolute offset.
    virtual void seek(IN off_t offset, IN int whence);

    /// \brief Close a previously opened file.
    virtual void close();
};


#endif // __FILE_IO_HANDLER_H__

