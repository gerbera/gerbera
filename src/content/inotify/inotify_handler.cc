/*GRB*

    Gerbera - https://gerbera.io/

    inotify_handler.cc - this file is part of Gerbera.

    Copyright (C) 2024-2025 Gerbera Contributors

    Gerbera is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.

    Gerbera is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Gerbera.  If not, see <http://www.gnu.org/licenses/>.

    $Id$
*/

/// \file inotify_handler.cc
#define GRB_LOG_FAC GrbLogFacility::autoscan

#ifdef HAVE_INOTIFY
#include "content/inotify/inotify_handler.h" // API

#include "cds/cds_objects.h"
#include "config/result/autoscan.h"
#include "content/autoscan_setting.h"
#include "content/content.h"
#include "content/inotify/autoscan_inotify.h"
#include "content/inotify/directory_watch.h"
#include "content/inotify/watch.h"
#include "database/database.h"
#include "util/logger.h"
#include "util/tools.h"

#include <array>
#include <numeric>
#include <sys/inotify.h>
#include <utility>

#define AUTOSCAN_WAS_MOVED(mask) ((mask) & (IN_MOVE_SELF))
#define AUTOSCAN_IS_GONE(mask) ((mask) & (IN_DELETE_SELF | IN_MOVE_SELF | IN_UNMOUNT))
#define AUTOSCAN_WAS_REMOVED(mask) ((mask) & (IN_DELETE | IN_DELETE_SELF | IN_MOVE_SELF | IN_MOVED_FROM | IN_UNMOUNT))
#define AUTOSCAN_IS_DIR(mask) ((mask) & (IN_ISDIR))
#define AUTOSCAN_IS_NEW_FILE(mask) ((mask) & (IN_CLOSE_WRITE | IN_CREATE | IN_MOVED_TO | IN_ATTRIB))
#define AUTOSCAN_IS_MOVED(mask) ((mask) & (IN_MOVED_TO))
#define AUTOSCAN_IS_NEW(mask) ((mask) & (IN_CREATE | IN_MOVED_TO | IN_ATTRIB))
#define AUTOSCAN_IS_CREATED(mask) ((mask) & (IN_CREATE | IN_ATTRIB))
#define AUTOSCAN_IS_WRITTEN(mask) ((mask) & (IN_CLOSE_WRITE | IN_ATTRIB))
#define AUTOSCAN_IS_IGNORED(mask) ((mask) & (IN_IGNORED))

#ifndef IN_MASK_CREATE
#define IN_MASK_CREATE 0x10000000
#endif

static constexpr std::array<std::pair<std::string_view, InotifyFlags>, 22> inotifyFlags {
    /* the following are legal, implemented events that user-space can watch for */
    std::pair("ACCESS", IN_ACCESS), /* File was accessed */
    std::pair("MODIFY", IN_MODIFY), /* File was modified */
    std::pair("ATTRIB", IN_ATTRIB), /* Metadata changed */
    std::pair("CLOSE_WRITE", IN_CLOSE_WRITE), /* Writable file was closed */
    std::pair("CLOSE_NOWRITE", IN_CLOSE_NOWRITE), /* Unwritable file closed */
    std::pair("OPEN", IN_OPEN), /* File was opened */
    std::pair("MOVED_FROM", IN_MOVED_FROM), /* File was moved from X */
    std::pair("MOVED_TO", IN_MOVED_TO), /* File was moved to Y */
    std::pair("CREATE", IN_CREATE), /* Subfile was created */
    std::pair("DELETE", IN_DELETE), /* Subfile was deleted */
    std::pair("DELETE_SELF", IN_DELETE_SELF), /* Self was deleted */
    std::pair("MOVE_SELF", IN_MOVE_SELF), /* Self was moved; Generates an IN_MOVED_FROM event for dir1, an IN_MOVED_TO event for dir2 */
    /* the following are legal events.  they are sent as needed to any watch */
    std::pair("UNMOUNT", IN_UNMOUNT), /* Backing fs was unmounted */
    std::pair("Q_OVERFLOW", IN_Q_OVERFLOW), /* Event queued overflowed */
    std::pair("IGNORED", IN_IGNORED), /* File was ignored */
    /* special flags */
    std::pair("ONLYDIR", IN_ONLYDIR), /* only watch the path if it is a directory */
    std::pair("DONT_FOLLOW", IN_DONT_FOLLOW), /* don't follow a sym link */
    std::pair("EXCL_UNLINK", IN_EXCL_UNLINK), /* exclude events on unlinked objects */
    std::pair("MASK_CREATE", IN_MASK_CREATE), /* only create watches */
    std::pair("MASK_ADD", IN_MASK_ADD), /* add to the mask of an already existing watch */
    std::pair("ISDIR", IN_ISDIR), /* event occurred against dir */
    std::pair("ONESHOT", IN_ONESHOT), /* only send event once */
};

