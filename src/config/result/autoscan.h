/*MT*

    MediaTomb - http://www.mediatomb.cc/

    autoscan.h - this file is part of MediaTomb.

    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>

    Copyright (C) 2006-2010 Gena Batyan <bgeradz@mediatomb.cc>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>,
                            Leonhard Wimmer <leo@mediatomb.cc>

    Copyright (C) 2016-2025 Gerbera Contributors

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

#include "common.h"
#include "config/result/edit_helper.h"
#include "upnp/upnp_common.h"
#include "util/timer.h"

#include <mutex>
#include <string_view>

// forward declarations
class CdsContainer;
class Database;
class ImportService;

#define AUTOSCAN_INOTIFY "inotify"
#define AUTOSCAN_TIMED "timed"

///\brief Scan mode - type of scan (timed, inotify, etc.)
enum class AutoscanScanMode {
    Timed,
    INotify
};

///\brief Media mode - media handling mode (timed, inotify, etc.)
enum class AutoscanMediaMode {
    Audio,
    Image,
    Video,
    Mixed,
};

/// \brief Provides information about one autoscan directory.
class AutoscanDirectory : public Editable {
public:
    enum class MediaType {
        Any = -1,

        Audio,
        Music,
        AudioBook,
        AudioBroadcast,

        Image,
        Photo,

        Video,
        Movie,
        TV,
        MusicVideo,

        MAX,
    };

    static int makeMediaType(const std::string& optValue);
    static std::string_view mapMediaType(MediaType mt);
    static std::string mapMediaType(int mt);
    static int remapMediaType(const std::string& flag);
    static const std::map<AutoscanMediaMode, std::string> ContainerTypesDefaults;

    AutoscanDirectory() = default;

    /// \brief Creates a new AutoscanDirectory object.
    /// \param location autoscan path
    /// \param mode scan mode
    /// \param recursive process directories recursively
    /// \param persistent entry is from config file
    /// \param interval rescan interval in seconds (only for timed scan mode)
    /// \param hidden include hidden files
    /// \param followSymlinks follow symbolic links
    /// \param mediaType type of media to load from directory zero means none.
    /// \param containerMap mapping of media types to container types
    AutoscanDirectory(
        fs::path location,
        AutoscanScanMode mode,
        bool recursive,
        bool persistent,
        unsigned int interval = 0,
        bool hidden = false,
        bool followSymlinks = false,
        int mediaType = -1,
        const std::map<AutoscanMediaMode, std::string>& containerMap = ContainerTypesDefaults);
    virtual ~AutoscanDirectory() = default;

    bool equals(const std::shared_ptr<AutoscanDirectory>& other) { return this->getScanID() == other->getScanID(); }

    void setDatabaseID(int databaseID) { this->databaseID = databaseID; }
    int getDatabaseID() const { return databaseID; }

    /// \brief The location can only be set once!
    void setLocation(const fs::path& location);
    const fs::path& getLocation() const { return location; }

    void setScanMode(AutoscanScanMode mode) { this->mode = mode; }
    AutoscanScanMode getScanMode() const { return mode; }

    void setRecursive(bool recursive) { this->recursive = recursive; }
    bool getRecursive() const { return recursive; }

    void setRetryCount(unsigned int retryCount) { this->retryCount = retryCount; }
    unsigned int getRetryCount() const { return retryCount; }

    void setHidden(bool hidden) { this->hidden = hidden; }
    bool getHidden() const { return hidden; }

    void setDirTypes(bool dirTypes) { this->dirTypes = dirTypes; }
    bool hasDirTypes() const { return dirTypes; }

    void setFollowSymlinks(bool followSymlinks) { this->followSymlinks = followSymlinks; }
    bool getFollowSymlinks() const { return followSymlinks; }

    void setScanContent(const std::map<std::string, bool>& scanContent) { this->scanContent = scanContent; }
    std::map<std::string, bool> getScanContent() const { return scanContent; }
    void setMediaType(int mt);
    int getMediaType() const { return mediaType; }
    bool hasContent(const std::string& upnpClass) const;

    void setInterval(std::chrono::seconds interval) { this->interval = interval; }
    std::chrono::seconds getInterval() const { return interval; }

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
    int getTaskCount() const { return taskCount; }
    void setTaskCount(int taskCount) { this->taskCount = taskCount; }

    /// \brief Sets the task ID.
    ///
    /// The task ID helps us to identify to which scan a particular task
    /// belongs. Recursive scans spawn new tasks - they all should have
    /// the same id.
    void setScanID(int id);
    int getScanID() const { return scanID; }

    void setObjectID(int id) { objectID = id; }
    int getObjectID() const { return objectID; }

    void setPersistent(bool persistentFlag) { this->persistentFlag = persistentFlag; }
    bool persistent() const { return persistentFlag; }

    /// @brief Enable rescanning unknown files
    void setForceRescan(bool forceRescan) { this->forceRescan = forceRescan; }
    bool getForceRescan() const { return forceRescan; }

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

    /// \brief Get the container types dictionary
    const std::map<AutoscanMediaMode, std::string>& getContainerTypes()
    {
        return containerMap;
    }
    void setContainerType(AutoscanMediaMode entry, const std::string& value)
    {
        containerMap[entry] = value;
    }

    void setImportService(std::shared_ptr<ImportService> is) { importService = std::move(is); }
    std::shared_ptr<ImportService> getImportService() const { return importService; }

    /* helpers for autoscan enum stuff */
    static const char* mapScanmode(AutoscanScanMode scanmode);
    static AutoscanScanMode remapScanmode(const std::string& scanmode);

    /* overrides for Editable */
    void setValid(bool newEntry, std::size_t index) override
    {
        setScanID(index);
    }
    void setInvalid() override
    {
        scanID = INVALID_SCAN_ID;
    }
    bool isValid() const
    {
        return scanID != INVALID_SCAN_ID;
    }

protected:
    fs::path location;
    AutoscanScanMode mode {};
    bool recursive {};
    bool hidden {};
    bool forceRescan {};
    bool dirTypes { true };
    bool followSymlinks {};
    bool persistentFlag {};
    std::chrono::seconds interval = std::chrono::seconds::zero();
    unsigned int retryCount { 0 };
    int taskCount {};
    int scanID { INVALID_SCAN_ID };
    int objectID { INVALID_OBJECT_ID };
    int databaseID { INVALID_OBJECT_ID };
    std::chrono::seconds last_mod_previous_scan = std::chrono::seconds::zero();
    std::chrono::seconds last_mod_current_scan = std::chrono::seconds::zero();
    std::shared_ptr<Timer::Parameter> timer_parameter { std::make_shared<Timer::Parameter>(Timer::TimerParamType::IDAutoscan, INVALID_SCAN_ID) };
    std::map<fs::path, std::chrono::seconds> lastModified;
    unsigned int activeScanCount {};
    std::map<std::string, bool> scanContent { { UPNP_CLASS_AUDIO_ITEM, true }, { UPNP_CLASS_IMAGE_ITEM, true }, { UPNP_CLASS_VIDEO_ITEM, true } };
    int mediaType { -1 };
    std::map<AutoscanMediaMode, std::string> containerMap;
    std::shared_ptr<ImportService> importService;

    constexpr const static int INVALID_SCAN_ID = -1;
};

#endif
