/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    dvdnav_read.h - this file is part of MediaTomb.
    
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

/// \file dvdnav_read.h
/// \brief Definition of the FileIOHandler class.
#ifndef __DVDNAV_READ_H__
#define __DVDNAV_READ_H__

#include <stdint.h>

#include <sys/types.h>
#include <dvdnav/dvdnav.h>
#include "common.h"
#include "sync.h"

/// \brief Allows to read selected streams from a DVD image.
///
/// There are some constraints on the usage of this class:
/// First of all - you *must* call the selectPGC() function before you 
/// attempt to read data or get the stream length.
///
/// You must call selectPGC() each time when you want to reset read; calling
/// read consequently will return the stream data and advance further in the
/// stream.
/// 
/// The class is thread safe, meaning that locks are in place, however it's
/// design does not suggest multithreaded usage. selectPGC(), read(), 
/// getLength() will not work in parallel but block if one of the functions
/// is running.
class DVDNavReader : public zmm::Object
{
public:
    /// \brief Sets the filename to work with. Can be an ISO, device or
    /// directory.
    DVDNavReader(zmm::String path);
    ~DVDNavReader();

    /// \brief returns the number of titles on the DVD
    int titleCount();

    /// \brief returns the number of chapters for a given title
    /// \param title_idx index of the title
    int chapterCount(int title_idx);

    /// \brief Returns the number of audio streams for the selected PGC
    int audioTrackCount();

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
    void selectPGC(int title_idx, int chapter_idx);

    /// \brief Reads the stream, specified by selectPGC from the DVD.
    ///
    /// \param buffer buffer to store the data
    /// \param length length of the buffer
    /// \return number of bytes read (can be shorter than buffer length), value
    ///  of 0 indicates end of stream.
    size_t readSector(unsigned char *buffer, size_t length);  

    /// \brief Returns a human readable language string of the audio stream
    zmm::String audioLanguage(int stream_idx);

    /// \brief Returns the sampling frequency of the given audio stream
    int audioSampleFrequency(int stream_idx);

    /// \brief Returns the number of channels for the given audio stream
    int audioChannels(int stream_idx);

    /// \brief Returns the id of the audio stream
    int audioStreamID(int stream_idx);

    /// \brief Returns a human readable name of the audio format
    zmm::String audioFormat(int stream_idx);

protected:
    /// \brief Name of the DVD file.
    zmm::String dvd_path;

    /// \brief DVD handle
    dvdnav_t *dvd;

    /// \brief end of title flag
    bool EOT;

    zmm::Ref<Mutex> mutex;

    zmm::String getLanguage(char *code);
};


#endif // __DVDNAV_READ_H__
