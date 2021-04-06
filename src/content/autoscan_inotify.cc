/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    autoscan_inotify.cc - this file is part of MediaTomb.
    
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

/// \file autoscan_inotify.cc

#ifdef HAVE_INOTIFY
#include "autoscan_inotify.h" // API

#include <cassert>

#include "content_manager.h"
#include "database/database.h"

#define INOTIFY_MAX_USER_WATCHES_FILE "/proc/sys/fs/inotify/max_user_watches"

AutoscanInotify::AutoscanInotify(std::shared_ptr<ContentManager> content)
    : config(content->getContext()->getConfig())
    , database(content->getContext()->getDatabase())
    , content(std::move(content))
{
    std::error_code ec;
    if (isRegularFile(INOTIFY_MAX_USER_WATCHES_FILE, ec)) {
        try {
            [[maybe_unused]] int max_watches = std::stoi(trimString(readTextFile(INOTIFY_MAX_USER_WATCHES_FILE)));
            log_debug("Max watches on the system: {}", max_watches);
        } catch (const std::runtime_error& ex) {
            log_error("Could not determine maximum number of inotify user watches: {}", ex.what());
        }
    }

    watches = std::make_unique<std::unordered_map<int, std::shared_ptr<Wd>>>();
    shutdownFlag = true;
    events = IN_CLOSE_WRITE | IN_CREATE | IN_MOVED_FROM | IN_MOVED_TO | IN_DELETE | IN_DELETE_SELF | IN_MOVE_SELF | IN_UNMOUNT;
}

AutoscanInotify::~AutoscanInotify()
{
    std::unique_lock<std::mutex> lock(mutex);
    if (!shutdownFlag) {
        log_debug("start");
        shutdownFlag = true;
        inotify->stop();
        lock.unlock();
        thread_.join();
        log_debug("inotify thread died.");
        inotify = nullptr;
        watches->clear();
    }
}

void AutoscanInotify::run()
{
    AutoLock lock(mutex);

    if (shutdownFlag) {
        shutdownFlag = false;
        inotify = std::make_unique<Inotify>();
        thread_ = std::thread { &AutoscanInotify::threadProc, this };
    }
}

