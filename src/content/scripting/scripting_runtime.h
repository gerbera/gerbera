/*MT*

    MediaTomb - http://www.mediatomb.cc/

    scripting_runtime.h - this file is part of MediaTomb.

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

/// @file content/scripting/scripting_runtime.h

#ifndef __SCRIPTING_RUNTIME_H__
#define __SCRIPTING_RUNTIME_H__

#include <duktape.h>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

// forward declarations
class Script;

/// @brief ScriptingRuntime class definition.
class ScriptingRuntime {
protected:
    duk_context* ctx;
    mutable std::recursive_mutex mutex;

    std::vector<std::shared_ptr<Script>> activeScripts;

public:
    ScriptingRuntime();
    virtual ~ScriptingRuntime();

    ScriptingRuntime(const ScriptingRuntime&) = delete;
    ScriptingRuntime& operator=(const ScriptingRuntime&) = delete;

    /// @brief script listens to changes in folders
    void addScript(const std::shared_ptr<Script>& script) { activeScripts.push_back(script); }
    /// @brief get scripts
    const std::vector<std::shared_ptr<Script>>& getScripts() { return activeScripts; }
    /// @brief reload scripts in folders
    bool reloadFolders();

    /// @brief Returns a new (sub)context. !!! Not thread-safe !!!
    duk_context* createContext(const std::string& name);
    void destroyContext(const std::string& name);

    using AutoLock = std::scoped_lock<std::recursive_mutex>;
    std::recursive_mutex& getMutex() const { return mutex; }
};

#endif // __SCRIPTING_RUNTIME_H__
