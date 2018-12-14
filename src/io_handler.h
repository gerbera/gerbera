/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    io_handler.h - this file is part of MediaTomb.
    
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

/// \file io_handler.h
/// \brief Definitoin for the IOHandler class.
#ifndef __IO_HANDLER_H__
#define __IO_HANDLER_H__

#include <upnp.h>

#include "common.h"

class IOHandler : public zmm::Object {
public:
    IOHandler();

    /// \brief Opens a data for the web server.
    /// \param mode in which the data will be opened (we only support UPNP_READ)
    /// \todo Genych, ya tut che to zapamyatowal kak gawno rabotaet? kto filename poluchaet??
    virtual void open(enum UpnpOpenFileMode mode);

    /// \brief Reads previously opened/initialized data sequentially.
    /// \param buf This buffer will be filled by our read functions.
    /// \param length Number of bytes to read.
    virtual size_t read(char* buf, size_t length);

    /// \brief Writes to previously opened/initialized data sequentially.
    /// \param buf Data to be written.
    /// \param length Number of bytes to write.
    virtual size_t write(char* buf, size_t length);

    /// \brief Performs a seek on an open/initialized data.
    /// \param offset Number of bytes to move in the buffer.

    /// For seeking forwards
    /// positive values are used, for seeking backwards - negative. \b Offset must
    /// be positive if \b origin is set to \b SEEK_SET
    /// \param whence The position to move relative to. SEEK_CUR to move relative
    /// to current position, SEEK_END to move relative to the end of file,
    /// SEEK_SET to specify an absolute offset.
    virtual void seek(off_t offset, int whence);

    /// \brief Close/free previously opened/initialized data.
    virtual void close();
};

#endif // __IO_HANDLER_H__
