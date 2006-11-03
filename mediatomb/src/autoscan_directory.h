/*  autoscan_directory.h - this file is part of MediaTomb.
                                                                                
    Copyright (C) 2005 Gena Batyan <bgeradz@deadlock.dhs.org>,
                       Sergey Bostandzhyan <jin@deadlock.dhs.org>
                                                                                
    MediaTomb is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
                                                                                
    MediaTomb is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
                                                                                
    You should have received a copy of the GNU General Public License
    along with MediaTomb; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

///\file autoscan_directory.h
///\brief Definitions of the AutoscanDirectory class. 

#ifndef __AUTOSCAN_DIRECTORY_H__
#define __AUTOSCAN_DIRECTORY_H__

#include "zmmf/zmmf.h"

///\brief Scan level - the way how exactly directories should be scanned.
typedef enum scan_level_t
{
    BasicScan, // file was added or removed from the directory
    FullScan   // file was modified/added/removed
};

///\brief Scan mode - type of scan (timed, inotify, fam, etc.)
typedef enum scan_mode_t
{
    Timed
}

/// \brief Provides information about one autoscan directory.
class AutoscanDirectory : public zmm::Object
{
public:
    /// \brief Creates a new AutoscanDirectory object.
    /// \param location autoscan path
    /// \param mode scan mode
    /// \param level scan level
    /// \param recursive process directories recursively
    /// \param interval rescan interval in seconds (only for timed scan mode),
    /// zero means none.
    AutoscanDirectory(zmm::String location, scan_mode_t mode, 
                      scan_level_t level, bool recursive, 
                      unsigned int interval=0);

    zmm::String getLocation() { return location; }

    scan_mode_t getScanMode() { return mode; }

    void setScanMode(scan_mode_t mode) { this->mode = mode; }

    scan_level_t getScanLevel() { return level; }

    void setScanLevel(scan_level_t level) { this->level = level; }

    bool getRecursive() { return recursive; }

    void setRecursive(bool recursive) { this->recursive = recursive; }
    
    unsigned int getInterval() { return interval; }

    void setInterval(unsigned int interval) { this->interval = interval; }

protected:
    zmm::String location;
    scan_mode_t mode;
    scan_level_t level;
    bool recursive;
    unsigned int interval;
    
};

#endif

