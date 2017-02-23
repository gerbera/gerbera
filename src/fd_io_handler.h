/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    fd_io_handler.h - this file is part of MediaTomb.
    
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

/// \file fd_io_handler.h
/// \brief Definition of the FDIOHandler class.
#ifndef __FD_IO_HANDLER_H__
#define __FD_IO_HANDLER_H__

#include "common.h"
#include "io_handler.h"

/// \brief Allows the web server to read from a file, operation is blocking.
class FDIOHandler : public IOHandler
{
protected:
    /// \brief Name of the file.
    zmm::String filename;

    /// \brief Handle of the file.
    int fd;
    bool closed;
    zmm::Ref<IOHandler> other;
    zmm::Ref<zmm::Array<zmm::Object> > reference_list;
public:
    /// \brief Sets the filename to work with.
    FDIOHandler(zmm::String filename);

    /// \brief Sets an aleady opened file descriptor to work with, call to
    /// open() will be ignored.
    FDIOHandler(int fd);

    void addReference(zmm::Ref<zmm::Object> reference);
    void closeOther(zmm::Ref<IOHandler> other);

    /// \brief Opens file for reading (writing is not supported)
    virtual void open(IN enum UpnpOpenFileMode mode);

    /// \brief Reads a previously opened file sequentially.
    /// \param buf Data from the file will be copied into this buffer.
    /// \param length Number of bytes to be copied into the buffer.
    virtual int read(OUT char *buf, IN size_t length);
   
    /// \brief Writes to a previously opened file.
    /// \param buf Data from the buffer will be written to the file.
    /// \param length Number of bytes to be written from the buffer.
    /// \return number of bytes written.
    virtual int write(OUT char *buf, IN size_t length);

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


#endif // __FD_IO_HANDLER_H__