void AutoscanInotify::threadProc()
{
    std::error_code ec;
    while (!shutdownFlag) {
        try {
            std::unique_lock<std::mutex> lock(mutex);

            while (!unmonitorQueue.empty()) {
                auto adir = unmonitorQueue.front();
                unmonitorQueue.pop();

                lock.unlock();

                fs::path location = adir->getLocation();
                if (location.empty()) {
                    lock.lock();
                    continue;
                }
                auto dirEnt = fs::directory_entry(location, ec);

                if (adir->getRecursive()) {
                    log_debug("Removing recursive watch: {}", location.c_str());
                    monitorUnmonitorRecursive(dirEnt, true, adir, true, config->getBoolOption(CFG_IMPORT_FOLLOW_SYMLINKS));
                } else {
                    log_debug("Removing non-recursive watch: {}", location.c_str());
                    unmonitorDirectory(location, adir);
                }

                lock.lock();
            }

            while (!monitorQueue.empty()) {
                auto adir = monitorQueue.front();
                monitorQueue.pop();

                lock.unlock();

                fs::path location = adir->getLocation();
                if (location.empty()) {
                    lock.lock();
                    continue;
                }

                auto dirEnt = fs::directory_entry(location, ec);
                if (!ec) {
                    if (adir->getRecursive()) {
                        log_debug("Adding recursive watch: {}", location.c_str());
                        monitorUnmonitorRecursive(dirEnt, false, adir, true, config->getBoolOption(CFG_IMPORT_FOLLOW_SYMLINKS));
                    } else {
                        log_debug("Adding non-recursive watch: {}", location.c_str());
                        monitorDirectory(location, adir, true);
                    }
                    content->rescanDirectory(adir, adir->getObjectID(), location, false);
                } else {
                    log_error("Failed to read {}: {}", location.c_str(), ec.message());
                }

                lock.lock();
            }

            lock.unlock();

            /* --- get event --- (blocking) */
            inotify_event* event = inotify->nextEvent();
            /* --- */

            if (event) {
                int wd = event->wd;
                int mask = event->mask;
                std::string name = event->name;
                log_debug("inotify event: {} 0x{:x} {}", wd, mask, name.c_str());

                std::shared_ptr<Wd> wdObj = nullptr;
                try {
                    wdObj = watches->at(wd);
                } catch (const std::out_of_range& ex) {
                    inotify->removeWatch(wd);
                    continue;
                }

                fs::path path = wdObj->getPath();
                if (!(mask & (IN_DELETE_SELF | IN_MOVE_SELF | IN_UNMOUNT)))
                    path /= name;

                std::shared_ptr<AutoscanDirectory> adir;
                auto watchAs = getAppropriateAutoscan(wdObj, path);
                if (watchAs != nullptr)
                    adir = watchAs->getAutoscanDirectory();
                else
                    adir = nullptr;

                if (mask & IN_MOVE_SELF) {
                    checkMoveWatches(wd, wdObj);
                }

                if (mask & (IN_DELETE_SELF | IN_MOVE_SELF | IN_UNMOUNT)) {
                    recheckNonexistingMonitors(wd, wdObj);
                }

                if (mask & IN_ISDIR) {
                    if (mask & (IN_CREATE | IN_MOVED_TO)) {
                        recheckNonexistingMonitors(wd, wdObj);
                    }

                    if (adir != nullptr && adir->getRecursive()) {
                        if (mask & IN_CREATE) {
                            if (adir->getHidden() || name.at(0) != '.') {
                                log_debug("Detected new dir, adding to inotify: {}", path.c_str());
                                auto dirEnt = fs::directory_entry(path, ec);
                                if (!ec) {
                                    monitorUnmonitorRecursive(dirEnt, false, adir, false, config->getBoolOption(CFG_IMPORT_FOLLOW_SYMLINKS));
                                } else {
                                    log_error("Failed to read {}: {}", path.c_str(), ec.message());
                                }
                            } else {
                                log_debug("Detected new dir, irgnoring because it's hidden: {}", path.c_str());
                            }
                        }
                    }
                }

                if (adir != nullptr && mask & (IN_DELETE | IN_DELETE_SELF | IN_MOVE_SELF | IN_CLOSE_WRITE | IN_MOVED_FROM | IN_MOVED_TO | IN_UNMOUNT | IN_CREATE)) {
                    if (!(mask & (IN_MOVED_TO | IN_CREATE))) {
                        log_debug("deleting {}", path.c_str());

                        if (mask & (IN_DELETE_SELF | IN_MOVE_SELF | IN_UNMOUNT)) {
                            if (mask & IN_MOVE_SELF)
                                inotify->removeWatch(wd);
                            auto watch = getStartPoint(wdObj);
                            if (watch != nullptr) {
                                if (adir->persistent()) {
                                    monitorNonexisting(path, watch->getAutoscanDirectory());
                                    content->handlePeristentAutoscanRemove(adir);
                                }
                            }
                        }

                        int objectID = database->findObjectIDByPath(path, !(mask & IN_ISDIR));
                        if (objectID != INVALID_OBJECT_ID)
                            content->removeObject(adir, objectID, !(mask & IN_MOVED_TO));
                    }
                    if (mask & (IN_CLOSE_WRITE | IN_MOVED_TO | IN_CREATE)) {
                        log_debug("Adding {}", path.c_str());
                        auto dirEnt = fs::directory_entry(path, ec);
                        if (!ec) {
                            AutoScanSetting asSetting;
                            asSetting.adir = adir;
                            asSetting.followSymlinks = config->getBoolOption(CFG_IMPORT_FOLLOW_SYMLINKS);
                            asSetting.recursive = adir->getRecursive();
                            asSetting.hidden = adir->getHidden();
                            asSetting.rescanResource = true;
                            asSetting.mergeOptions(config, path);
                            // path, recursive, async, hidden, rescanResource, low priority, cancellable
                            content->addFile(dirEnt, adir->getLocation(), asSetting, true, true, false);
                            if (mask & IN_ISDIR) {
                                monitorUnmonitorRecursive(dirEnt, false, adir, false, asSetting.followSymlinks);
                            }
                        } else {
                            log_error("Failed to read {}: {}", path.c_str(), ec.message());
                        }
                    }
                }
                if (mask & IN_IGNORED) {
                    removeWatchMoves(wd);
                    removeDescendants(wd);
                    watches->erase(wd);
                }
            }
        } catch (const std::runtime_error& e) {
            log_error("Inotify thread caught exception: {}", e.what());
        }
    }
}

