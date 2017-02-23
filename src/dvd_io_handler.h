/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    dvd_io_handler.h - this file is part of MediaTomb.
    
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

/// \file dvd_io_handler.h

/// \brief Definition of the DVDIOHandler class.
#ifndef __DVD_IO_HANDLER_H__
#define __DVD_IO_HANDLER_H__

#include "common.h"
#include "io_handler.h"
#include "dvdnav_read.h"

/// \brief Allows the web server to read from a dvd.
class DVDIOHandler : public IOHandler
{
protected:
    /// \brief Name of the DVD image.
    zmm::String dvdname;

    /// \brief Handle of the DVD.
    zmm::Ref<DVDNavReader> dvd;

    off_t total_size;

    unsigned char *small_buffer;
    unsigned char *small_buffer_pos;
    bool last_read;

public:
    /// \brief Sets the dvdname to work with.
    DVDIOHandler(zmm::String dvdname, int track, int chapter, 
                 int audio_stream_id);

    ~DVDIOHandler();
    /// \brief Opens dvd for reading (writing is not supported)
    virtual void open(IN enum UpnpOpenFileMode mode);

    /// \brief Reads a previously opened dvd sequentially.
    /// \param buf Data from the dvd will be copied into this buffer.
    /// \param length Number of bytes to be copied into the buffer.
    virtual int read(OUT char *buf, IN size_t length);
   
    /// \brief Writes to a previously opened dvd.
    /// \param buf Data from the buffer will be written to the dvd.
    /// \param length Number of bytes to be written from the buffer.
    /// \return number of bytes written.
    virtual int write(OUT char *buf, IN size_t length);

    /// \brief Performs seek on an open dvd.
    /// \param offset Number of bytes to move in the dvd. For seeking forwards
    /// positive values are used, for seeking backwards - negative. Offset must
    /// be positive if origin is set to SEEK_SET
    /// \param whence The position to move relative to. SEEK_CUR to move relative
    /// to current position, SEEK_END to move relative to the end of dvd,
    /// SEEK_SET to specify an absolute offset.
    virtual void seek(IN off_t offset, IN int whence);

    /// \brief Close a previously opened dvd.
    virtual void close();

    /// \brief Returns the length of the stream in bytes
    off_t length();
};


#endif // __DVD_IO_HANDLER_H__
