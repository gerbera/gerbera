/*MT*

    MediaTomb - http://www.mediatomb.cc/

    autoscan_inotify.cc - this file is part of MediaTomb.

    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>

    Copyright (C) 2006-2010 Gena Batyan <bgeradz@mediatomb.cc>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>,
                            Leonhard Wimmer <leo@mediatomb.cc>

    Copyright (C) 2016-2026 Gerbera Contributors

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

/// @file content/inotify/autoscan_inotify.cc
/// @brief Implement manager class for autoscan directories with inotify
#define GRB_LOG_FAC GrbLogFacility::autoscan

#ifdef HAVE_INOTIFY
#include "autoscan_inotify.h" // API

#include "config/config_option_enum.h"
#include "config/config_val.h"
#include "config/result/autoscan.h"
#include "content/autoscan_setting.h"
#include "content/content.h"
#include "content/inotify/directory_watch.h"
#include "content/inotify/inotify_handler.h"
#include "content/inotify/inotify_manager_cc.h"
#include "content/inotify/watch.h"
#include "context.h"
#include "database/database.h"

template void InotifyManager<DirectoryWatch>::run();

AutoscanInotify::AutoscanInotify(const std::shared_ptr<Content>& content)
    : config(content->getContext()->getConfig())
    , database(content->getContext()->getDatabase())
    , content(content)
{
    defFollowSymlinks = this->config->getBoolOption(ConfigVal::IMPORT_FOLLOW_SYMLINKS);
    defHidden = this->config->getBoolOption(ConfigVal::IMPORT_HIDDEN_FILES);

    if (this->config->getBoolOption(ConfigVal::IMPORT_AUTOSCAN_INOTIFY_ATTRIB))
        events |= IN_ATTRIB;
}

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

                if (!adir)
                    log_debug("Empty autoscan");
                else if (!adir->getLocation().empty())
                    unmonitorDirectory(adir);

                lock.lock();
            }

            // monitor new dir
            while (!monitorQueue.empty()) {
                auto [adir, refreshExisting] = std::move(monitorQueue.front());
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
                    // Honors recursive=no while still watching direct files.
                    monitorRecursive(dirEnt, adir, true, adir->getFollowSymlinks());
                    content->rescanDirectory(adir, adir->getObjectID(), location, false, refreshExisting);
                } else {
                    log_error("Failed to read autoscan directory {}: {}", location.c_str(), ec.message());
                }

                lock.lock();
            }

            lock.unlock();

            /* --- get event --- (blocking) */
            inotify_event* event = inotify->nextEvent();
            /* --- */

            if (event && (event->mask & IN_Q_OVERFLOW)) {
                log_warning("Inotify queue overflow: rebuilding watches and reconciling monitored directories");
                auto directories = content->getAutoscanDirectories();
                auto replacement = std::make_unique<Inotify>();
                AutoLock watchLock(mutex);
                watches.clear();
                inotify.swap(replacement);
                for (const auto& adir : directories) {
                    if (adir->getScanMode() == AutoscanScanMode::INotify)
                        monitorQueue.emplace(adir, true);
                }
                continue;
            }

            if (event && (event->mask & (events | IN_IGNORED))) {
                auto handler = InotifyHandler(this, event, event->mask & (events | IN_IGNORED));
                auto wdObj = getWatch(handler);

                if (!wdObj)
                    continue;

                if (event->mask & IN_IGNORED) {
                    handler.doIgnored();
                    continue;
                }
                // The kernel returns the same wd only for the same watched object,
                // including another hard link. MASK_ADD preserves its event mask.
                auto currentWd = inotify->addWatch(wdObj->getPath(), IN_DELETE_SELF | IN_MASK_ADD);
                if (currentWd >= 0 && currentWd != event->wd) {
                    if (watches.find(currentWd) == watches.end())
                        inotify->removeWatch(currentWd);
                    log_debug("Ignoring event {} for replaced watch {} at {}", InotifyUtil::mapFlags(event->mask), event->wd, wdObj->getPath().c_str());
                    if (auto start = wdObj->getStartPoint())
                        monitor(start->getAutoscanDirectory(), true);
                    continue;
                }
                if (currentWd < 0) {
                    std::error_code statusError;
                    bool exists = fs::exists(wdObj->getPath(), statusError);
                    if (statusError && statusError != std::errc::no_such_file_or_directory && statusError != std::errc::not_a_directory)
                        throw fs::filesystem_error("Cannot verify inotify watch", wdObj->getPath(), statusError);
                    if (exists)
                        continue; // A failed identity check does not prove deletion.
                }

                fs::path path = handler.getPath(wdObj);
                auto [isDir, adir] = handler.getAutoscanDirectory(wdObj);

                handler.doMove(wdObj);

                AutoScanSetting asSetting;
                asSetting.adir = adir;
                asSetting.followSymlinks = adir ? adir->getFollowSymlinks() : defFollowSymlinks;
                asSetting.recursive = adir ? adir->getRecursive() : false;
                asSetting.hidden = adir ? adir->getHidden() : defHidden;
                asSetting.rescanResource = true;
                asSetting.async = true; // Share the content worker with rescans.
                asSetting.resourcePatterns.clear();
                asSetting.mergeOptions(config, path);

                // changed
                if (adir && handler.hasEvent()) {
                    // not new
                    int wd = handler.doExistingEntry(database, content, wdObj, adir->getLayoutMode(), isDir);
                    if (wd > INOTIFY_ROOT)
                        inotify->removeWatch(wd);
                    // new file
                    handler.doNewEntry(asSetting, content, isDir);
                }
                // target is directory
                if (isDir) {
                    handler.doDirectory(asSetting, content, wdObj);
                }
                handler.doIgnored();
                log_debug("end event {}", event->wd);
            }
        } catch (const std::runtime_error& e) {
            log_error("Inotify thread caught exception: {}", EX(e));
        }
    }
}

