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

#include "util/timer.h"

// forward declarations
class CdsContainer;
class Database;

///\brief Scan mode - type of scan (timed, inotify, fam, etc.)
enum class ScanMode {
    Timed,
    INotify
};

/// \brief Provides information about one autoscan directory.
class AutoscanDirectory {
public:
    AutoscanDirectory() = default;

    /// \brief Creates a new AutoscanDirectory object.
    /// \param location autoscan path
    /// \param mode scan mode
    /// \param recursive process directories recursively
    /// \param interval rescan interval in seconds (only for timed scan mode)
    /// \param hidden include hidden files
    /// zero means none.
    AutoscanDirectory(fs::path location, ScanMode mode, bool recursive, bool persistent);

    void setDatabaseID(int databaseID) { this->databaseID = databaseID; }
    int getDatabaseID() const { return databaseID; }

    /// \brief The location can only be set once!
    void setLocation(const fs::path& location);
    fs::path getLocation() const { return location; }

    void setScanMode(ScanMode mode) { this->mode = mode; }
    ScanMode getScanMode() const { return mode; }

    void setRecursive(bool recursive) { this->recursive = recursive; }
    [[nodiscard]] bool isRecursive() const { return recursive; }

    void setOriginal(bool orig) { this->isOrig = orig; }
    [[nodiscard]] bool isOriginal() const { return isOrig; }

    void setHidden(bool hidden) { this->hidden = hidden; }
    [[nodiscard]] bool getHidden() const { return hidden; }

    void setInterval(std::chrono::seconds interval) { this->interval = interval; }
    [[nodiscard]] std::chrono::seconds getInterval() const { return interval; }

    /// \brief Increments the task count.
    ///
    /// When recursive autoscan is in progress, we only want to subscribe to
    /// a timer event when the scan is finished. However, recursive scans
    /// spawn tasks for each directory. When adding a rescan task for
    /// subdirectories, the taskCount will be incremented. When a task is
    /// done the count will be decremented. When timer gets to zero,
    /// we will resubscribe.
    void incTaskCount() { taskCount++; }
    void decTaskCount() { taskCount--; }
    [[nodiscard]] int getTaskCount() const { return taskCount; }
    void setTaskCount(int taskCount) { this->taskCount = taskCount; }

    /// \brief Sets the task ID.
    ///
    /// The task ID helps us to identify to which scan a particular task
    /// belongs. Recursive scans spawn new tasks - they all should have
    /// the same id.
    void setScanID(int id);
    [[nodiscard]] int getScanID() const { return scanID; }

    void setObjectID(int id) { objectID = id; }
    [[nodiscard]] int getObjectID() const { return objectID; }

    void setPersistent(bool persistent_flag) { this->persistent_flag = persistent_flag; }
    [[nodiscard]] bool isPersistent() const { return persistent_flag; }

    /// \brief Sets the last modification time of the current ongoing scan.
    ///
    /// When doing a FullScan we look at modification times of the files.
    /// During the recursion of one AutoscanDirectory (which will be the
    /// starting point and which may have subcontainers) we must compare
    /// the last modification time of the starting point but we may not
    /// overwrite it until we are done.
    /// The time will be only set if it is higher than the previous value!
    void setCurrentLMT(const fs::path& loc, std::chrono::seconds lmt);
    std::chrono::seconds getPreviousLMT() const;
    std::chrono::seconds getPreviousLMT(const fs::path& loc, const std::shared_ptr<CdsContainer>& parent) const;
    bool updateLMT();
    void resetLMT()
    {
        lastModified.clear();
        last_mod_previous_scan = {};
        last_mod_current_scan = {};
    }
    unsigned int getActiveScanCount() const { return activeScanCount; }

    /// \brief copies all properties to another object
    void copyTo(const std::shared_ptr<AutoscanDirectory>& copy) const;

    /// \brief Get the timer notify parameter associated with this directory.
    std::shared_ptr<Timer::Parameter> getTimerParameter() const;

    /* helpers for autoscan stuff */
    static const char* mapScanMode(ScanMode scanmode);
    static ScanMode remapScanMode(const std::string& scanmode);

    static constexpr int INVALID_SCAN_ID = -1;
    [[nodiscard]] bool isValid() const {return scanID != INVALID_SCAN_ID; };

protected:
    fs::path location;
    ScanMode mode {};
    bool isOrig {};
    bool recursive {};
    bool hidden {};
    bool persistent_flag {};
    std::chrono::seconds interval {};
    int taskCount {};
    int scanID { INVALID_SCAN_ID };
    int objectID { INVALID_OBJECT_ID };
    int databaseID { INVALID_OBJECT_ID };
    std::chrono::seconds last_mod_previous_scan {};
    std::chrono::seconds last_mod_current_scan {};
    std::shared_ptr<Timer::Parameter> timer_parameter { std::make_shared<Timer::Parameter>(Timer::Parameter::IDAutoscan, AutoscanDirectory::INVALID_SCAN_ID) };
    std::map<fs::path, std::chrono::seconds> lastModified;
    unsigned int activeScanCount {};
};

#endif
