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
#include <thread>
#include <unordered_map>
#include <vector>
#include <queue>

#include "autoscan.h"
#include "util/mt_inotify.h"
#include "zmm/zmmf.h"

#define INOTIFY_ROOT -1
#define INOTIFY_UNKNOWN_PARENT_WD -2

// forward declaration
class Storage;
class ContentManager;

class AutoscanInotify {
public:
    AutoscanInotify(std::shared_ptr<Storage> storage, std::shared_ptr<ContentManager> content);
    ~AutoscanInotify();

    void run();

    /// \brief Start monitoring a directory
    void monitor(std::shared_ptr<AutoscanDirectory> dir);

    /// \brief Stop monitoring a directory
    void unmonitor(std::shared_ptr<AutoscanDirectory> dir);

private:
    std::shared_ptr<Storage> storage;
    std::shared_ptr<ContentManager> content;

    void threadProc();

    std::thread thread_;

    zmm::Ref<Inotify> inotify;

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
        Watch(WatchType type)
        {
            this->type = type;
        }
        WatchType getType() { return type; }

    private:
        WatchType type;
    };

    class WatchAutoscan : public Watch {
    public:
        WatchAutoscan(bool startPoint, std::shared_ptr<AutoscanDirectory> adir, std::string normalizedAutoscanPath)
            : Watch(WatchType::Autoscan)
        {
            setAutoscanDirectory(adir);
            setNormalizedAutoscanPath(normalizedAutoscanPath);
            this->startPoint = startPoint;
        }
        std::shared_ptr<AutoscanDirectory> getAutoscanDirectory() { return adir; }
        void setAutoscanDirectory(std::shared_ptr<AutoscanDirectory> adir) { this->adir = adir; }
        std::string getNormalizedAutoscanPath() { return normalizedAutoscanPath; }
        void setNormalizedAutoscanPath(std::string normalizedAutoscanPath) { this->normalizedAutoscanPath = normalizedAutoscanPath; }
        bool isStartPoint() { return startPoint; }
        std::vector<std::string> getNonexistingPathArray() { return nonexistingPathArray; }
        void setNonexistingPathArray(std::vector<std::string> nonexistingPathArray) { this->nonexistingPathArray = nonexistingPathArray; }
        void addDescendant(int wd)
        {
            descendants.push_back(wd);
        }
        const std::vector<int>& getDescendants() const { return descendants; }

    private:
        std::shared_ptr<AutoscanDirectory> adir;
        bool startPoint;
        std::vector<int> descendants;
        std::string normalizedAutoscanPath;
        std::vector<std::string> nonexistingPathArray;
    };

    class WatchMove : public Watch {
    public:
        WatchMove(int removeWd)
            : Watch(WatchType::Move)
        {
            this->removeWd = removeWd;
        }
        int getRemoveWd() { return removeWd; }

    private:
        int removeWd;
    };

    class Wd : public zmm::Object {
    public:
        Wd(std::string path, int wd, int parentWd)
        {
            wdWatches = std::make_shared<std::vector<std::shared_ptr<Watch>>>();
            this->path = path;
            this->wd = wd;
            this->parentWd = parentWd;
        }
        std::string getPath() { return path; }
        int getWd() { return wd; }
        int getParentWd() { return parentWd; }
        void setParentWd(int parentWd) { this->parentWd = parentWd; }

        std::shared_ptr<std::vector<std::shared_ptr<Watch>>> getWdWatches() { return wdWatches; }
        void addWatch(std::shared_ptr<Watch> w) { wdWatches->push_back(w); }

    private:
        std::shared_ptr<std::vector<std::shared_ptr<Watch>>> wdWatches;
        std::string path;
        int parentWd;
        int wd;
    };

    std::shared_ptr<std::unordered_map<int, zmm::Ref<Wd>>> watches;

    std::string normalizePathNoEx(std::string path);

    void monitorUnmonitorRecursive(std::string startPath, bool unmonitor, std::shared_ptr<AutoscanDirectory> adir, std::string normalizedAutoscanPath, bool startPoint);
    int monitorDirectory(std::string path, std::shared_ptr<AutoscanDirectory> adir, std::string normalizedAutoscanPath, bool startPoint, std::vector<std::string>* pathArray = nullptr);
    void unmonitorDirectory(std::string path, std::shared_ptr<AutoscanDirectory> adir);

    std::shared_ptr<WatchAutoscan> getAppropriateAutoscan(zmm::Ref<Wd> wdObj, std::shared_ptr<AutoscanDirectory> adir);
    std::shared_ptr<WatchAutoscan> getAppropriateAutoscan(zmm::Ref<Wd> wdObj, std::string path);
    std::shared_ptr<WatchAutoscan> getStartPoint(zmm::Ref<Wd> wdObj);

    bool removeFromWdObj(zmm::Ref<Wd> wdObj, std::shared_ptr<Watch> toRemove);

    void monitorNonexisting(std::string path, std::shared_ptr<AutoscanDirectory> adir, std::string normalizedAutoscanPath);
    void recheckNonexistingMonitor(int curWd, std::vector<std::string> nonexistingPathArray, std::shared_ptr<AutoscanDirectory> adir, std::string normalizedAutoscanPath);
    void recheckNonexistingMonitors(int wd, zmm::Ref<Wd> wdObj);
    void removeNonexistingMonitor(int wd, zmm::Ref<Wd> wdObj, std::vector<std::string> pathAr);

    int watchPathForMoves(std::string path, int wd);
    int addMoveWatch(std::string path, int removeWd, int parentWd);
    void checkMoveWatches(int wd, zmm::Ref<Wd> wdObj);
    void removeWatchMoves(int wd);

    void addDescendant(int startPointWd, int addWd, std::shared_ptr<AutoscanDirectory> adir);
    void removeDescendants(int wd);

    /// \brief is set to true by shutdown() if the inotify thread should terminate
    bool shutdownFlag;
};

#endif // __AUTOSCAN_INOTIFY_H__
