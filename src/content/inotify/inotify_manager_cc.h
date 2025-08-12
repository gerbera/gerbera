/*GRB*

    Gerbera - https://gerbera.io/

    inotify_manager_cc.h - this file is part of Gerbera.

    Copyright (C) 2025 Gerbera Contributors

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

/// @file inotify_manager_cc.h
/// @brief Implement manager base class for inotify

#ifndef __INOTIFY_MANAGER_CC_H__
#define __INOTIFY_MANAGER_CC_H__
#ifdef HAVE_INOTIFY

#include "content/inotify/inotify_manager.h"
#include "util/logger.h"

#include <sys/inotify.h>

#define INOTIFY_MAX_USER_WATCHES_FILE "/proc/sys/fs/inotify/max_user_watches"

template <class Watcher>
InotifyManager<Watcher>::InotifyManager()
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

    events = IN_CLOSE_WRITE | IN_CREATE | IN_MOVED_FROM | IN_MOVED_TO | IN_DELETE | IN_DELETE_SELF | IN_MOVE_SELF | IN_UNMOUNT;
    shutdownFlag = true;
}

template <class Watcher>
InotifyManager<Watcher>::~InotifyManager()
{
    std::unique_lock<std::mutex> lock(mutex);
    if (!shutdownFlag) {
        log_debug("start");
        shutdownFlag = true;
        inotify->stop();
        lock.unlock();
        inotifyThread.join();
        log_debug("inotify thread died.");
        inotify = nullptr;
        watches.clear();
    }
}

template <class Watcher>
void InotifyManager<Watcher>::run()
{
    AutoLock lock(mutex);

    if (shutdownFlag) {
        shutdownFlag = false;
        inotify = std::make_unique<Inotify>();
        inotifyThread = std::thread([this] { threadProc(); });
    }
}

#endif // HAVE_INOTIFY
#endif // __INOTIFY_MANAGER_CC_H__