void AutoscanInotify::monitor(const std::shared_ptr<AutoscanDirectory>& dir)
{
    assert(dir->getScanMode() == ScanMode::INotify);
    log_debug("Requested to monitor \"{}\"", dir->getLocation().c_str());
    AutoLock lock(mutex);
    monitorQueue.push(dir);
    inotify->stop();
}

void AutoscanInotify::unmonitor(const std::shared_ptr<AutoscanDirectory>& dir)
{
    // must not be persistent
    assert(!dir->persistent());

    log_debug("Requested to stop monitoring \"{}\"", dir->getLocation().c_str());
    AutoLock lock(mutex);
    unmonitorQueue.push(dir);
    inotify->stop();
}

int AutoscanInotify::watchPathForMoves(const fs::path& path, int wd)
{
    int parentWd = INOTIFY_ROOT;

    fs::path watchPath;
    for (auto it = path.begin(); it != std::prev(path.end()); ++it) {
        auto&& p = *it;
        watchPath /= p;
        log_debug("adding move watch: {}", watchPath.c_str());
        parentWd = addMoveWatch(watchPath, wd, parentWd);
    }

    return parentWd;
}

int AutoscanInotify::addMoveWatch(const fs::path& path, int removeWd, int parentWd)
{
    int wd = inotify->addWatch(path, events);
    if (wd >= 0) {
        bool alreadyThere = false;

        std::shared_ptr<Wd> wdObj = nullptr;
        try {
            wdObj = watches->at(wd);

            int parentWdSet = wdObj->getParentWd();
            if (parentWdSet >= 0) {
                if (parentWd != parentWdSet) {
                    log_debug("error: parentWd doesn't match wd: {}, parent is: {}, should be: {}", wd, parentWdSet, parentWd);
                    wdObj->setParentWd(parentWd);
                }
            } else
                wdObj->setParentWd(parentWd);

            //find
            //alreadyThere =...
            //FIXME: not finished?

        } catch (const std::out_of_range& ex) {
            wdObj = std::make_shared<Wd>(path, wd, parentWd);
            watches->emplace(wd, wdObj);
        }

        if (!alreadyThere) {
            auto watch = std::make_shared<WatchMove>(removeWd);
            wdObj->addWatch(watch);
        }
    }
    return wd;
}

void AutoscanInotify::monitorNonexisting(const fs::path& path, const std::shared_ptr<AutoscanDirectory>& adir)
{
    auto pathAr = splitString(path, DIR_SEPARATOR);
    recheckNonexistingMonitor(-1, pathAr, adir);
}

void AutoscanInotify::recheckNonexistingMonitor(int curWd, const std::vector<std::string>& pathAr, const std::shared_ptr<AutoscanDirectory>& adir)
{
    bool first = true;
    for (size_t i = pathAr.size() + 1; i > 0;) {
        i--;
        std::ostringstream buf;
        if (i == 0)
            buf << DIR_SEPARATOR;
        else {
            for (size_t j = 0; j < i; j++) {
                buf << DIR_SEPARATOR << pathAr.at(j);
                //                log_debug("adding: {}", pathAr->get(j)->data);
            }
        }

        fs::path path = buf.str();
        bool pathExists = fs::is_directory(path);
        //        log_debug("checking {}: {}", path.c_str(), pathExists);
        if (pathExists) {
            if (curWd != -1)
                removeNonexistingMonitor(curWd, watches->at(curWd), pathAr);
            if (first) {
                monitorDirectory(path, adir, true);
                content->handlePersistentAutoscanRecreate(adir);
            } else {
                monitorDirectory(path, adir, false, &pathAr);
            }
            break;
        }
        if (first)
            first = false;
    }
}

