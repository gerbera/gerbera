/*GRB*

    Gerbera - https://gerbera.io/

    directory_watch.h - this file is part of Gerbera.

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

/// \file directory_watch.h

#ifndef __DIRECTORY_WATCH_H__
#define __DIRECTORY_WATCH_H__

#ifdef HAVE_INOTIFY

#include "util/grb_fs.h"

#include <vector>

class AutoscanDirectory;
class Watch;
class WatchAutoscan;

class DirectoryWatch {
public:
    DirectoryWatch(fs::path path, int wd, int parentWd)
        : path(std::move(path))
        , parentWd(parentWd)
        , wd(wd)
    {
    }
    fs::path getPath() const { return path; }
    void setPath(const fs::path& newPath) { path = newPath; }
    int getWd() const { return wd; }
    int getParentWd() const { return parentWd; }
    void setParentWd(int parentWd) { this->parentWd = parentWd; }

    const std::unique_ptr<std::vector<std::shared_ptr<Watch>>>& getWdWatches() const { return wdWatches; }
    void addWatch(const std::shared_ptr<Watch>& w) { wdWatches->push_back(w); }
    std::shared_ptr<WatchAutoscan> getStartPoint() const;
    std::shared_ptr<WatchAutoscan> getAppropriateAutoscan(const fs::path& path) const;

private:
    std::unique_ptr<std::vector<std::shared_ptr<Watch>>> wdWatches { std::make_unique<std::vector<std::shared_ptr<Watch>>>() };
    fs::path path;
    int parentWd;
    int wd;
};

#endif
#endif
