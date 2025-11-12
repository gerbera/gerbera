/*GRB*

    Gerbera - https://gerbera.io/

    scripting_inotify.cc - this file is part of Gerbera.

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

/// @file content/inotify/scripting_inotify.cc
/// @brief Implement manager class for scripting directories with inotify
#define GRB_LOG_FAC GrbLogFacility::inotify

#ifdef HAVE_INOTIFY
#ifdef HAVE_JS
#include "scripting_inotify.h" // API

#include "content/inotify/directory_watch.h"
#include "content/inotify/inotify_manager_cc.h"
#include "content/inotify/inotify_types.h"
#include "content/inotify/watch.h"
#include "content/scripting/scripting_runtime.h"
#include "context.h"

#include <algorithm>

template void InotifyManager<PathWatch>::run();

ScriptingInotify::ScriptingInotify(
    const std::shared_ptr<Config>& config,
    const std::shared_ptr<ScriptingRuntime>& scriptingRuntime)
    : config(config)
    , scriptingRuntime(scriptingRuntime)
{
}

void ScriptingInotify::monitor(const fs::path& dir)
{
    log_debug("Requested to monitor \"{}\"", dir.c_str());
    AutoLock lock(mutex);
    monitorQueue.push(dir);
    inotify->stop();
}

void ScriptingInotify::threadProc()
{
    std::error_code ec;
    while (!shutdownFlag) {
        try {
            std::unique_lock<std::mutex> lock(mutex);

            // monitor new dir
            while (!monitorQueue.empty()) {
                auto location = std::move(monitorQueue.front());
                monitorQueue.pop();

                lock.unlock();

                if (location.empty()) {
                    log_debug("Empty dir");
                    lock.lock();
                    continue;
                }

                auto dirEnt = fs::directory_entry(location, ec);
                if (!ec) {
                    // handle dir recursively
                    log_debug("Adding recursive watch: {}", location.c_str());
                    monitorDirectory(location);
                } else {
                    log_error("Failed to read directory {}: {}", location.c_str(), ec.message());
                }

                lock.lock();
            }

            lock.unlock();

            /* --- get event --- (blocking) */
            inotify_event* event = inotify->nextEvent();
            /* --- */

            if (event && (event->mask & events)) {
                log_debug("inotify event: {} mask={} name={}", event->wd, InotifyUtil::mapFlags(event->mask), event->name);
                scriptingRuntime->reloadFolders();
            }
        } catch (const std::runtime_error& e) {
            log_error("Inotify thread caught exception: {}", e.what());
        }
    }
}

int ScriptingInotify::monitorDirectory(const fs::path& path)
{
    int wd = inotify->addWatch(path, events);

    log_debug("start {} => {}", path.c_str(), wd);

    if (wd > INOTIFY_ROOT) {
        std::shared_ptr<PathWatch> wdObj;
        try {
            wdObj = watches.at(wd);
        } catch (const std::out_of_range&) {
            wdObj = std::make_shared<PathWatch>(path, wd, INOTIFY_UNKNOWN_PARENT_WD);
            watches.emplace(wd, wdObj);
        }

        auto watch = std::make_shared<WatchScript>(path);

        int startPointWd = inotify->addWatch(path, events);
        log_debug("getting start point for {} -> {} wd={}", path.c_str(), path.c_str(), startPointWd);
        if (wd > INOTIFY_ROOT)
            watch->addDescendant(wd);
        wdObj->addWatch(std::move(watch));
    } else {
        log_debug("failed {} => {}", path.c_str(), wd);
    }
    return wd;
}

#endif

#endif // HAVE_INOTIFY
