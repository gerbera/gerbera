/*GRB*

    Gerbera - https://gerbera.io/

    inotify_manager.h - this file is part of Gerbera.

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

/// @file inotify_manager.h
#ifndef __INOTIFY_MANAGER_H__
#define __INOTIFY_MANAGER_H__

#include "content/inotify/inotify_types.h"
#include "content/inotify/mt_inotify.h"
#include "util/grb_fs.h"
#include "util/tools.h"

#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>

// forward declarations
class Inotify;

#define INOTIFY_ROOT (-1)
#define INOTIFY_UNKNOWN_PARENT_WD (-2)

/// @brief Manager base class for directories with inotify
template <class Watcher>
class InotifyManager {
public:
    explicit InotifyManager();
    virtual ~InotifyManager();

    /// @brief Run inotify manager
    void run();

protected:
    /// @brief is set to true by shutdown() if the inotify thread should terminate
    bool shutdownFlag;

    /// @brief thread running the inotify manager
    std::thread inotifyThread;

    /// @brief inotify system interface
    std::unique_ptr<Inotify> inotify;

    /// @brief Storage for watches
    std::unordered_map<int, std::shared_ptr<Watcher>> watches;

    /// @brief event mask with events to watch for (set by constructor)
    InotifyFlags events;

    mutable std::mutex mutex;
    using AutoLock = std::scoped_lock<std::mutex>;

    /// @brief main thread proc loop
    virtual void threadProc() = 0;
};

#endif // __INOTIFY_MANAGER_H__
