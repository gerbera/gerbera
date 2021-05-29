/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    autoscan_inotify.h - this file is part of MediaTomb.
    
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

/// \file autoscan_inotify.h
#ifndef __AUTOSCAN_INOTIFY_H__
#define __AUTOSCAN_INOTIFY_H__

#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <unordered_map>
#include <vector>

#include "autoscan.h"
#include "config/config.h"
#include "context.h"
#include "util/mt_inotify.h"

// forward declaration
class ContentManager;

#define INOTIFY_ROOT (-1)
#define INOTIFY_UNKNOWN_PARENT_WD (-2)

class AutoscanInotify {
public:
    explicit AutoscanInotify(std::shared_ptr<ContentManager> content);
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

    std::mutex mutex;
    using AutoLock = std::lock_guard<std::mutex>;

    std::queue<std::shared_ptr<AutoscanDirectory>> monitorQueue;
    std::queue<std::shared_ptr<AutoscanDirectory>> unmonitorQueue;

    // event mask with events to watch for (set by constructor);
    int events;

    enum class WatchType {
        Autoscan,
        Move
    };

    class Watch {
    public:
        explicit Watch(WatchType type)
        {
            this->type = type;
        }
        WatchType getType() const { return type; }

    private:
        WatchType type;
    };

    class WatchAutoscan : public Watch {
    public:
        WatchAutoscan(bool startPoint, std::shared_ptr<AutoscanDirectory> adir)
            : Watch(WatchType::Autoscan)
            , adir(std::move(adir))
            , startPoint(startPoint)
        {
        }
        std::shared_ptr<AutoscanDirectory> getAutoscanDirectory() const { return adir; }
        bool isStartPoint() const { return startPoint; }
        std::vector<std::string> getNonexistingPathArray() const { return nonexistingPathArray; }
        void setNonexistingPathArray(const std::vector<std::string>& nonexistingPathArray) { this->nonexistingPathArray = nonexistingPathArray; }
        void addDescendant(int wd)
        {
            descendants.push_back(wd);
        }
        const std::vector<int>& getDescendants() const { return descendants; }

    private:
        std::shared_ptr<AutoscanDirectory> adir;
        bool startPoint;
        std::vector<int> descendants;
        std::vector<std::string> nonexistingPathArray;
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

    class Wd {
    public:
        Wd(fs::path path, int wd, int parentWd)
            : wdWatches(std::make_shared<std::vector<std::shared_ptr<Watch>>>())
            , path(std::move(path))
            , parentWd(parentWd)
            , wd(wd)
        {
        }
        fs::path getPath() const { return path; }
        int getWd() const { return wd; }
        int getParentWd() const { return parentWd; }
        void setParentWd(int parentWd) { this->parentWd = parentWd; }

        std::shared_ptr<std::vector<std::shared_ptr<Watch>>> getWdWatches() const { return wdWatches; }
        void addWatch(const std::shared_ptr<Watch>& w) { wdWatches->push_back(w); }

    private:
        std::shared_ptr<std::vector<std::shared_ptr<Watch>>> wdWatches;
        fs::path path;
        int parentWd;
        int wd;
    };

    std::unique_ptr<std::unordered_map<int, std::shared_ptr<Wd>>> watches;

    void monitorUnmonitorRecursive(const fs::directory_entry& startPath, bool unmonitor, const std::shared_ptr<AutoscanDirectory>& adir, bool startPoint, bool followSymlinks);
    int monitorDirectory(const fs::path& path, const std::shared_ptr<AutoscanDirectory>& adir, bool startPoint, const std::vector<std::string>* pathArray = nullptr);
    void unmonitorDirectory(const fs::path& path, const std::shared_ptr<AutoscanDirectory>& adir);

    static std::shared_ptr<WatchAutoscan> getAppropriateAutoscan(const std::shared_ptr<Wd>& wdObj, const std::shared_ptr<AutoscanDirectory>& adir);
    static std::shared_ptr<WatchAutoscan> getAppropriateAutoscan(const std::shared_ptr<Wd>& wdObj, const fs::path& path);
    static std::shared_ptr<WatchAutoscan> getStartPoint(const std::shared_ptr<Wd>& wdObj);

    bool removeFromWdObj(const std::shared_ptr<Wd>& wdObj, const std::shared_ptr<Watch>& toRemove);

    void monitorNonexisting(const fs::path& path, const std::shared_ptr<AutoscanDirectory>& adir);
    void recheckNonexistingMonitor(int curWd, const std::vector<std::string>& pathAr, const std::shared_ptr<AutoscanDirectory>& adir);
    void recheckNonexistingMonitors(int wd, const std::shared_ptr<Wd>& wdObj);
    void removeNonexistingMonitor(int wd, const std::shared_ptr<Wd>& wdObj, const std::vector<std::string>& pathAr);

    int watchPathForMoves(const fs::path& path, int wd);
    int addMoveWatch(const fs::path& path, int removeWd, int parentWd);
    void checkMoveWatches(int wd, const std::shared_ptr<Wd>& wdObj);
    void removeWatchMoves(int wd);

    void addDescendant(int startPointWd, int addWd, const std::shared_ptr<AutoscanDirectory>& adir);
    void removeDescendants(int wd);

    /// \brief is set to true by shutdown() if the inotify thread should terminate
    bool shutdownFlag;
};

#endif // __AUTOSCAN_INOTIFY_H__
