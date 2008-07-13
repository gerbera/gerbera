/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    dvd_read.h - this file is part of MediaTomb.
    
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

/// \file dvd_read.h
/// \brief Definition of the FileIOHandler class.
#ifndef __DVD_READ_H__
#define __DVD_READ_H__

#ifdef HAVE_INTTYPES_H
    #include <inttypes.h>
#else
    #include <stdint.h>
#endif

#include <sys/types.h>

#include <dvdread/dvd_reader.h>
#include <dvdread/ifo_read.h>
#include <dvdread/nav_types.h>

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
    /// \param title_idx index of the title
    int chapterCount(int title_idx);

    /// \brief returns the number of angles for a given title
    int angleCount(int title_idx);

    void dumpVMG();

    /// \brief Selects the title, chapter and angle to play, returns the
    /// number of bytes for the selection.
    ///
    /// The DVD is divided into titles, each title is didvided into chapters
    /// and can have one or more angles. This function selects what data we
    /// want to retrieve from the DVD (i.e. what stream we want to watch),
    /// and returns the size of the stream in bytes. 
    /// Note, that we will always treat the chapter as "starting point", i.e.
    /// we will play from the given chapter to the very end, we will not stop
    /// at the end of the chapter.
    ///
    /// \param title_idx index of the title
    /// \param chapter_idx index of the chapter from where we start
    /// \param angle_idx index of the angle
    /// \return number of bytes in the stream
    off_t selectPGC(int title_idx, int chapter_idx, int angle_idx);

    /// \brief Reads the stream, specified by selectPGC from the DVD.
    ///
    /// \param buffer buffer to store the data
    /// \param length length of the buffer
    /// \return number of bytes read (can be shorter than buffer length), value
    ///  of 0 indicates end of stream.
    size_t readSector(unsigned char *buffer, size_t length);  

protected:
    /// \brief Name of the DVD file.
    zmm::String dvd_path;

    /// \brief DVD handle
    dvd_reader_t *dvd;

    /// \brief Video Manager handle
    ifo_handle_t *vmg;

    /// \brief Currently selected Program Chain
    pgc_t *pgc;

    /// \brief Currently selected Video Title Set
    ifo_handle_t *vts;

    /// \brief Currently selected Title
    dvd_file_t *title;

    /// \brief Sets the global variables to the next cell.
    int nextCell();

    int title_index;
    int chapter_index;
    int angle_index;
    int start_cell;
    int next_cell;
    int current_cell;
    int current_pack;

    bool cont_sector;

    bool initialized;

    // libdvdread has a bunch of weird variable names in the structs which tend
    // to scare you away, most of them are in ifo_types.h 
    //
    // Know your enemy:
    //
    // VOB          Video Object
    // VTS          Video Title Set
    // VMG          Video Manager
    // VMGI         Video Manager Information
    // VMGM         Video Manager Menu
    // VMGI_MAT     Video Manager Information Management Table
    // TT_SRPT      Title Search Pointer Table
    // VMGM_PGCI_UT Video Manager Menu Program Chain Information Unit Table
    // VTS_PTT_SRPT Video Title Set "Part Of Title" Search Pointer Table
    // VTS_PGCIT    Video Title Set Program Chain Information Table
    // VTS_ATRT     Video Title Set Attribute Table
    // VTS_TTN      Video Title Set Title Number
    // PGC          Program Chain
    // PGCN         Program Chain Number
    // PGCI         Program Chain Information
    // PGN          Program Number
    // PTT          Part Of Title
    // PTTN         Part Of Title Number
    // TTN          Title Number
    // ILVU         Interleaved Unit
    // SML_AGLI     Angle Information For Seamless
    // DSI          Data Search Information
    // DSI_GI       DSI General Information
};


#endif // __DVD_READ_H__