std::shared_ptr<DirectoryWatch> AutoscanInotify::getWatch(const InotifyHandler& handler)
{
    std::shared_ptr<DirectoryWatch> wdObj;
    try {
        wdObj = watches.at(handler.getWd());
    } catch (const std::out_of_range&) {
        inotify->removeWatch(handler.getWd());
    }
    return wdObj;
}

void AutoscanInotify::monitor(const std::shared_ptr<AutoscanDirectory>& dir, bool refreshExisting)
{
    assert(dir->getScanMode() == AutoscanScanMode::INotify);
    log_debug("Requested to monitor \"{}\"", dir->getLocation().c_str());
    AutoLock lock(mutex);
    monitorQueue.emplace(dir, refreshExisting);
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

int AutoscanInotify::watchPathForMoves(const fs::path& path, int wd, unsigned int retryCount)
{
    int parentWd = INOTIFY_ROOT;

    fs::path parent = path.parent_path();
    fs::path watchPath;
    for (const auto& it : parent) {
        watchPath /= it;
        log_debug("adding move watch: {}", watchPath.c_str());
        parentWd = addMoveWatch(watchPath, wd, parentWd, retryCount);
    }

    return parentWd;
}

int AutoscanInotify::addMoveWatch(const fs::path& path, int removeWd, int parentWd, unsigned int retryCount)
{
    int wd = inotify->addWatch(path, events, retryCount);
    if (wd > INOTIFY_ROOT) {
        std::shared_ptr<DirectoryWatch> wdObj;
        try {
            // find
            wdObj = watches.at(wd);

            // alreadyThere
            int parentWdSet = wdObj->getParentWd();
            if (parentWdSet > INOTIFY_ROOT) {
                if (parentWd != parentWdSet) {
                    log_debug("error: parentWd doesn't match wd: {}, parent is: {}, should be: {}", wd, parentWdSet, parentWd);
                    wdObj->setParentWd(parentWd);
                }
            } else
                wdObj->setParentWd(parentWd);
        } catch (const std::out_of_range&) {
            // add new wstch
            wdObj = std::make_shared<DirectoryWatch>(path, wd, parentWd);
            watches.emplace(wd, wdObj);
        }

        // add move watch
        auto watch = std::make_shared<WatchMove>(removeWd);
        wdObj->addWatch(std::move(watch));
    }
    return wd;
}

void AutoscanInotify::monitorNonexisting(const fs::path& path, const std::shared_ptr<AutoscanDirectory>& adir)
{
    recheckNonexistingMonitor(INOTIFY_ROOT, path, adir);
}

void AutoscanInotify::recheckNonexistingMonitor(int curWd, const fs::path& nonExistingPath, const std::shared_ptr<AutoscanDirectory>& adir)
{
    std::error_code ec;
    bool first = true;
    for (fs::path path = nonExistingPath; path != "/"; path = path.parent_path()) {
        bool pathExists = fs::is_directory(path, ec);
        log_debug("recheckNonexistingMonitor: Checking {}: {}", path.c_str(), pathExists);
        if (pathExists) {
            if (curWd > INOTIFY_ROOT)
                removeNonexistingMonitor(curWd, watches.at(curWd), nonExistingPath);
            if (first) {
                monitorDirectory(path, adir, true);
                content->handlePersistentAutoscanRecreate(adir);
            } else {
                monitorDirectory(path, adir, false, true, nonExistingPath);
            }
            break;
        }
        first = false;
    }
}

void AutoscanInotify::checkMoveWatches(int wd, const std::shared_ptr<DirectoryWatch>& wdObj)
{
    auto&& wdWatches = wdObj->getWdWatches();
    for (auto it = wdWatches.begin(); it != wdWatches.end(); /*++it*/) {
        if ((*it)->getType() == WatchType::Move) {
            auto watchMv = std::static_pointer_cast<WatchMove>(*it);
            int removeWd = watchMv->getRemoveWd();
            if (wdWatches.size() == 1) {
                inotify->removeWatch(wd);
                ++it;
            } else {
                it = wdWatches.erase(it);
            }

            try {
                auto wdToRemove = watches.at(removeWd);

                recheckNonexistingMonitors(removeWd, wdToRemove);

                fs::path path = wdToRemove->getPath();
                log_debug("found wd to remove because of move event: {} {}", removeWd, path.c_str());

                inotify->removeWatch(removeWd);
                auto watchToRemove = wdToRemove->getStartPoint();
                if (watchToRemove) {
                    auto adir = watchToRemove->getAutoscanDirectory();
                    if (adir && adir->persistent()) {
                        monitorNonexisting(path, adir);
                        content->handlePeristentAutoscanRemove(adir);
                    }

                    auto object = database->findObjectByPath(path, UNUSED_CLIENT_GROUP, DbFileType::File);
                    if (object)
                        content->removeObject(adir, object, path, false);
                }
            } catch (const std::out_of_range&) {
            } // Not found in map
        } else
            ++it;
    }
}

void AutoscanInotify::recheckNonexistingMonitors(int wd, const std::shared_ptr<DirectoryWatch>& wdObj)
{
    for (auto&& watch : wdObj->getWdWatches()) {
        if (watch->getType() == WatchType::Autoscan) {
            auto watchAs = std::static_pointer_cast<WatchAutoscan>(watch);
            auto nonExistingPath = watchAs->getNonExistingPath();
            if (!nonExistingPath.empty()) {
                recheckNonexistingMonitor(wd, nonExistingPath, watchAs->getAutoscanDirectory());
            }
        }
    }
}

void AutoscanInotify::removeNonexistingMonitor(int wd, const std::shared_ptr<DirectoryWatch>& wdObj, const fs::path& nonExistingPath)
{
    auto&& wdWatches = wdObj->getWdWatches();
    auto it = std::find_if(wdWatches.begin(), wdWatches.end(),
        [&](auto&& watch) {
            if (watch->getType() == WatchType::Autoscan) {
                auto watchAs = std::static_pointer_cast<WatchAutoscan>(watch);
                return watchAs->getNonExistingPath() == nonExistingPath;
            }
            return false; });
    if (it != wdWatches.end()) {
        if (wdWatches.size() == 1) {
            // removeWatch triggers an IN_IGNORED event so watches.erase(wd) is called in threadProc
            inotify->removeWatch(wd);
        } else {
            wdWatches.erase(it);
        }
    }
}

std::shared_ptr<DirectoryWatch> AutoscanInotify::monitorRecursive(const fs::directory_entry& startPath, const std::shared_ptr<AutoscanDirectory>& adir, bool isStartPoint, bool followSymlinks)
{
    log_debug("start {}", startPath.path().c_str());

    std::error_code ec;
    bool directory = startPath.is_directory(ec);
    if (ec) {
        log_warning("Cannot inspect watched object {}: {}", startPath.path().c_str(), ec.message());
        return nullptr;
    }
    int result = monitorDirectory(startPath.path(), adir, isStartPoint, false, {}, directory ? 0 : IN_DELETE_SELF | IN_MOVE_SELF | IN_UNMOUNT);
    if (result <= INOTIFY_ROOT)
        return nullptr;
    if (!directory)
        return watches.at(result);
    if (!startPath.exists(ec)) {
        log_warning("Could not open {}: {}", startPath.path().c_str(), ec.message());
        return nullptr;
    }
    auto dIter = fs::directory_iterator(startPath, ec);
    if (ec) {
        log_error("monitorRecursive: Failed to iterate {}, {}", startPath.path().c_str(), ec.message());
        return nullptr;
    }
    for (auto&& dirEnt : dIter) {
        if (shutdownFlag)
            break;

        AutoScanSetting asSetting;
        asSetting.adir = adir;
        asSetting.followSymlinks = followSymlinks;
        asSetting.recursive = adir ? adir->getRecursive() : false;
        asSetting.hidden = adir ? adir->getHidden() : defHidden;
        asSetting.rescanResource = true;
        asSetting.async = false;
        asSetting.mergeOptions(config, dirEnt.path());

        if (content->isHiddenFile(dirEnt, dirEnt.is_directory(ec), asSetting)) {
            log_debug("Hidden file {} skipped", dirEnt.path().c_str());
            continue;
        }

        if ((dirEnt.is_directory(ec) && asSetting.recursive) || isRegularFile(dirEnt, ec))
            monitorRecursive(dirEnt, adir, false, followSymlinks);

        if (ec) {
            log_error("AutoscanInotify::monitorRecursive {}: Failed to read {}, {}", startPath.path().c_str(), dirEnt.path().c_str(), ec.message());
        }
    }
    return watches.at(result);
}

int AutoscanInotify::monitorDirectory(
    const fs::path& path,
    const std::shared_ptr<AutoscanDirectory>& adir,
    bool isStartPoint,
    bool hasNonExisting,
    const fs::path& nonExistingPath,
    InotifyFlags monitoredEvents)
{
    int wd = inotify->addWatch(path, monitoredEvents ? monitoredEvents : events, adir->getRetryCount());

    log_debug("start {} => {}", path.c_str(), wd);

    if (wd <= INOTIFY_ROOT) {
        if (isStartPoint && adir && adir->persistent()) {
            monitorNonexisting(path, adir);
        }
    } else {
        bool alreadyWatching = false;
        int parentWd = INOTIFY_UNKNOWN_PARENT_WD;
        if (isStartPoint)
            parentWd = watchPathForMoves(path, wd, adir->getRetryCount());

        std::shared_ptr<DirectoryWatch> wdObj;
        try {
            wdObj = watches.at(wd);
            wdObj->setPath(path);
            if (parentWd > INOTIFY_ROOT && wdObj->getParentWd() <= INOTIFY_ROOT) {
                wdObj->setParentWd(parentWd);
            }

            if (!hasNonExisting)
                alreadyWatching = getAppropriateAutoscan(wdObj, adir) != nullptr;

            // should we check for already existing "nonexisting" watches?
            // ...
        } catch (const std::out_of_range&) {
            wdObj = std::make_shared<DirectoryWatch>(path, wd, parentWd);
            watches.emplace(wd, wdObj);
        }

        if (!alreadyWatching) {
            auto watch = std::make_shared<WatchAutoscan>(isStartPoint, adir);
            if (hasNonExisting) {
                watch->setNonExistingPath(nonExistingPath);
            }
            wdObj->addWatch(std::move(watch));

            if (!isStartPoint && adir) {
                int startPointWd = inotify->addWatch(adir->getLocation(), events, adir->getRetryCount());
                log_debug("getting start point for {} -> {} wd={}", path.c_str(), adir->getLocation().c_str(), startPointWd);
                if (wd > INOTIFY_ROOT)
                    addDescendant(startPointWd, wd, adir);
            }
        }
    }
    return wd;
}

void AutoscanInotify::unmonitorDirectory(const std::shared_ptr<AutoscanDirectory>& adir)
{
    // Remove this autoscan's registrations even if the directory is gone.
    for (const auto& [wd, wdObj] : watches) {
        auto watch = getAppropriateAutoscan(wdObj, adir);
        if (!watch)
            continue;
        if (watch->getIsStartPoint())
            removeWatchMoves(wd);
        removeFromWdObj(wdObj, watch);
    }
}

std::shared_ptr<WatchAutoscan> AutoscanInotify::getAppropriateAutoscan(const std::shared_ptr<DirectoryWatch>& wdObj, const std::shared_ptr<AutoscanDirectory>& adir)
{
    for (auto&& watch : wdObj->getWdWatches()) {
        if (watch->getType() == WatchType::Autoscan) {
            auto watchAs = std::static_pointer_cast<WatchAutoscan>(watch);
            if (watchAs->getNonExistingPath().empty() && watchAs->getAutoscanDirectory()->getLocation() == adir->getLocation()) {
                return watchAs;
            }
        }
    }
    return nullptr;
}

void AutoscanInotify::removeWatchMoves(int wd)
{
    bool first = true;
    int checkWd = wd;
    do {
        std::shared_ptr<DirectoryWatch> wdObj;
        try {
            wdObj = watches.at(checkWd);
        } catch (const std::out_of_range&) {
            break;
        }

        auto&& wdWatches = wdObj->getWdWatches();
        if (wdWatches.empty())
            break;

        if (first) {
            first = false;
        } else {
            for (auto it = wdWatches.begin(); it != wdWatches.end(); /*++it*/) {
                if ((*it)->getType() == WatchType::Move) {
                    auto watchMv = std::static_pointer_cast<WatchMove>(*it);
                    if (watchMv->getRemoveWd() == wd) {
                        log_debug("removing watch move");
                        if (wdWatches.size() > 1) {
                            it = wdWatches.erase(it);
                            continue;
                        }
                        inotify->removeWatch(checkWd);
                    }
                }
                ++it;
            }
        }
        checkWd = wdObj->getParentWd();
    } while (checkWd > INOTIFY_ROOT);
}

void AutoscanInotify::removeFromWdObj(const std::shared_ptr<DirectoryWatch>& wdObj, const std::shared_ptr<Watch>& toRemove)
{
    auto&& wdWatches = wdObj->getWdWatches();
    auto it = std::find(wdWatches.begin(), wdWatches.end(), toRemove);
    if (it == wdWatches.end())
        return;

    wdWatches.erase(it);
    if (wdWatches.empty())
        inotify->removeWatch(wdObj->getWd());
}

void AutoscanInotify::addDescendant(int startPointWd, int addWd, const std::shared_ptr<AutoscanDirectory>& adir)
{
    std::shared_ptr<DirectoryWatch> wdObj;
    try {
        wdObj = watches.at(startPointWd);
    } catch (const std::out_of_range&) {
        return;
    }
    if (!wdObj)
        return;

    auto watch = getAppropriateAutoscan(wdObj, adir);
    if (!watch)
        return;
    watch->addDescendant(addWd);
}

void AutoscanInotify::removeDescendants(int wd)
{
    std::shared_ptr<DirectoryWatch> wdObj;
    try {
        wdObj = watches.at(wd);
    } catch (const std::out_of_range&) {
        return;
    }

    for (auto&& watch : wdObj->getWdWatches()) {
        if (watch->getType() == WatchType::Autoscan) {
            auto watchAs = std::static_pointer_cast<WatchAutoscan>(watch);
            for (int descWd : watchAs->getDescendants()) {
                inotify->removeWatch(descWd);
            }
        }
    }
    watches.erase(wd);
}

#endif // HAVE_INOTIFY
