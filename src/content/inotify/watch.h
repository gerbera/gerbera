/*GRB*

    Gerbera - https://gerbera.io/

    watch.h - this file is part of Gerbera.

    Copyright (C) 2024-2026 Gerbera Contributors

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

/// @file content/inotify/watch.h

#ifndef __WATCH_DIR_H__
#define __WATCH_DIR_H__

#ifdef HAVE_INOTIFY

#include "util/grb_fs.h"

#include <vector>

// forward declaration
class AutoscanDirectory;

enum class WatchType {
    Autoscan,
    Script,
    Move
};

/// @brief generic watch
class Watch {
protected:
    explicit Watch(WatchType type)
        : type(type)
    {
    }

public:
    /// @brief get type of watch
    WatchType getType() const { return type; }

private:
    WatchType type;
};

/// @brief watch for autoscan directory
class WatchScript : public Watch {
public:
    WatchScript(fs::path dir)
        : Watch(WatchType::Script)
        , dir(std::move(dir))
    {
    }

    /// @brief get associated script directory
    fs::path getPath() const { return dir; }

    /// @brief add sub watch
    void addDescendant(int wd) { descendants.push_back(wd); }

    /// @brief get sub watches
    const std::vector<int>& getDescendants() const { return descendants; }

private:
    std::vector<int> descendants;
    fs::path dir;
};

/// @brief watch for autoscan directory
class WatchAutoscan : public Watch {
public:
    WatchAutoscan(bool isStartPoint, std::shared_ptr<AutoscanDirectory> adir)
        : Watch(WatchType::Autoscan)
        , adir(std::move(adir))
        , isStartPoint(isStartPoint)
    {
    }
    /// @brief get associated autoscan directory
    std::shared_ptr<AutoscanDirectory> getAutoscanDirectory() const { return adir; }

    /// @brief is watch root of a watch tree
    bool getIsStartPoint() const { return isStartPoint; }

    /// @brief get watch for non existing path
    fs::path getNonExistingPath() const { return nonExistingPath; }

    /// @brief set watch for non existing path
    void setNonExistingPath(const fs::path& nonExistingPath) { this->nonExistingPath = nonExistingPath; }

    /// @brief add sub watch
    void addDescendant(int wd) { descendants.push_back(wd); }

    /// @brief get sub watches
    const std::vector<int>& getDescendants() const { return descendants; }

private:
    std::shared_ptr<AutoscanDirectory> adir;
    bool isStartPoint;
    std::vector<int> descendants;
    fs::path nonExistingPath;
};

// @brief watch for move operations to be deleted
class WatchMove : public Watch {
public:
    explicit WatchMove(int removeWd)
        : Watch(WatchType::Move)
        , removeWd(removeWd)
    {
    }

    // @brief get id to be deleted
    int getRemoveWd() const { return removeWd; }

private:
    int removeWd;
};

#endif
#endif
