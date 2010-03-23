/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    autoscan.h - this file is part of MediaTomb.
    
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

/// \file autoscan.h
///\brief Definitions of the Autoscan classes. 

#ifndef __AUTOSCAN_H__
#define __AUTOSCAN_H__

#include "zmmf/zmmf.h"
#include "sync.h"
#include "timer.h"

#define INVALID_SCAN_ID -1

///\brief Scan level - the way how exactly directories should be scanned.
enum scan_level_t
{
    BasicScanLevel, // file was added or removed from the directory
    FullScanLevel   // file was modified/added/removed
};

///\brief Scan mode - type of scan (timed, inotify, fam, etc.)
enum scan_mode_t
{
    TimedScanMode,
    InotifyScanMode
};

class AutoscanDirectory;

class AutoscanList : public zmm::Object
{
public:
    AutoscanList();
    
    /// \brief Adds a new AutoscanDirectory to the list. 
    /// 
    /// The scanID of the directory is invalidated and set to 
    /// the index in the AutoscanList.
    /// 
    /// \param dir AutoscanDirectory to add to the list.
    /// \return scanID of the newly added AutoscanDirectory
    int add(zmm::Ref<AutoscanDirectory> dir);

    void addList(zmm::Ref<AutoscanList> list);

    zmm::Ref<AutoscanDirectory> get(int id);

    zmm::Ref<AutoscanDirectory> get(zmm::String location);
    
    zmm::Ref<AutoscanDirectory> getByObjectID(int objectID);

    int size() { return list->size(); }
   
    /// \brief removes the AutoscanDirectory given by its scan ID
    void remove(int id);

    int removeByObjectID(int objectID);
    
    /// \brief removes the AutoscanDirectory with the given location
    /// \param location the location to remove
    /// \return the scanID, that was removed; if nothing removed: INVALID_SCAN_ID
    int remove(zmm::String location);

    /// \brief removes the AutoscanDirectory if it is a subdirectory of a given location.
    /// \param parent parent directory.
    /// \param persistent also remove persistent directories.
    /// \return AutoscanList of removed directories, where each directory object in the list is a copy and not the original reference.
    zmm::Ref<AutoscanList> removeIfSubdir(zmm::String parent, bool persistent = false);

    /*
    /// \brief Add timer for each directory in the list.
    /// \param obj instance of the class that will receive notifications.
    void subscribeAll(zmm::Ref<TimerSubscriber> obj);
    */

    /// \brief Send notification for each directory that is stored in the list.
    /// 
    /// \param obj instance of the class that will receive the notifications.
    void notifyAll(zmm::Ref<TimerSubscriberSingleton<Object> > obj);
        
    /*
    /// \brief Add timer for given directory.
    /// \param obj instance of the class that will receive notifications.
    /// \param id dir id.
    void subscribeDir(zmm::Ref<TimerSubscriber> obj, int id, bool once = true);
    */
    
    /// \brief updates the last_modified data for all AutoscanDirectories.
    void updateLMinDB();

    /// \brief returns a copy of the autoscan list in the form of an array
    zmm::Ref<zmm::Array<AutoscanDirectory> > getArrayCopy();

/*
    void dump();
*/

protected:
    zmm::Ref<Mutex> mutex;
    zmm::Ref<zmm::Array<AutoscanDirectory> > list;
    int _add(zmm::Ref<AutoscanDirectory> dir);
};

/// \brief Provides information about one autoscan directory.
class AutoscanDirectory : public zmm::Object
{
public:
    AutoscanDirectory();

    /// \brief Creates a new AutoscanDirectory object.
    /// \param location autoscan path
    /// \param mode scan mode
    /// \param level scan level
    /// \param recursive process directories recursively
    /// \param interval rescan interval in seconds (only for timed scan mode)
    /// \param hidden include hidden files
    /// zero means none.
    AutoscanDirectory(zmm::String location, scan_mode_t mode, 
                      scan_level_t level, bool recursive,
                      bool persistent,
                      int id = INVALID_SCAN_ID, unsigned int interval = 0, bool hidden = false);

