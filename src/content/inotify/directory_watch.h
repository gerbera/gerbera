/*GRB*

    Gerbera - https://gerbera.io/

    directory_watch.h - this file is part of Gerbera.

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

/// @file directory_watch.h

#ifndef __DIRECTORY_WATCH_H__
#define __DIRECTORY_WATCH_H__

#ifdef HAVE_INOTIFY

#include "util/grb_fs.h"

#include <vector>

// forward declarations
class Watch;
class WatchAutoscan;

/// @brief Manager class for watch with path and sub items
class PathWatch {
public:
    PathWatch(fs::path path, int wd, int parentWd)
        : path(std::move(path))
        , parentWd(parentWd)
        , wd(wd)
    {
    }

    /// @brief get associated path
    fs::path getPath() const { return path; }

    /// @brief set associated path
    void setPath(const fs::path& newPath) { path = newPath; }

    /// @brief get associated watch id
    int getWd() const { return wd; }

    /// @brief get associated parent watch id
    int getParentWd() const { return parentWd; }

    /// @brief set associated parent watch id
    void setParentWd(int parentWd) { this->parentWd = parentWd; }

    /// @brief set associated child watches
    const std::unique_ptr<std::vector<std::shared_ptr<Watch>>>& getWdWatches() const { return wdWatches; }

    /// @brief add child watch
    void addWatch(const std::shared_ptr<Watch>& w) { wdWatches->push_back(w); }

protected:
    std::unique_ptr<std::vector<std::shared_ptr<Watch>>> wdWatches { std::make_unique<std::vector<std::shared_ptr<Watch>>>() };
    fs::path path;
    int parentWd;
    int wd;
};

/// @brief Manager class for autoscan directory watch
class DirectoryWatch : public PathWatch {
    using PathWatch::PathWatch;

public:
    /// @brief get root watch item for autoscan watch
    std::shared_ptr<WatchAutoscan> getStartPoint() const;

    /// @brief get autoscan watch item for path
    std::shared_ptr<WatchAutoscan> getAppropriateAutoscan(const fs::path& path) const;
};

#endif
#endif
