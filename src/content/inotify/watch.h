/*GRB*

    Gerbera - https://gerbera.io/

    watch_dir.h - this file is part of Gerbera.

    Copyright (C) 2024 Gerbera Contributors

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

/// \file watch_dir.h

#ifndef __WATCH_DIR_H__
#define __WATCH_DIR_H__

#ifdef HAVE_INOTIFY

#include "util/grb_fs.h"

#include <vector>

class AutoscanDirectory;

enum class WatchType {
    Autoscan,
    Move
};

class Watch {
public:
    explicit Watch(WatchType type)
        : type(type)
    {
    }
    WatchType getType() const { return type; }

private:
    WatchType type;
};

class WatchAutoscan : public Watch {
public:
    WatchAutoscan(bool isStartPoint, std::shared_ptr<AutoscanDirectory> adir)
        : Watch(WatchType::Autoscan)
        , adir(std::move(adir))
        , isStartPoint(isStartPoint)
    {
    }
    std::shared_ptr<AutoscanDirectory> getAutoscanDirectory() const { return adir; }
    bool getIsStartPoint() const { return isStartPoint; }
    fs::path getNonExistingPath() const { return nonExistingPath; }
    void setNonExistingPath(const fs::path& nonExistingPath) { this->nonExistingPath = nonExistingPath; }
    void addDescendant(int wd) { descendants.push_back(wd); }
    const std::vector<int>& getDescendants() const { return descendants; }

private:
    std::shared_ptr<AutoscanDirectory> adir;
    bool isStartPoint;
    std::vector<int> descendants;
    fs::path nonExistingPath;
};

class WatchMove : public Watch {
public:
    explicit WatchMove(int removeWd)
        : Watch(WatchType::Move)
        , removeWd(removeWd)
    {
    }
    int getRemoveWd() const { return removeWd; }

private:
    int removeWd;
};

#endif
#endif
