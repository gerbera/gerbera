/*MT*

    MediaTomb - http://www.mediatomb.cc/

    autoscan_inotify.cc - this file is part of MediaTomb.

    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>

    Copyright (C) 2006-2010 Gena Batyan <bgeradz@mediatomb.cc>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>,
                            Leonhard Wimmer <leo@mediatomb.cc>

    Copyright (C) 2016-2023 Gerbera Contributors

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
#define LOG_FAC log_facility_t::autoscan

#ifdef HAVE_INOTIFY
#include "autoscan_inotify.h" // API

#include <sstream>

#include "content_manager.h"
#include "database/database.h"
#include "util/tools.h"

#define INOTIFY_MAX_USER_WATCHES_FILE "/proc/sys/fs/inotify/max_user_watches"

AutoscanInotify::AutoscanInotify(const std::shared_ptr<ContentManager>& content)
    : config(content->getContext()->getConfig())
    , database(content->getContext()->getDatabase())
    , content(content)
{
    std::error_code ec;
    if (isRegularFile(INOTIFY_MAX_USER_WATCHES_FILE, ec)) {
        try {
            [[maybe_unused]] int maxWatches = std::stoi(trimString(GrbFile(INOTIFY_MAX_USER_WATCHES_FILE).readTextFile()));
            log_debug("Max watches on the system: {}", maxWatches);
        } catch (const std::runtime_error& ex) {
            log_error("Could not determine maximum number of inotify user watches: {}", ex.what());
        }
    }

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
        watches.clear();
    }
}

void AutoscanInotify::run()
{
    AutoLock lock(mutex);

    if (shutdownFlag) {
        shutdownFlag = false;
        inotify = std::make_unique<Inotify>();
        thread_ = std::thread([this] { threadProc(); });
    }
}

/// \brief main proc for thread
void AutoscanInotify::threadProc()
{
    std::error_code ec;
    while (!shutdownFlag) {
        try {
            std::unique_lock<std::mutex> lock(mutex);

            //  remove old dirs
            while (!unmonitorQueue.empty()) {
                auto adir = std::move(unmonitorQueue.front());
                unmonitorQueue.pop();
                lock.unlock();

                if (!adir) {
                    log_debug("Empty autoscan");
                    lock.lock();
                    continue;
                }

                const fs::path& location = adir->getLocation();
                if (location.empty()) {
                    lock.lock();
                    continue;
                }
                // read dir
                auto dirEnt = fs::directory_entry(location, ec);

                if (adir->getRecursive()) {
                    log_debug("Removing recursive watch: {}", location.c_str());
                    monitorUnmonitorRecursive(dirEnt, true, adir, true, adir->getFollowSymlinks());
                } else {
                    log_debug("Removing non-recursive watch: {}", location.c_str());
                    unmonitorDirectory(location, adir);
                }

                lock.lock();
            }

            // monitor new dir
            while (!monitorQueue.empty()) {
                auto adir = std::move(monitorQueue.front());
                monitorQueue.pop();

                lock.unlock();

                if (!adir) {
                    log_debug("Empty autoscan");
                    lock.lock();
                    continue;
                }

                // read dir
                const fs::path& location = adir->getLocation();
                if (location.empty()) {
                    lock.lock();
                    continue;
                }

                auto dirEnt = fs::directory_entry(location, ec);
                if (!ec) {
                    // handle dir recursively
                    if (adir->getRecursive()) {
                        log_debug("Adding recursive watch: {}", location.c_str());
                        monitorUnmonitorRecursive(dirEnt, false, adir, true, adir->getFollowSymlinks());
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
                // lock.lock();
                int wd = event->wd;
                uint32_t mask = event->mask;
                std::string name = event->len > 0 ? event->name : "";
                log_debug("inotify event: {} mask=0x{:x} name={}", wd, mask, name);

                std::shared_ptr<Wd> wdObj;
                try {
                    wdObj = watches.at(wd);
                } catch (const std::out_of_range&) {
                    inotify->removeWatch(wd);
                    // lock.unlock();
                    continue;
                }

                fs::path path = wdObj->getPath();
                // file is not gone
                if (mask & IN_MOVE_SELF)
                    path = path.parent_path() / name;
                else if (!(mask & (IN_DELETE_SELF | IN_MOVE_SELF | IN_UNMOUNT)) && !name.empty())
                    path /= name;

                auto dirEnt = fs::directory_entry(path, ec);
                bool isDir = mask & IN_ISDIR;
                if (!ec) {
                    isDir = isDir || dirEnt.is_directory(ec);
                }
                if (ec && !(mask & (IN_DELETE_SELF | IN_DELETE | IN_MOVED_FROM | IN_MOVE_SELF | IN_UNMOUNT))) {
                    log_error("Failed to read {}: {}", path.c_str(), ec.message());
                }
                std::shared_ptr<AutoscanDirectory> adir;
                auto watchAs = getAppropriateAutoscan(wdObj, path);
                if (watchAs)
                    adir = watchAs->getAutoscanDirectory();
                if (!watchAs || !adir) {
                    log_debug("autoscan not found in watches? ({}, watchAs:{}, adir:{}, {})", wd, watchAs == nullptr, adir == nullptr, path.c_str());
                }

                // file is renamed
                if (mask & IN_MOVE_SELF) {
                    checkMoveWatches(wd, wdObj);
                }

                // file is deleted
                if (mask & (IN_DELETE_SELF | IN_MOVE_SELF | IN_UNMOUNT)) {
                    recheckNonexistingMonitors(wd, wdObj);
                }
                AutoScanSetting asSetting;
                asSetting.adir = adir;
                asSetting.followSymlinks = adir->getFollowSymlinks();
                asSetting.recursive = adir->getRecursive();
                asSetting.hidden = adir->getHidden();
                asSetting.rescanResource = true;
                asSetting.async = true;
                asSetting.mergeOptions(config, path);

                // file is directory
                if (isDir) {
                    if (mask & (IN_CREATE | IN_MOVED_TO)) {
                        recheckNonexistingMonitors(wd, wdObj);
                    }

                    if (adir && adir->getRecursive() && (mask & IN_CREATE)) {
                        if (!content->isHiddenFile(dirEnt, isDir, asSetting)) {
                            log_debug("Detected new dir, adding to inotify: {}", path.c_str());
                            monitorUnmonitorRecursive(dirEnt, false, adir, false, asSetting.followSymlinks);
                        } else {
                            log_debug("Detected new dir, ignoring because it's hidden: {}", path.c_str());
                        }
                    }
                }

                // changed
                if (adir && (mask & (IN_DELETE | IN_DELETE_SELF | IN_MOVE_SELF | IN_CLOSE_WRITE | IN_MOVED_FROM | IN_MOVED_TO | IN_UNMOUNT | IN_CREATE))) {
                    // not new
                    if (!(mask & (IN_MOVED_TO | IN_CREATE))) {
                        log_debug("deleting {}", path.c_str());

                        // deleted
                        if (mask & (IN_DELETE_SELF | IN_MOVE_SELF | IN_UNMOUNT)) {
                            if (!(mask & IN_MOVE_SELF)) {
                                log_debug("removing watch {}", path.c_str());
                                inotify->removeWatch(wd);
                            }
                            auto watch = getStartPoint(wdObj);
                            if (watch && adir->persistent()) {
                                monitorNonexisting(path, watch->getAutoscanDirectory());
                                content->handlePeristentAutoscanRemove(adir);
                            }
                        }

                        auto object = database->findObjectByPath(path, !(mask & IN_ISDIR) ? DbFileType::File : DbFileType::Directory);
                        if (object)
                            content->removeObject(adir, object, path, !(mask & IN_MOVED_TO));
                    }
                    // new file
                    if (mask & (IN_CLOSE_WRITE | IN_MOVED_TO | IN_CREATE)) {
                        log_debug("Adding {}", path.c_str());
                        // dirEnt, path, rootPath, settings, lowPriority, cancellable
                        content->addFile(dirEnt, adir->getLocation(), asSetting, true, false);
                        if (isDir) {
                            int wdPath = monitorUnmonitorRecursive(dirEnt, false, adir, false, asSetting.followSymlinks);
                            if ((mask & IN_MOVED_TO) && wdPath > -1) {
                                auto wdObjPath = watches.at(wdPath);
                                log_debug("Resetting {} to {}", wdObjPath->getPath().c_str(), path.c_str());
                                wdObjPath->setPath(path);
                            }
                        }
                    }
                }
                if (mask & IN_IGNORED) {
                    removeWatchMoves(wd);
                    removeDescendants(wd);
                    watches.erase(wd);
                }
                // lock.unlock();
            }
        } catch (const std::runtime_error& e) {
            log_error("Inotify thread caught exception: {}", e.what());
        }
    }
}

void AutoscanInotify::monitor(const std::shared_ptr<AutoscanDirectory>& dir)
{
    assert(dir->getScanMode() == AutoscanScanMode::INotify);
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
        watchPath /= *it;
        log_debug("adding move watch: {}", watchPath.c_str());
        parentWd = addMoveWatch(watchPath, wd, parentWd);
    }

    return parentWd;
}

int AutoscanInotify::addMoveWatch(const fs::path& path, int removeWd, int parentWd)
{
    int wd = inotify->addWatch(path, events);
    if (wd >= 0) {
        std::shared_ptr<Wd> wdObj;
        try {
            // find
            wdObj = watches.at(wd);

            // alreadyThere
            int parentWdSet = wdObj->getParentWd();
            if (parentWdSet >= 0) {
                if (parentWd != parentWdSet) {
                    log_debug("error: parentWd doesn't match wd: {}, parent is: {}, should be: {}", wd, parentWdSet, parentWd);
                    wdObj->setParentWd(parentWd);
                }
            } else
                wdObj->setParentWd(parentWd);
        } catch (const std::out_of_range&) {
            // add new wstch
            wdObj = std::make_shared<Wd>(path, wd, parentWd);
            watches.emplace(wd, wdObj);
        }

        // add move watch
        auto watch = std::make_shared<WatchMove>(removeWd);
        wdObj->addWatch(move(watch));
    }
    return wd;
}

void AutoscanInotify::monitorNonexisting(const fs::path& path, const std::shared_ptr<AutoscanDirectory>& adir)
{
    auto pathAr = splitString(path.string(), DIR_SEPARATOR);
    recheckNonexistingMonitor(-1, pathAr, adir);
}

void AutoscanInotify::recheckNonexistingMonitor(int curWd, const std::vector<std::string>& pathAr, const std::shared_ptr<AutoscanDirectory>& adir)
{
    bool first = true;
    for (std::size_t i = pathAr.size() + 1; i > 0;) {
        i--;
        std::ostringstream buf;
        if (i == 0)
            buf << DIR_SEPARATOR;
        else {
            for (std::size_t j = 0; j < i; j++) {
                buf << DIR_SEPARATOR << pathAr.at(j);
                //                log_debug("adding: {}", pathAr->get(j)->data);
            }
        }

        fs::path path = buf.str();
        bool pathExists = fs::is_directory(path);
        //        log_debug("checking {}: {}", path.c_str(), pathExists);
        if (pathExists) {
            if (curWd != -1)
                removeNonexistingMonitor(curWd, watches.at(curWd), pathAr);
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
    auto&& wdWatches = wdObj->getWdWatches();
    for (auto it = wdWatches->begin(); it != wdWatches->end(); /*++it*/) {
        if ((*it)->getType() == WatchType::Move) {
            if (wdWatches->size() == 1) {
                inotify->removeWatch(wd);
                ++it;
            } else {
                it = wdWatches->erase(it);
            }

            auto watchMv = std::static_pointer_cast<WatchMove>(*it);
            int removeWd = watchMv->getRemoveWd();
            try {
                auto wdToRemove = watches.at(removeWd);

                recheckNonexistingMonitors(removeWd, wdToRemove);

                fs::path path = wdToRemove->getPath();
                log_debug("found wd to remove because of move event: {} {}", removeWd, path.c_str());

                inotify->removeWatch(removeWd);
                auto watchToRemove = getStartPoint(wdToRemove);
                if (watchToRemove) {
                    auto adir = watchToRemove->getAutoscanDirectory();
                    if (adir->persistent()) {
                        monitorNonexisting(path, adir);
                        content->handlePeristentAutoscanRemove(adir);
                    }

                    auto object = database->findObjectByPath(path, DbFileType::File);
                    if (object)
                        content->removeObject(adir, object, path, false);
                }
            } catch (const std::out_of_range&) {
            } // Not found in map
        } else
            ++it;
    }
}

