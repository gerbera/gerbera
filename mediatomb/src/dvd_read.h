/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    file_io_handler.h - this file is part of MediaTomb.
    
    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>
    
    Copyright (C) 2006-2008 Gena Batyan <bgeradz@mediatomb.cc>,
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
    
    $Id: file_io_handler.h 1698 2008-02-23 20:48:30Z lww $
*/

/// \file file_io_handler.h
/// \brief Definition of the FileIOHandler class.
#ifndef __DVD_READ_H__
#define __DVD_READ_H__

#ifdef HAVE_INTTYPES_H
    #include <inttypes.h>
#else
    #include <stdint.h>
#endif

#include <dvdread/dvd_reader.h>
#include <dvdread/ifo_read.h>

#include "common.h"

/// \brief Allows the web server to read from a file.
class DVDReader : public zmm::Object
{
public:
    /// \brief Sets the filename to work with. Can be an ISO, device or
    /// directory.
    DVDReader(zmm::String path);
    ~DVDReader();

    /// \brief returns the number of titles on the DVD
    int titleCount();

    /// \brief returns the number of chapters for a given title
    int chapterCount(int titleID);

    void dumpVMG();

    void selectPGC(int title, int chapter, int angle);

protected:
    /// \brief Name of the DVD file.
    zmm::String dvd_path;

    dvd_reader_t *dvd;
    dvd_file_t   *title;

    // video title set
    ifo_handle_t *vts;
    // video manager
    ifo_handle_t *vmg;
    // program chain
    pgc_t   *current_pgc;

    int title_block_start;
    int title_block_end;
    int title_blocks;
    int title_offset;

    int title_cell_start;
    int title_cell_end;
    int current_cell;
    int next_cell;

};


#endif // __DVD_READ_H__