InotifyHandler::InotifyHandler(AutoscanInotify* ai, struct inotify_event* event, InotifyFlags maskedEvent)
    : ai(ai)
    , wd(event->wd)
    , mask(maskedEvent)
    , name(event->len > 0 ? event->name : "") // The name field is only present when an event is returned for a file inside a watched directory
{
    log_debug("inotify event: {} mask={} name={}", wd, mapFlags(mask), name);
}

InotifyFlags InotifyHandler::remapFlag(const std::string& flag)
{
    for (auto [qLabel, qFlag] : inotifyFlags) {
        if (flag == qLabel) {
            return qFlag;
        }
    }
    return stoulString(flag, 0, 0);
}

InotifyFlags InotifyHandler::makeFlags(const std::string& optValue)
{
    std::vector<std::string> flagsVector = splitString(optValue, '|', false);
    return std::accumulate(flagsVector.begin(), flagsVector.end(), 0, [](auto flg, auto&& i) { return flg | InotifyHandler::remapFlag(trimString(i)); });
}

std::string InotifyHandler::mapFlags(InotifyFlags flags)
{
    if (!flags)
        return "None";

    std::vector<std::string> myFlags;

    for (auto [qLabel, qFlag] : inotifyFlags) {
        if (flags & qFlag) {
            myFlags.emplace_back(qLabel);
            flags &= ~qFlag;
        }
    }

    if (flags) {
        myFlags.push_back(fmt::format("{:#04x}", flags));
    }

    return fmt::format("{}", fmt::join(myFlags, " | "));
}

fs::path InotifyHandler::getPath(const std::shared_ptr<DirectoryWatch>& wdObj)
{
    path = wdObj->getPath();
    // file is not gone
    if (!AUTOSCAN_IS_GONE(mask) && !name.empty()) {
        path /= name;
        log_debug("!AUTOSCAN_IS_GONE({}): {}", mapFlags(mask), path.c_str());
    } else {
        log_debug("processing {} event {}", path.c_str(), mapFlags(mask));
    }
    return path;
}

std::pair<bool, std::shared_ptr<AutoscanDirectory>> InotifyHandler::getAutoscanDirectory(const std::shared_ptr<DirectoryWatch>& wdObj)
{
    std::error_code ec;
    dirEnt = fs::directory_entry(path, ec);
    bool isDir = AUTOSCAN_IS_DIR(mask);
    if (!ec) {
        isDir = isDir || !isRegularFile(path, ec);
    } else if (!isDir) {
        isDir = !isRegularFile(path, ec);
    }
    auto watchAs = wdObj->getAppropriateAutoscan(path);
    if (watchAs)
        adir = watchAs->getAutoscanDirectory();
    if (!watchAs || !adir) {
        log_debug("autoscan not found in watches? ({}, watchAs:{}, adir:{}, {})", wd, watchAs != nullptr, adir != nullptr, path.c_str());
    } else if (ec && !AUTOSCAN_WAS_REMOVED(mask)) {
        log_error("Failed to read {} for event {}: {}", path.c_str(), mapFlags(mask), ec.message());
    }
    log_debug("autoscan for watch (isDir:{}, adir:{}, {})", isDir, adir != nullptr, path.c_str());
    return { isDir, adir };
}