    void setStorageID(int storageID) { this->storageID = storageID; }

    int getStorageID() { return storageID; }

    /// \brief The location can only be set once! 
    void setLocation(zmm::String location);

    zmm::String getLocation() { return location; }

    scan_mode_t getScanMode() { return mode; }

    void setScanMode(scan_mode_t mode) { this->mode = mode; }

    scan_level_t getScanLevel() { return level; }

    void setScanLevel(scan_level_t level) { this->level = level; }

    bool getRecursive() { return recursive; }
   
    void setHidden(bool hidden) { this->hidden = hidden; }

    bool getHidden() { return hidden; }

    void setRecursive(bool recursive) { this->recursive = recursive; }
    
    unsigned int getInterval() { return interval; }

    void setInterval(unsigned int interval) { this->interval = interval; }

    /// \brief Increments the task count. 
    ///
    /// When recursive autoscan is in progress, we only want to subcribe to
    /// a timer event when the scan is finished. However, recursive scans
    /// spawn tasks for each directory. When adding a rescan task for 
    /// subdirectories, the taskCount will be incremented. When a task is
    /// done the count will be decremented. When timer gets to zero, 
    /// we will resubscribe.
    void incTaskCount() { taskCount++; }
    
    void decTaskCount() { taskCount--; }

    int getTaskCount() { return taskCount; }

    void setTaskCount(int taskCount) { this->taskCount = taskCount; }
  
    /// \brief Sets the task ID.
    ///
    /// The task ID helps us to identify to which scan a particular task
    /// belongs. Recursive scans spawn new tasks - they all should have
    /// the same id.
    void setScanID(int id);

    int getScanID() { return scanID; }

    void setObjectID(int id) { objectID = id; }

    int getObjectID() { return objectID; }

    bool persistent() { return persistent_flag; }

    /// \brief Sets the last modification time of the current ongoing scan.
    /// 
    /// When doing a FullScan we look at modification times of the files.
    /// During the recursion of one AutoscanDirectory (which will be the
    /// starting point and which may have subcontainers) we must compare
    /// the last modification time of the starting point but we may not
    /// overwrite it until we are done.
    /// The time will be only set if it is higher than the previous value!
    void setCurrentLMT(time_t lmt);
    
    time_t getPreviousLMT() { return last_mod_previous_scan; }
    
    void updateLMT() { last_mod_previous_scan = last_mod_current_scan; }

    void resetLMT() { last_mod_previous_scan = 0; last_mod_current_scan = 0; }
 
    /// \brief copies all properties to another object
    void copyTo(zmm::Ref<AutoscanDirectory> copy);

    /// \brief Set the parameter for timer notify that is associated with
    /// the particular autoscan directory.
//    void setTimerParamter(zmm::Ref<zmm::Object> parameter);

    /// \brief Get the timer notify parameter associated with this directory.
    zmm::Ref<zmm::Object> getTimerParameter();

//    bool equals(Ref<AutoscanDirectory> dir);
   
    /* helpers for autoscan stuff */
    static zmm::String mapScanmode(scan_mode_t scanmode);
    static scan_mode_t remapScanmode(zmm::String scanmode);
    static zmm::String mapScanlevel(scan_level_t scanlevel);
    static scan_level_t remapScanlevel(zmm::String scanlevel);
    
protected:
    zmm::String location;
    scan_mode_t mode;
    scan_level_t level;
    bool recursive;
    bool hidden;
    bool persistent_flag;
    unsigned int interval;
    int taskCount;
    int scanID;
    int objectID;
    int storageID;
    time_t  last_mod_previous_scan; 
    time_t  last_mod_current_scan;
    zmm::Ref<zmm::Object> timer_parameter;
};

#endif