void AutoscanInotify::checkMoveWatches(int wd, const std::shared_ptr<Wd>& wdObj)
{
    auto wdWatches = wdObj->getWdWatches();
    for (auto it = wdWatches->begin(); it != wdWatches->end(); /*++it*/) {
        auto& watchToCheck = *it;
        if (watchToCheck->getType() == WatchType::Move) {
            if (wdWatches->size() == 1) {
                inotify->removeWatch(wd);
                ++it;
            } else {
                it = wdWatches->erase(it);
            }

            auto watchMv = std::static_pointer_cast<WatchMove>(watchToCheck);
            int removeWd = watchMv->getRemoveWd();
            try {
                auto wdToRemove = watches->at(removeWd);

                recheckNonexistingMonitors(removeWd, wdToRemove);

                fs::path path = wdToRemove->getPath();
                log_debug("found wd to remove because of move event: {} {}", removeWd, path.c_str());

                inotify->removeWatch(removeWd);
                auto watchToRemove = getStartPoint(wdToRemove);
                if (watchToRemove != nullptr) {
                    std::shared_ptr<AutoscanDirectory> adir = watchToRemove->getAutoscanDirectory();
                    if (adir->persistent()) {
                        monitorNonexisting(path, adir);
                        content->handlePeristentAutoscanRemove(adir);
                    }

                    int objectID = database->findObjectIDByPath(path, true);
                    if (objectID != INVALID_OBJECT_ID)
                        content->removeObject(adir, objectID, false);
                }
            } catch (const std::out_of_range& ex) {
            } // Not found in map
        } else
            ++it;
    }
}

void AutoscanInotify::recheckNonexistingMonitors(int wd, const std::shared_ptr<Wd>& wdObj)
{
    auto wdWatches = wdObj->getWdWatches();
    for (auto&& watch : *wdWatches) {
        if (watch->getType() == WatchType::Autoscan) {
            auto watchAs = std::static_pointer_cast<WatchAutoscan>(watch);
            auto pathAr = watchAs->getNonexistingPathArray();
            if (!pathAr.empty()) {
                recheckNonexistingMonitor(wd, pathAr, watchAs->getAutoscanDirectory());
            }
        }
    }
}

void AutoscanInotify::removeNonexistingMonitor(int wd, const std::shared_ptr<Wd>& wdObj, const std::vector<std::string>& pathAr)
{
    auto wdWatches = wdObj->getWdWatches();
    for (auto it = wdWatches->begin(); it != wdWatches->end(); /*++it*/) {
        auto& watch = *it;
        if (watch->getType() == WatchType::Autoscan) {
            auto watchAs = std::static_pointer_cast<WatchAutoscan>(watch);
            if (watchAs->getNonexistingPathArray() == pathAr) {
                if (wdWatches->size() == 1) {
                    // should be done automatically, because removeWatch triggers an IGNORED event
                    //watches->remove(wd);

                    inotify->removeWatch(wd);
                    ++it;
                } else {
                    it = wdWatches->erase(it);
                }
                return;
            }
        } else
            ++it;
    }
}

void AutoscanInotify::monitorUnmonitorRecursive(const fs::directory_entry& startPath, bool unmonitor, const std::shared_ptr<AutoscanDirectory>& adir, bool startPoint, bool followSymlinks)
{
    if (unmonitor)
        unmonitorDirectory(startPath.path(), adir);
    else {
        bool ok = (monitorDirectory(startPath.path(), adir, startPoint) > 0);
        if (!ok)
            return;
    }

    std::error_code ec;
    if (!startPath.exists(ec) || !startPath.is_directory(ec)) {
        log_warning("Could not open {}: {}", startPath.path().c_str(), ec.message());
        return;
    }
    auto dIter = fs::directory_iterator(startPath, ec);
    if (ec) {
        log_error("monitorUnmonitorRecursive: Failed to iterate {}, {}", startPath.path().c_str(), ec.message());
        return;
    }
    for (auto&& dirEnt : dIter) {
        if (shutdownFlag)
            break;

        if (!followSymlinks && dirEnt.is_symlink(ec)) {
            log_debug("link {} skipped", dirEnt.path().c_str());
            continue;
        }

        if (dirEnt.is_directory(ec) && adir->getRecursive()) {
            monitorUnmonitorRecursive(dirEnt, unmonitor, adir, false, followSymlinks);
        }

        if (ec) {
            log_error("monitorUnmonitorRecursive: Failed to read {}, {}", dirEnt.path().c_str(), ec.message());
        }
    }
}

