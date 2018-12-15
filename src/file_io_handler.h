/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    file_io_handler.h - this file is part of MediaTomb.
    
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

/// \file file_io_handler.h
/// \brief Definition of the FileIOHandler class.
#ifndef __FILE_IO_HANDLER_H__
#define __FILE_IO_HANDLER_H__

#include "common.h"
#include "io_handler.h"

/// \brief Allows the web server to read from a file.
class FileIOHandler : public IOHandler {
protected:
    /// \brief Name of the file.
    zmm::String filename;

    /// \brief Handle of the file.
    FILE* f;

public:
    /// \brief Sets the filename to work with.
    explicit FileIOHandler(zmm::String filename);

    /// \brief Opens file for reading (writing is not supported)
    void open(IN enum UpnpOpenFileMode mode) override;

    /// \brief Reads a previously opened file sequentially.
    /// \param buf Data from the file will be copied into this buffer.
    /// \param length Number of bytes to be copied into the buffer.
    size_t read(OUT char* buf, IN size_t length) override;

    /// \brief Writes to a previously opened file.
    /// \param buf Data from the buffer will be written to the file.
    /// \param length Number of bytes to be written from the buffer.
    /// \return number of bytes written.
    size_t write(OUT char* buf, IN size_t length) override;

    /// \brief Performs seek on an open file.
    /// \param offset Number of bytes to move in the file. For seeking forwards
    /// positive values are used, for seeking backwards - negative. Offset must
    /// be positive if origin is set to SEEK_SET
    /// \param whence The position to move relative to. SEEK_CUR to move relative
    /// to current position, SEEK_END to move relative to the end of file,
    /// SEEK_SET to specify an absolute offset.
    void seek(IN off_t offset, IN int whence) override;

    /// \brief Close a previously opened file.
    void close() override;
};

#endif // __FILE_IO_HANDLER_H__