void AutoscanInotify::recheckNonexistingMonitors(int wd, const std::shared_ptr<Wd>& wdObj)
{
    auto&& wdWatches = wdObj->getWdWatches();
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
    auto&& wdWatches = wdObj->getWdWatches();
    auto it = std::find_if(wdWatches->begin(), wdWatches->end(),
        [&](auto&& watch) {
            if (watch->getType() == WatchType::Autoscan) {
                auto watchAs = std::static_pointer_cast<WatchAutoscan>(watch);
                return watchAs->getNonexistingPathArray() == pathAr;
            }
            return false; });
    if (it != wdWatches->end()) {
        if (wdWatches->size() == 1) {
            // removeWatch triggers an IN_IGNORED event so watches.erase(wd) is called in threadProc
            inotify->removeWatch(wd);
        } else {
            wdWatches->erase(it);
        }
    }
}

int AutoscanInotify::monitorUnmonitorRecursive(const fs::directory_entry& startPath, bool unmonitor, const std::shared_ptr<AutoscanDirectory>& adir, bool startPoint, bool followSymlinks)
{
    int result = -1;
    if (unmonitor)
        unmonitorDirectory(startPath.path(), adir);
    else {
        result = monitorDirectory(startPath.path(), adir, startPoint);
        bool ok = (result > 0);
        if (!ok)
            return -1;
    }

    std::error_code ec;
    if (!startPath.exists(ec) || !startPath.is_directory(ec)) {
        log_warning("Could not open {}: {}", startPath.path().c_str(), ec.message());
        return -1;
    }
    auto dIter = fs::directory_iterator(startPath, ec);
    if (ec) {
        log_error("monitorUnmonitorRecursive: Failed to iterate {}, {}", startPath.path().c_str(), ec.message());
        return -1;
    }
    for (auto&& dirEnt : dIter) {
        if (shutdownFlag)
            break;

        AutoScanSetting asSetting;
        asSetting.adir = adir;
        asSetting.followSymlinks = adir->getFollowSymlinks();
        asSetting.recursive = adir->getRecursive();
        asSetting.hidden = adir->getHidden();
        asSetting.rescanResource = true;
        asSetting.async = true;
        asSetting.mergeOptions(config, dirEnt.path());

        if (content->isHiddenFile(dirEnt, dirEnt.is_directory(ec), asSetting)) {
            log_debug("Hidden file {} skipped", dirEnt.path().c_str());
            continue;
        }

        if (dirEnt.is_directory(ec) && adir->getRecursive()) {
            monitorUnmonitorRecursive(dirEnt, unmonitor, adir, false, followSymlinks);
        }

        if (ec) {
            log_error("AutoscanInotify::monitorUnmonitorRecursive {}: Failed to read {}, {}", startPath.path().c_str(), dirEnt.path().c_str(), ec.message());
        }
    }
    return result;
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

        std::shared_ptr<Wd> wdObj;
        try {
            wdObj = watches.at(wd);
            if (parentWd >= 0 && wdObj->getParentWd() < 0) {
                wdObj->setParentWd(parentWd);
            }

            if (!pathArray)
                alreadyWatching = getAppropriateAutoscan(wdObj, adir) != nullptr;

            // should we check for already existing "nonexisting" watches?
            // ...
        } catch (const std::out_of_range&) {
            wdObj = std::make_shared<Wd>(path, wd, parentWd);
            watches.emplace(wd, wdObj);
        }

        if (!alreadyWatching) {
            auto watch = std::make_shared<WatchAutoscan>(startPoint, adir);
            if (pathArray) {
                watch->setNonexistingPathArray(*pathArray);
            }
            wdObj->addWatch(std::move(watch));

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

    auto wdObj = watches.find(wd);
    if (wdObj == watches.end()) {
        log_error("wd not found in watches!? ({}, {})", wd, path.c_str());
        return;
    }

    auto watchAs = getAppropriateAutoscan(wdObj->second, adir);
    if (!watchAs) {
        log_debug("autoscan not found in watches? ({}, {})", wd, path.c_str());
    } else {
        if (wdObj->second->getWdWatches()->size() == 1) {
            // should be done automatically, because removeWatch triggers an IGNORED event
            // watches.remove(wd);

            inotify->removeWatch(wd);
        } else {
            removeFromWdObj(wdObj->second, watchAs);
        }
    }
}

std::shared_ptr<AutoscanInotify::WatchAutoscan> AutoscanInotify::getAppropriateAutoscan(const std::shared_ptr<Wd>& wdObj, const std::shared_ptr<AutoscanDirectory>& adir)
{
    auto&& wdWatches = wdObj->getWdWatches();
    for (auto&& watch : *wdWatches) {
        if (watch->getType() == WatchType::Autoscan) {
            auto watchAs = std::static_pointer_cast<WatchAutoscan>(watch);
            if (watchAs->getNonexistingPathArray().empty() && watchAs->getAutoscanDirectory()->getLocation() == adir->getLocation()) {
                return watchAs;
            }
        }
    }
    return nullptr;
}

std::shared_ptr<AutoscanInotify::WatchAutoscan> AutoscanInotify::getAppropriateAutoscan(const std::shared_ptr<Wd>& wdObj, const fs::path& path)
{
    fs::path pathBestMatch;
    std::shared_ptr<WatchAutoscan> bestMatch;
    auto&& wdWatches = wdObj->getWdWatches();
    for (auto&& watch : *wdWatches) {
        if (watch->getType() == WatchType::Autoscan) {
            auto watchAs = std::static_pointer_cast<WatchAutoscan>(watch);
            if (watchAs->getNonexistingPathArray().empty()) {
                fs::path testLocation = watchAs->getAutoscanDirectory()->getLocation();
                if (isSubDir(path, testLocation)) {
                    if (!pathBestMatch.empty()) {
                        if (pathBestMatch < testLocation) {
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
        std::shared_ptr<Wd> wdObj;
        try {
            wdObj = watches.at(checkWd);
        } catch (const std::out_of_range&) {
            break;
        }

        auto&& wdWatches = wdObj->getWdWatches();
        if (wdWatches->empty())
            break;

        if (first) {
            first = false;
        } else {
            for (auto it = wdWatches->begin(); it != wdWatches->end(); /*++it*/) {
                if ((*it)->getType() == WatchType::Move) {
                    auto watchMv = std::static_pointer_cast<WatchMove>(*it);
                    if (watchMv->getRemoveWd() == wd) {
                        log_debug("removing watch move");
                        if (wdWatches->size() > 1) {
                            it = wdWatches->erase(it);
                            continue;
                        }
                        inotify->removeWatch(checkWd);
                    }
                    ++it;
                }
            }
        }
        checkWd = wdObj->getParentWd();
    } while (checkWd >= 0);
}

bool AutoscanInotify::removeFromWdObj(const std::shared_ptr<Wd>& wdObj, const std::shared_ptr<Watch>& toRemove)
{
    auto&& wdWatches = wdObj->getWdWatches();
    auto it = std::find(wdWatches->begin(), wdWatches->end(), toRemove);
    if (it != wdWatches->end()) {
        if (wdWatches->size() == 1) {
            inotify->removeWatch(wdObj->getWd());
        } else {
            wdWatches->erase(it);
        }
        return true;
    }

    return false;
}

std::shared_ptr<AutoscanInotify::WatchAutoscan> AutoscanInotify::getStartPoint(const std::shared_ptr<Wd>& wdObj)
{
    auto&& wdWatches = wdObj->getWdWatches();
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
    std::shared_ptr<Wd> wdObj;
    try {
        wdObj = watches.at(startPointWd);
    } catch (const std::out_of_range&) {
        return;
    }
    if (!wdObj)
        return;

    //   log_debug("found wdObj");
    auto watch = getAppropriateAutoscan(wdObj, adir);
    if (!watch)
        return;
    // log_debug("adding descendant");
    watch->addDescendant(addWd);
    // log_debug("added descendant to {} (adir->path={}): {}; now: {}", startPointWd, adir->getLocation(), addWd, fmt::join(*(watch->getDescendants()), ","));
}

void AutoscanInotify::removeDescendants(int wd)
{
    std::shared_ptr<Wd> wdObj;
    try {
        wdObj = watches.at(wd);
    } catch (const std::out_of_range&) {
        return;
    }

    auto&& wdWatches = wdObj->getWdWatches();
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
