/*GRB*

    Gerbera - https://gerbera.io/

    scripting_inotify.h - this file is part of Gerbera.

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

/// @file content/inotify/scripting_inotify.h
#ifndef __SCRIPTING_INOTIFY_H__
#define __SCRIPTING_INOTIFY_H__

#ifdef HAVE_JS

#include "inotify_manager.h"

#include <queue>

// forward declarations
class Config;
class PathWatch;
class ScriptingRuntime;

/// @brief Manager class for script directories with inotify
class ScriptingInotify : public InotifyManager<PathWatch> {
public:
    explicit ScriptingInotify(
        const std::shared_ptr<Config>& config,
        const std::shared_ptr<ScriptingRuntime>& scriptingRuntime);

    ScriptingInotify(const ScriptingInotify&) = delete;
    ScriptingInotify& operator=(const ScriptingInotify&) = delete;

    /// @brief Start monitoring a directory
    void monitor(const fs::path& dir);

private:
    std::shared_ptr<Config> config;
    std::shared_ptr<ScriptingRuntime> scriptingRuntime;

    /// @brief directory to add to monitoring
    std::queue<fs::path> monitorQueue;

    /// @brief main thread proc loop
    void threadProc() override;

    /// @brief Update monitoring of a directory
    int monitorDirectory(const fs::path& path);
};
#endif

#endif // __SCRIPTING_INOTIFY_H__