int AutoscanInotify::monitorDirectory(const fs::path& path, const std::shared_ptr<AutoscanDirectory>& adir, bool startPoint, const std::vector<std::string>* pathArray)
{
    int wd = inotify->addWatch(path, events);
    if (wd < 0) {
        if (startPoint && adir->persistent()) {
            monitorNonexisting(path, adir);
        }
    } else {
        bool alreadyWatching = false;
        int parentWd = INOTIFY_UNKNOWN_PARENT_WD;
        if (startPoint)
            parentWd = watchPathForMoves(path, wd);

        std::shared_ptr<Wd> wdObj = nullptr;
        try {
            wdObj = watches->at(wd);
            if (parentWd >= 0 && wdObj->getParentWd() < 0) {
                wdObj->setParentWd(parentWd);
            }

            if (pathArray == nullptr)
                alreadyWatching = (getAppropriateAutoscan(wdObj, adir) != nullptr);

            // should we check for already existing "nonexisting" watches?
            // ...
        } catch (const std::out_of_range& ex) {
            wdObj = std::make_shared<Wd>(path, wd, parentWd);
            watches->emplace(wd, wdObj);
        }

        if (!alreadyWatching) {
            auto watch = std::make_shared<WatchAutoscan>(startPoint, adir);
            if (pathArray != nullptr) {
                watch->setNonexistingPathArray(*pathArray);
            }
            wdObj->addWatch(watch);

            if (!startPoint) {
                int startPointWd = inotify->addWatch(adir->getLocation(), events);
                log_debug("getting start point for {} -> {} wd={}", path.c_str(), adir->getLocation().c_str(), startPointWd);
                if (wd >= 0)
                    addDescendant(startPointWd, wd, adir);
            }
        }
    }
    return wd;
}

void AutoscanInotify::unmonitorDirectory(const fs::path& path, const std::shared_ptr<AutoscanDirectory>& adir)
{
    // maybe there is a faster method...
    // we use addWatch, because it returns the wd to the filename
    // this should not add a new watch, because it should be already watched
    int wd = inotify->addWatch(path, events);

    if (wd < 0) {
        // doesn't seem to be monitored currently
        log_debug("unmonitorDirectory called, but it isn't monitored? ({})", path.c_str());
        return;
    }

    auto wdObj = watches->at(wd);
    if (wdObj == nullptr) {
        log_error("wd not found in watches!? ({}, {})", wd, path.c_str());
        return;
    }

    auto watchAs = getAppropriateAutoscan(wdObj, adir);
    if (watchAs == nullptr) {
        log_debug("autoscan not found in watches? ({}, {})", wd, path.c_str());
    } else {
        if (wdObj->getWdWatches()->size() == 1) {
            // should be done automatically, because removeWatch triggers an IGNORED event
            //watches->remove(wd);

            inotify->removeWatch(wd);
        } else {
            removeFromWdObj(wdObj, watchAs);
        }
    }
}

std::shared_ptr<AutoscanInotify::WatchAutoscan> AutoscanInotify::getAppropriateAutoscan(const std::shared_ptr<Wd>& wdObj, const std::shared_ptr<AutoscanDirectory>& adir)
{
    auto wdWatches = wdObj->getWdWatches();
    for (auto&& watch : *wdWatches) {
        if (watch->getType() == WatchType::Autoscan) {
            auto watchAs = std::static_pointer_cast<WatchAutoscan>(watch);
            if (watchAs->getNonexistingPathArray().empty()) {
                if (watchAs->getAutoscanDirectory()->getLocation() == adir->getLocation()) {
                    return watchAs;
                }
            }
        }
    }
    return nullptr;
}

std::shared_ptr<AutoscanInotify::WatchAutoscan> AutoscanInotify::getAppropriateAutoscan(const std::shared_ptr<Wd>& wdObj, const fs::path& path)
{
    fs::path pathBestMatch;
    std::shared_ptr<WatchAutoscan> bestMatch = nullptr;
    auto wdWatches = wdObj->getWdWatches();
    for (auto&& watch : *wdWatches) {
        if (watch->getType() == WatchType::Autoscan) {
            auto watchAs = std::static_pointer_cast<WatchAutoscan>(watch);
            if (watchAs->getNonexistingPathArray().empty()) {
                fs::path testLocation = watchAs->getAutoscanDirectory()->getLocation();
                if (startswith(path, testLocation)) {
                    if (!pathBestMatch.empty()) {
                        if (pathBestMatch.string().length() < testLocation.string().length()) {
                            pathBestMatch = testLocation;
                            bestMatch = watchAs;
                        }
                    } else {
                        pathBestMatch = testLocation;
                        bestMatch = watchAs;
                    }
                }
            }
        }
    }
    return bestMatch;
}

