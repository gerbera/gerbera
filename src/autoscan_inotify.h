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

#include "zmmf/zmmf.h"
#include "sync.h"
#include "hash.h"
#include "autoscan.h"
#include "mt_inotify.h"

#define INOTIFY_ROOT -1
#define INOTIFY_UNKNOWN_PARENT_WD -2

enum inotify_watch_type_t
{
    InotifyWatchTypeNonexisting,
    InotifyWatchTypeAutoscan
};

class AutoscanInotify : public zmm::Object
{
public:
    AutoscanInotify();
    virtual ~AutoscanInotify();
    void init();
    
    /// \brief shutdown the inotify thread
    /// 
    /// warning: currently doesn't remove all the remaining inotify watches!
    void shutdown();
    
    /// \brief Start monitoring a directory
    void monitor(zmm::Ref<AutoscanDirectory> dir);
    
    /// \brief Stop monitoring a directory
    void unmonitor(zmm::Ref<AutoscanDirectory> dir);
    
private:
    static void *staticThreadProc(void *arg);
    void threadProc();
    
    pthread_t thread;
    
    zmm::Ref<Inotify> inotify;
    
    zmm::Ref<Cond> cond;
    zmm::Ref<Mutex> mutex;
    
    zmm::Ref<zmm::ObjectQueue<AutoscanDirectory> > monitorQueue;
    zmm::Ref<zmm::ObjectQueue<AutoscanDirectory> > unmonitorQueue;
    
    // event mask with events to watch for (set by constructor);
    int events;
    
    enum watch_type_t
    {
        WatchAutoscanType,
        WatchMoveType
    };
    
    class Watch : public zmm::Object
    {
    public:
        Watch(watch_type_t type)
        {
            this->type = type;
        }
        watch_type_t getType() { return type; }
    private:
        watch_type_t type;
    };
    
    class WatchAutoscan : public Watch
    {
    public:
        WatchAutoscan(bool startPoint, zmm::Ref<AutoscanDirectory> adir, zmm::String normalizedAutoscanPath) : Watch(WatchAutoscanType)
        {
            setAutoscanDirectory(adir);
            setNormalizedAutoscanPath(normalizedAutoscanPath);
            setNonexistingPathArray(nil);
            this->startPoint = startPoint;
            this->descendants = nil;
        }
        zmm::Ref<AutoscanDirectory> getAutoscanDirectory() { return adir; }
        void setAutoscanDirectory(zmm::Ref<AutoscanDirectory> adir) { this->adir = adir; }
        zmm::String getNormalizedAutoscanPath() { return normalizedAutoscanPath; }
        void setNormalizedAutoscanPath(zmm::String normalizedAutoscanPath) { this->normalizedAutoscanPath = normalizedAutoscanPath; }
        bool isStartPoint() { return startPoint; }
        zmm::Ref<zmm::Array<zmm::StringBase> > getNonexistingPathArray() { return nonexistingPathArray; }
        void setNonexistingPathArray(zmm::Ref<zmm::Array<zmm::StringBase> > nonexistingPathArray) { this->nonexistingPathArray = nonexistingPathArray; }
        void addDescendant(int wd)
        {
            if (descendants == nil)
                descendants = zmm::Ref<zmm::IntArray>(new zmm::IntArray());
            descendants->append(wd);
        }
        zmm::Ref<zmm::IntArray> getDescendants() { return descendants; }
    private:
        zmm::Ref<AutoscanDirectory> adir;
        bool startPoint;
        zmm::Ref<zmm::IntArray> descendants;
        zmm::String normalizedAutoscanPath;
        zmm::Ref<zmm::Array<zmm::StringBase> > nonexistingPathArray;
    };
    
    class WatchMove : public Watch
    {
    public:
        WatchMove(int removeWd) : Watch(WatchMoveType)
        {
            this->removeWd = removeWd;
        }
        int getRemoveWd() { return removeWd; }
    private:
        int removeWd;
    };
    
    class Wd : public zmm::Object
    {
    public:
        Wd(zmm::String path, int wd, int parentWd)
        {
            wdWatches = zmm::Ref<zmm::Array<Watch> >(new zmm::Array<Watch>(1));
            this->path = path;
            this->wd = wd;
            this->parentWd = parentWd;
        }
        zmm::String getPath() { return path; }
        int getWd() { return wd; }
        int getParentWd() { return parentWd; }
        void setParentWd(int parentWd) { this->parentWd = parentWd; }
        zmm::Ref<zmm::Array<Watch> > getWdWatches() { return wdWatches; }
    private:
        zmm::Ref<zmm::Array<Watch> > wdWatches;
        zmm::String path;
        int parentWd;
        int wd;
    };
    
    zmm::Ref<DBOHash<int, Wd> > watches;
    
    zmm::String normalizePathNoEx(zmm::String path);
    
    void monitorUnmonitorRecursive(zmm::String startPath, bool unmonitor, zmm::Ref<AutoscanDirectory> adir, zmm::String normalizedAutoscanPath, bool startPoint);
    int monitorDirectory(zmm::String path, zmm::Ref<AutoscanDirectory> adir, zmm::String normalizedAutoscanPath, bool startPoint, zmm::Ref<zmm::Array<zmm::StringBase> > pathArray = nil);
    void unmonitorDirectory(zmm::String path, zmm::Ref<AutoscanDirectory> adir);
    
    zmm::Ref<WatchAutoscan> getAppropriateAutoscan(zmm::Ref<Wd> wdObj, zmm::Ref<AutoscanDirectory> adir);
    zmm::Ref<WatchAutoscan> getAppropriateAutoscan(zmm::Ref<Wd> wdObj, zmm::String path);
    zmm::Ref<WatchAutoscan> getStartPoint(zmm::Ref<Wd> wdObj);
    
    bool removeFromWdObj(zmm::Ref<Wd> wdObj, zmm::Ref<Watch> toRemove);
    bool removeFromWdObj(zmm::Ref<Wd> wdObj, zmm::Ref<WatchAutoscan> toRemove);
    bool removeFromWdObj(zmm::Ref<Wd> wdObj, zmm::Ref<WatchMove> toRemove);
    
    void monitorNonexisting(zmm::String path, zmm::Ref<AutoscanDirectory> adir, zmm::String normalizedAutoscanPath);
    void recheckNonexistingMonitor(int curWd, zmm::Ref<zmm::Array<zmm::StringBase> > nonexistingPathArray, zmm::Ref<AutoscanDirectory> adir, zmm::String normalizedAutoscanPath);
    void recheckNonexistingMonitors(int wd, zmm::Ref<Wd> wdObj);
    void removeNonexistingMonitor(int wd, zmm::Ref<Wd> wdObj, zmm::Ref<zmm::Array<zmm::StringBase> > pathAr);
    
    int watchPathForMoves(zmm::String path, int wd);
    int addMoveWatch(zmm::String path, int removeWd, int parentWd);
    void checkMoveWatches(int wd, zmm::Ref<Wd> wdObj);
    void removeWatchMoves(int wd);
    
    void addDescendant(int startPointWd, int addWd, zmm::Ref<AutoscanDirectory> adir);
    void removeDescendants(int wd);
    
    /// \brief is set to true by shutdown() if the inotify thread should terminate
    bool shutdownFlag;
};

#endif // __AUTOSCAN_INOTIFY_H__