void InotifyHandler::doMove(const std::shared_ptr<DirectoryWatch>& wdObj)
{
    // file is renamed
    if (AUTOSCAN_WAS_MOVED(mask)) {
        ai->checkMoveWatches(wd, wdObj);
    }

    // file is deleted
    if (AUTOSCAN_IS_GONE(mask)) {
        ai->recheckNonexistingMonitors(wd, wdObj);
    }
}

void InotifyHandler::doDirectory(AutoScanSetting& asSetting, const std::shared_ptr<Content>& content, const std::shared_ptr<DirectoryWatch>& wdObj)
{
    if (AUTOSCAN_IS_NEW(mask)) {
        ai->recheckNonexistingMonitors(wd, wdObj);
    }

    if (asSetting.recursive && AUTOSCAN_IS_CREATED(mask)) {
        if (!content->isHiddenFile(dirEnt, true, asSetting)) {
            log_debug("Detected new dir, adding to inotify: {}", path.c_str());
            ai->monitorUnmonitorRecursive(dirEnt, false, asSetting.adir, false, asSetting.followSymlinks);
        } else {
            log_debug("Detected new dir, ignoring because it's hidden: {}", path.c_str());
        }
    }
}

int InotifyHandler::doExistingFile(const std::shared_ptr<Database>& database, const std::shared_ptr<Content>& content, const std::shared_ptr<DirectoryWatch>& wdObj, ImportMode importMode, bool& isDir)
{
    int result = INOTIFY_ROOT;
    if (!AUTOSCAN_IS_NEW(mask)) {
        // deleted
        log_debug("updating {}", path.c_str());
        if (AUTOSCAN_IS_GONE(mask)) {
            if (!AUTOSCAN_WAS_MOVED(mask)) {
                log_debug("removing watch {}", path.c_str());
                result = wd;
            }
            auto watch = wdObj->getStartPoint();
            if (watch && adir->persistent()) {
                ai->monitorNonexisting(path, watch->getAutoscanDirectory());
                content->handlePeristentAutoscanRemove(adir);
            }
        }

        changedObject = database->findObjectByPath(path, UNUSED_CLIENT_GROUP, DbFileType::Any);
        log_debug("found {} -> {}", path.c_str(), changedObject ? changedObject->getID() : INVALID_OBJECT_ID);
        if (changedObject)
            isDir = changedObject->isContainer();
        if ((!AUTOSCAN_IS_WRITTEN(mask) || importMode != ImportMode::Gerbera) && changedObject) {
            log_debug("deleting {}", path.c_str());
            content->removeObject(adir, changedObject, path, !AUTOSCAN_IS_MOVED(mask), false);
            changedObject = nullptr;
        }
    }
    return result;
}

void InotifyHandler::doNewFile(AutoScanSetting& asSetting, const std::shared_ptr<Content>& content, bool isDir)
{
    if (changedObject)
        asSetting.changedObject = changedObject;

    if (AUTOSCAN_IS_NEW_FILE(mask)) {
        log_debug("Adding {}", path.c_str());
        // dirEnt, path, rootPath, settings, lowPriority, cancellable
        content->addFile(dirEnt, adir->getLocation(), asSetting, true, false);
        asSetting.changedObject = nullptr;
        if (isDir) {
            auto wdObjPath = ai->monitorUnmonitorRecursive(dirEnt, false, adir, false, asSetting.followSymlinks);
            if (AUTOSCAN_IS_MOVED(mask) && wdObjPath) {
                log_debug("Resetting {} to {}", wdObjPath->getPath().c_str(), path.c_str());
                wdObjPath->setPath(path);
            }
        }
    }
}

void InotifyHandler::doIgnored()
{
    if (AUTOSCAN_IS_IGNORED(mask)) {
        ai->removeWatchMoves(wd);
        ai->removeDescendants(wd);
    }
}

#endif
