/*MT*

    MediaTomb - http://www.mediatomb.cc/

    autoscan_inotify.h - this file is part of MediaTomb.

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

/// @file content/inotify/autoscan_inotify.h
#ifndef __AUTOSCAN_INOTIFY_H__
#define __AUTOSCAN_INOTIFY_H__

#ifdef HAVE_INOTIFY
#include "inotify_manager.h"
#include "util/grb_fs.h"

#include <queue>

// forward declarations
class AutoscanDirectory;
class Config;
class Content;
class Database;
class InotifyHandler;
class Watch;
class DirectoryWatch;
class WatchAutoscan;

/// @brief Manager class for autoscan directories with inotify
class AutoscanInotify : public InotifyManager<DirectoryWatch> {
public:
    explicit AutoscanInotify(const std::shared_ptr<Content>& content);

    AutoscanInotify(const AutoscanInotify&) = delete;
    AutoscanInotify& operator=(const AutoscanInotify&) = delete;

    /// @brief Start monitoring a directory
    void monitor(const std::shared_ptr<AutoscanDirectory>& dir);

    /// @brief Stop monitoring a directory
    void unmonitor(const std::shared_ptr<AutoscanDirectory>& dir);

    /// @brief Update monitoring of a directory
    std::shared_ptr<DirectoryWatch> monitorUnmonitorRecursive(
        const fs::directory_entry& startPath,
        bool unmonitor,
        const std::shared_ptr<AutoscanDirectory>& adir,
        bool isStartPoint,
        bool followSymlinks);

    /// @brief Add monitoring hook for a non existing directory
    void monitorNonexisting(
        const fs::path& path,
        const std::shared_ptr<AutoscanDirectory>& adir);

    /// @brief Check for monitoring hooks on non existing directory
    void recheckNonexistingMonitors(
        int wd,
        const std::shared_ptr<DirectoryWatch>& wdObj);

    /// @brief Check for moving items
    void checkMoveWatches(
        int wd,
        const std::shared_ptr<DirectoryWatch>& wdObj);

    /// @brief Remove check for moving items
    void removeWatchMoves(int wd);

    /// @brief Remove watches recursively under some start point
    void removeDescendants(int wd);

private:
    std::shared_ptr<Config> config;
    std::shared_ptr<Database> database;
    std::shared_ptr<Content> content;

    /// @brief main thread proc loop
    void threadProc() override;

    /// @brief directory to add to monitoring
    std::queue<std::shared_ptr<AutoscanDirectory>> monitorQueue;

    /// @brief directory to remove from monitoring
    std::queue<std::shared_ptr<AutoscanDirectory>> unmonitorQueue;

    /// @brief default setting for follow symbolic links
    bool defFollowSymlinks;

    /// @brief default setting for hidden files/folders
    bool defHidden;

    /// @brief Update monitoring of a directory
    int monitorDirectory(
        const fs::path& path,
        const std::shared_ptr<AutoscanDirectory>& adir,
        bool isStartPoint,
        bool hasNonExisting = false,
        const fs::path& nonExistingPath = {});

    /// @brief Remove monitoring of a directory
    void unmonitorDirectory(
        const fs::path& path,
        const std::shared_ptr<AutoscanDirectory>& adir);

    /// @brief Get autoscan associated with watch
    static std::shared_ptr<WatchAutoscan> getAppropriateAutoscan(
        const std::shared_ptr<DirectoryWatch>& wdObj,
        const std::shared_ptr<AutoscanDirectory>& adir);

    /// @brief Remove watch
    bool removeFromWdObj(
        const std::shared_ptr<DirectoryWatch>& wdObj,
        const std::shared_ptr<Watch>& toRemove);

    /// @brief Check for monitoring hooks on non existing directory
    void recheckNonexistingMonitor(
        int curWd,
        const fs::path& nonExistingPath,
        const std::shared_ptr<AutoscanDirectory>& adir);

    /// @brief Remove monitoring hooks on non existing directory
    void removeNonexistingMonitor(
        int wd,
        const std::shared_ptr<DirectoryWatch>& wdObj,
        const fs::path& nonExistingPath);

    /// @brief Add watch for renaming and moving
    int watchPathForMoves(
        const fs::path& path,
        int wd,
        unsigned int retryCount);

    /// @brief Add watch for renaming and moving
    int addMoveWatch(
        const fs::path& path,
        int removeWd,
        int parentWd,
        unsigned int retryCount);

    /// @brief Get directory watch from inotify handler
    std::shared_ptr<DirectoryWatch> getWatch(const InotifyHandler& handler);

    /// @brief Add watches recursively under some start point
    void addDescendant(
        int startPointWd,
        int addWd,
        const std::shared_ptr<AutoscanDirectory>& adir);
};

#endif // HAVE_INOTIFY
#endif // __AUTOSCAN_INOTIFY_H__
