/*MT*

    MediaTomb - http://www.mediatomb.cc/

    autoscan_inotify.h - this file is part of MediaTomb.

    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>

    Copyright (C) 2006-2010 Gena Batyan <bgeradz@mediatomb.cc>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>,
                            Leonhard Wimmer <leo@mediatomb.cc>

    Copyright (C) 2016-2024 Gerbera Contributors

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

/// \file autoscan_inotify.h
#ifndef __AUTOSCAN_INOTIFY_H__
#define __AUTOSCAN_INOTIFY_H__

#include "config/config.h"
#include "context.h"
#include "inotify_types.h"

// forward declaration
class AutoscanDirectory;
class ContentManager;
class Inotify;
class InotifyHandler;
enum class WatchType;
class Watch;
class DirectoryWatch;
class WatchAutoscan;
class WatchMove;

#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <unordered_map>
#include <vector>

#define INOTIFY_ROOT (-1)
#define INOTIFY_UNKNOWN_PARENT_WD (-2)

class AutoscanInotify {
public:
    explicit AutoscanInotify(const std::shared_ptr<ContentManager>& content);
    ~AutoscanInotify();

    AutoscanInotify(const AutoscanInotify&) = delete;
    AutoscanInotify& operator=(const AutoscanInotify&) = delete;

    void run();

    /// \brief Start monitoring a directory
    void monitor(const std::shared_ptr<AutoscanDirectory>& dir);

    /// \brief Stop monitoring a directory
    void unmonitor(const std::shared_ptr<AutoscanDirectory>& dir);
    std::shared_ptr<DirectoryWatch> monitorUnmonitorRecursive(const fs::directory_entry& startPath, bool unmonitor, const std::shared_ptr<AutoscanDirectory>& adir, bool isStartPoint, bool followSymlinks);
    void monitorNonexisting(const fs::path& path, const std::shared_ptr<AutoscanDirectory>& adir);
    void recheckNonexistingMonitors(int wd, const std::shared_ptr<DirectoryWatch>& wdObj);
    void checkMoveWatches(int wd, const std::shared_ptr<DirectoryWatch>& wdObj);
    void removeWatchMoves(int wd);
    void removeDescendants(int wd);

private:
    std::shared_ptr<Config> config;
    std::shared_ptr<Database> database;
    std::shared_ptr<ContentManager> content;

    void threadProc();

    std::thread thread_;

    std::unique_ptr<Inotify> inotify;

    mutable std::mutex mutex;
    using AutoLock = std::scoped_lock<std::mutex>;

    std::queue<std::shared_ptr<AutoscanDirectory>> monitorQueue;
    std::queue<std::shared_ptr<AutoscanDirectory>> unmonitorQueue;

    // event mask with events to watch for (set by constructor);
    InotifyFlags events;
    bool defFollowSymlinks;
    bool defHidden;

    std::unordered_map<int, std::shared_ptr<DirectoryWatch>> watches;

    int monitorDirectory(const fs::path& path, const std::shared_ptr<AutoscanDirectory>& adir, bool isStartPoint, bool hasNonExisting = false, const fs::path& nonExistingPath = {});
    void unmonitorDirectory(const fs::path& path, const std::shared_ptr<AutoscanDirectory>& adir);

    static std::shared_ptr<WatchAutoscan> getAppropriateAutoscan(const std::shared_ptr<DirectoryWatch>& wdObj, const std::shared_ptr<AutoscanDirectory>& adir);

    bool removeFromWdObj(const std::shared_ptr<DirectoryWatch>& wdObj, const std::shared_ptr<Watch>& toRemove);

    void recheckNonexistingMonitor(int curWd, const fs::path& nonExistingPath, const std::shared_ptr<AutoscanDirectory>& adir);

    void removeNonexistingMonitor(int wd, const std::shared_ptr<DirectoryWatch>& wdObj, const fs::path& nonExistingPath);

    int watchPathForMoves(const fs::path& path, int wd, unsigned int retryCount);
    int addMoveWatch(const fs::path& path, int removeWd, int parentWd, unsigned int retryCount);
    std::shared_ptr<DirectoryWatch> getWatch(const InotifyHandler& handler);

    void addDescendant(int startPointWd, int addWd, const std::shared_ptr<AutoscanDirectory>& adir);

    /// \brief is set to true by shutdown() if the inotify thread should terminate
    bool shutdownFlag;
};

#endif // __AUTOSCAN_INOTIFY_H__
