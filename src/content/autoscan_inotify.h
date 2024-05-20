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

#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <unordered_map>
#include <vector>

#include "config/config.h"
#include "context.h"
#include "util/mt_inotify.h"

// forward declaration
class AutoscanDirectory;
class ContentManager;

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
    int events;
    bool defFollowSymlinks;
    bool defHidden;

    enum class WatchType {
        Autoscan,
        Move
    };

    class Watch {
    public:
        explicit Watch(WatchType type)
            : type(type)
        {
        }
        WatchType getType() const { return type; }

    private:
        WatchType type;
    };

    class WatchAutoscan : public Watch {
    public:
        WatchAutoscan(bool isStartPoint, std::shared_ptr<AutoscanDirectory> adir)
            : Watch(WatchType::Autoscan)
            , adir(std::move(adir))
            , isStartPoint(isStartPoint)
        {
        }
        std::shared_ptr<AutoscanDirectory> getAutoscanDirectory() const { return adir; }
        bool getIsStartPoint() const { return isStartPoint; }
        fs::path getNonExistingPath() const { return nonExistingPath; }
        void setNonExistingPath(const fs::path& nonExistingPath) { this->nonExistingPath = nonExistingPath; }
        void addDescendant(int wd) { descendants.push_back(wd); }
        const std::vector<int>& getDescendants() const { return descendants; }

    private:
        std::shared_ptr<AutoscanDirectory> adir;
        bool isStartPoint;
        std::vector<int> descendants;
        fs::path nonExistingPath;
    };

    class WatchMove : public Watch {
    public:
        explicit WatchMove(int removeWd)
            : Watch(WatchType::Move)
            , removeWd(removeWd)
        {
        }
        int getRemoveWd() const { return removeWd; }

    private:
        int removeWd;
    };

public:
    class Wd {
    public:
        Wd(fs::path path, int wd, int parentWd)
            : path(std::move(path))
            , parentWd(parentWd)
            , wd(wd)
        {
        }
        fs::path getPath() const { return path; }
        void setPath(const fs::path& newPath) { path = newPath; }
        int getWd() const { return wd; }
        int getParentWd() const { return parentWd; }
        void setParentWd(int parentWd) { this->parentWd = parentWd; }

        const std::unique_ptr<std::vector<std::shared_ptr<Watch>>>& getWdWatches() const { return wdWatches; }
        void addWatch(const std::shared_ptr<Watch>& w) { wdWatches->push_back(w); }

    private:
        std::unique_ptr<std::vector<std::shared_ptr<Watch>>> wdWatches { std::make_unique<std::vector<std::shared_ptr<Watch>>>() };
        fs::path path;
        int parentWd;
        int wd;
    };

private:
    std::unordered_map<int, std::shared_ptr<Wd>> watches;

public:
    std::shared_ptr<Wd> monitorUnmonitorRecursive(const fs::directory_entry& startPath, bool unmonitor, const std::shared_ptr<AutoscanDirectory>& adir, bool isStartPoint, bool followSymlinks);

private:
    int monitorDirectory(const fs::path& path, const std::shared_ptr<AutoscanDirectory>& adir, bool isStartPoint, bool hasNonExisting = false, const fs::path& nonExistingPath = {});
    void unmonitorDirectory(const fs::path& path, const std::shared_ptr<AutoscanDirectory>& adir);

    static std::shared_ptr<WatchAutoscan> getAppropriateAutoscan(const std::shared_ptr<Wd>& wdObj, const std::shared_ptr<AutoscanDirectory>& adir);

public:
    static std::shared_ptr<WatchAutoscan> getAppropriateAutoscan(const std::shared_ptr<Wd>& wdObj, const fs::path& path);

    static std::shared_ptr<WatchAutoscan> getStartPoint(const std::shared_ptr<Wd>& wdObj);

private:
    bool removeFromWdObj(const std::shared_ptr<Wd>& wdObj, const std::shared_ptr<Watch>& toRemove);

public:
    void monitorNonexisting(const fs::path& path, const std::shared_ptr<AutoscanDirectory>& adir);

private:
    void recheckNonexistingMonitor(int curWd, const fs::path& nonExistingPath, const std::shared_ptr<AutoscanDirectory>& adir);

public:
    void recheckNonexistingMonitors(int wd, const std::shared_ptr<Wd>& wdObj);

private:
    void removeNonexistingMonitor(int wd, const std::shared_ptr<Wd>& wdObj, const fs::path& nonExistingPath);

    int watchPathForMoves(const fs::path& path, int wd, unsigned int retryCount);
    int addMoveWatch(const fs::path& path, int removeWd, int parentWd, unsigned int retryCount);
    std::shared_ptr<Wd> getWatch(int wd);

public:
    void checkMoveWatches(int wd, const std::shared_ptr<Wd>& wdObj);
    void removeWatchMoves(int wd);

private:
    void addDescendant(int startPointWd, int addWd, const std::shared_ptr<AutoscanDirectory>& adir);

public:
    void removeDescendants(int wd);

private:
    /// \brief is set to true by shutdown() if the inotify thread should terminate
    bool shutdownFlag;
};

#endif // __AUTOSCAN_INOTIFY_H__