void AutoscanInotify::removeWatchMoves(int wd)
{
    bool first = true;
    int checkWd = wd;
    do {
        std::shared_ptr<Wd> wdObj = nullptr;
        try {
            wdObj = watches->at(checkWd);
        } catch (const std::out_of_range& ex) {
            break;
        }

        auto wdWatches = wdObj->getWdWatches();
        if (wdWatches->empty())
            break;

        if (first) {
            first = false;
        } else {
            for (auto it = wdWatches->begin(); it != wdWatches->end(); /*++it*/) {
                auto& watch = *it;
                if (watch->getType() == WatchType::Move) {
                    auto watchMv = std::static_pointer_cast<WatchMove>(watch);
                    if (watchMv->getRemoveWd() == wd) {
                        log_debug("removing watch move");
                        if (wdWatches->size() == 1) {
                            inotify->removeWatch(checkWd);
                            ++it;
                        } else
                            it = wdWatches->erase(it);
                    } else
                        ++it;
                }
            }
        }
        checkWd = wdObj->getParentWd();
    } while (checkWd >= 0);
}

bool AutoscanInotify::removeFromWdObj(const std::shared_ptr<Wd>& wdObj, const std::shared_ptr<Watch>& toRemove)
{
    auto wdWatches = wdObj->getWdWatches();
    for (auto it = wdWatches->begin(); it != wdWatches->end(); /*++it*/) {
        auto& watch = *it;
        if (watch == toRemove) {
            if (wdWatches->size() == 1) {
                inotify->removeWatch(wdObj->getWd());
                ++it;
            } else
                it = wdWatches->erase(it);
            return true;
        }
        ++it;
    }
    return false;
}

std::shared_ptr<AutoscanInotify::WatchAutoscan> AutoscanInotify::getStartPoint(const std::shared_ptr<Wd>& wdObj)
{
    auto wdWatches = wdObj->getWdWatches();
    for (auto&& watch : *wdWatches) {
        if (watch->getType() == WatchType::Autoscan) {
            auto watchAs = std::static_pointer_cast<WatchAutoscan>(watch);
            if (watchAs->isStartPoint())
                return watchAs;
        }
    }
    return nullptr;
}

void AutoscanInotify::addDescendant(int startPointWd, int addWd, const std::shared_ptr<AutoscanDirectory>& adir)
{
    //    log_debug("called for {}, (adir->path={}); adding {}", startPointWd, adir->getLocation().c_str(), addWd);
    std::shared_ptr<Wd> wdObj = nullptr;
    try {
        wdObj = watches->at(startPointWd);
    } catch (const std::out_of_range& ex) {
        return;
    }
    if (wdObj == nullptr)
        return;

    //   log_debug("found wdObj");
    auto watch = getAppropriateAutoscan(wdObj, adir);
    if (watch == nullptr)
        return;
    //    log_debug("adding descendant");
    watch->addDescendant(addWd);
    //    log_debug("added descendant to {} (adir->path={}): {}; now: {}", startPointWd, adir->getLocation().c_str(), addWd, watch->getDescendants()->toCSV().c_str());
}

void AutoscanInotify::removeDescendants(int wd)
{
    std::shared_ptr<Wd> wdObj = nullptr;
    try {
        wdObj = watches->at(wd);
    } catch (const std::out_of_range& ex) {
        return;
    }

    auto wdWatches = wdObj->getWdWatches();
    for (auto&& watch : *wdWatches) {
        if (watch->getType() == WatchType::Autoscan) {
            auto watchAs = std::static_pointer_cast<WatchAutoscan>(watch);
            for (int descWd : watchAs->getDescendants()) {
                inotify->removeWatch(descWd);
            }
        }
    }
}

#endif // HAVE_INOTIFY
