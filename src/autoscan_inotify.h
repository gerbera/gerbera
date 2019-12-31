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
    void monitor(zmm::Ref<AutoscanDirectory> dir);

    /// \brief Stop monitoring a directory
    void unmonitor(zmm::Ref<AutoscanDirectory> dir);

private:
    std::shared_ptr<Storage> storage;
    std::shared_ptr<ContentManager> content;

    void threadProc();

    std::thread thread_;

    zmm::Ref<Inotify> inotify;

    std::mutex mutex;
    using AutoLock = std::lock_guard<std::mutex>;

    zmm::Ref<zmm::ObjectQueue<AutoscanDirectory>> monitorQueue;
    zmm::Ref<zmm::ObjectQueue<AutoscanDirectory>> unmonitorQueue;

    // event mask with events to watch for (set by constructor);
    int events;

    enum class WatchType {
        Autoscan,
        Move
    };

    class Watch : public zmm::Object {
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
        WatchAutoscan(bool startPoint, zmm::Ref<AutoscanDirectory> adir, std::string normalizedAutoscanPath)
            : Watch(WatchType::Autoscan)
        {
            setAutoscanDirectory(adir);
            setNormalizedAutoscanPath(normalizedAutoscanPath);
            this->startPoint = startPoint;
        }
        zmm::Ref<AutoscanDirectory> getAutoscanDirectory() { return adir; }
        void setAutoscanDirectory(zmm::Ref<AutoscanDirectory> adir) { this->adir = adir; }
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
        zmm::Ref<AutoscanDirectory> adir;
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
            wdWatches = zmm::Ref<zmm::Array<Watch>>(new zmm::Array<Watch>(1));
            this->path = path;
            this->wd = wd;
            this->parentWd = parentWd;
        }
        std::string getPath() { return path; }
        int getWd() { return wd; }
        int getParentWd() { return parentWd; }
        void setParentWd(int parentWd) { this->parentWd = parentWd; }
        zmm::Ref<zmm::Array<Watch>> getWdWatches() { return wdWatches; }

    private:
        zmm::Ref<zmm::Array<Watch>> wdWatches;
        std::string path;
        int parentWd;
        int wd;
    };

    std::shared_ptr<std::unordered_map<int, zmm::Ref<Wd>>> watches;

    std::string normalizePathNoEx(std::string path);

    void monitorUnmonitorRecursive(std::string startPath, bool unmonitor, zmm::Ref<AutoscanDirectory> adir, std::string normalizedAutoscanPath, bool startPoint);
    int monitorDirectory(std::string path, zmm::Ref<AutoscanDirectory> adir, std::string normalizedAutoscanPath, bool startPoint, std::vector<std::string>* pathArray = nullptr);
    void unmonitorDirectory(std::string path, zmm::Ref<AutoscanDirectory> adir);

    zmm::Ref<WatchAutoscan> getAppropriateAutoscan(zmm::Ref<Wd> wdObj, zmm::Ref<AutoscanDirectory> adir);
    zmm::Ref<WatchAutoscan> getAppropriateAutoscan(zmm::Ref<Wd> wdObj, std::string path);
    zmm::Ref<WatchAutoscan> getStartPoint(zmm::Ref<Wd> wdObj);

    bool removeFromWdObj(zmm::Ref<Wd> wdObj, zmm::Ref<Watch> toRemove);
    bool removeFromWdObj(zmm::Ref<Wd> wdObj, zmm::Ref<WatchAutoscan> toRemove);
    bool removeFromWdObj(zmm::Ref<Wd> wdObj, zmm::Ref<WatchMove> toRemove);

    void monitorNonexisting(std::string path, zmm::Ref<AutoscanDirectory> adir, std::string normalizedAutoscanPath);
    void recheckNonexistingMonitor(int curWd, std::vector<std::string> nonexistingPathArray, zmm::Ref<AutoscanDirectory> adir, std::string normalizedAutoscanPath);
    void recheckNonexistingMonitors(int wd, zmm::Ref<Wd> wdObj);
    void removeNonexistingMonitor(int wd, zmm::Ref<Wd> wdObj, std::vector<std::string> pathAr);

    int watchPathForMoves(std::string path, int wd);
    int addMoveWatch(std::string path, int removeWd, int parentWd);
    void checkMoveWatches(int wd, zmm::Ref<Wd> wdObj);
    void removeWatchMoves(int wd);

    void addDescendant(int startPointWd, int addWd, zmm::Ref<AutoscanDirectory> adir);
    void removeDescendants(int wd);

    /// \brief is set to true by shutdown() if the inotify thread should terminate
    bool shutdownFlag;
};

#endif // __AUTOSCAN_INOTIFY_H__
