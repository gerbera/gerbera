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

#include <mutex>
#include <filesystem>
namespace fs = std::filesystem;

#include "util/timer.h"

#define INVALID_SCAN_ID -1

///\brief Scan level - the way how exactly directories should be scanned.
enum class ScanLevel {
    Basic, // file was added or removed from the directory
    Full // file was modified/added/removed
};

///\brief Scan mode - type of scan (timed, inotify, fam, etc.)
enum class ScanMode {
    Timed,
    INotify
};

// forward declaration
class Storage;
class AutoscanDirectory;

class AutoscanList {
public:
    AutoscanList(std::shared_ptr<Storage> storage);

    /// \brief Adds a new AutoscanDirectory to the list.
    ///
    /// The scanID of the directory is invalidated and set to
    /// the index in the AutoscanList.
    ///
    /// \param dir AutoscanDirectory to add to the list.
    /// \return scanID of the newly added AutoscanDirectory
    int add(const std::shared_ptr<AutoscanDirectory>& dir);

    void addList(const std::shared_ptr<AutoscanList>& list);

    std::shared_ptr<AutoscanDirectory> get(size_t id);

    std::shared_ptr<AutoscanDirectory> get(const std::string& location);

    std::shared_ptr<AutoscanDirectory> getByObjectID(int objectID);

    size_t size() { return list.size(); }

    /// \brief removes the AutoscanDirectory given by its scan ID
    void remove(size_t id);

    int removeByObjectID(int objectID);

    /// \brief removes the AutoscanDirectory with the given location
    /// \param location the location to remove
    /// \return the scanID, that was removed; if nothing removed: INVALID_SCAN_ID
    int remove(const std::string& location);

    /// \brief removes the AutoscanDirectory if it is a subdirectory of a given location.
    /// \param parent parent directory.
    /// \param persistent also remove persistent directories.
    /// \return AutoscanList of removed directories, where each directory object in the list is a copy and not the original reference.
    std::shared_ptr<AutoscanList> removeIfSubdir(const std::string& parent, bool persistent = false);

    /// \brief Send notification for each directory that is stored in the list.
    ///
    /// \param sub instance of the class that will receive the notifications.
    void notifyAll(Timer::Subscriber* sub);

    /// \brief updates the last_modified data for all AutoscanDirectories.
    void updateLMinDB();

    /// \brief returns a copy of the autoscan list in the form of an array
    std::vector<std::shared_ptr<AutoscanDirectory>> getArrayCopy();

protected:
    std::shared_ptr<Storage> storage;

    std::recursive_mutex mutex;
    using AutoLock = std::lock_guard<std::recursive_mutex>;

    std::vector<std::shared_ptr<AutoscanDirectory>> list;
    int _add(const std::shared_ptr<AutoscanDirectory>& dir);
};

/// \brief Provides information about one autoscan directory.
class AutoscanDirectory {
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
    AutoscanDirectory(fs::path location, ScanMode mode,
        ScanLevel level, bool recursive,
        bool persistent,
        int id = INVALID_SCAN_ID, unsigned int interval = 0, bool hidden = false);

    void setStorageID(int storageID) { this->storageID = storageID; }

    int getStorageID() { return storageID; }

    /// \brief The location can only be set once!
    void setLocation(fs::path location);

    fs::path getLocation() { return location; }

    ScanMode getScanMode() { return mode; }

    void setScanMode(ScanMode mode) { this->mode = mode; }

    ScanLevel getScanLevel() { return level; }

    void setScanLevel(ScanLevel level) { this->level = level; }

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

    void resetLMT()
    {
        last_mod_previous_scan = 0;
        last_mod_current_scan = 0;
    }

    /// \brief copies all properties to another object
    void copyTo(const std::shared_ptr<AutoscanDirectory>& copy);

    /// \brief Get the timer notify parameter associated with this directory.
    std::shared_ptr<Timer::Parameter> getTimerParameter();

    /* helpers for autoscan stuff */
    static std::string mapScanmode(ScanMode scanmode);
    static ScanMode remapScanmode(const std::string& scanmode);
    static std::string mapScanlevel(ScanLevel scanlevel);
    static ScanLevel remapScanlevel(const std::string& scanlevel);

protected:
    fs::path location;
    ScanMode mode;
    ScanLevel level;
    bool recursive;
    bool hidden;
    bool persistent_flag;
    unsigned int interval;
    int taskCount;
    int scanID;
    int objectID;
    int storageID;
    time_t last_mod_previous_scan;
    time_t last_mod_current_scan;
    std::shared_ptr<Timer::Parameter> timer_parameter;
};

#endif
