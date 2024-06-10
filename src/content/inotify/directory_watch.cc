/*GRB*

    Gerbera - https://gerbera.io/

    directory_watch.cc - this file is part of Gerbera.

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

/// \file directory_watch.cc
#define GRB_LOG_FAC GrbLogFacility::autoscan

#ifdef HAVE_INOTIFY
#include "content/inotify/directory_watch.h" // API

#include "config/result/autoscan.h"
#include "content/inotify/watch.h"

std::shared_ptr<WatchAutoscan> DirectoryWatch::getStartPoint() const
{
    for (auto&& watch : *wdWatches) {
        if (watch->getType() == WatchType::Autoscan) {
            auto watchAs = std::static_pointer_cast<WatchAutoscan>(watch);
            if (watchAs->getIsStartPoint())
                return watchAs;
        }
    }
    return nullptr;
}

std::shared_ptr<WatchAutoscan> DirectoryWatch::getAppropriateAutoscan(const fs::path& path) const
{
    fs::path pathBestMatch;
    std::shared_ptr<WatchAutoscan> bestMatch;
    for (auto&& watch : *wdWatches) {
        if (watch->getType() == WatchType::Autoscan) {
            auto watchAs = std::static_pointer_cast<WatchAutoscan>(watch);
            if (watchAs->getNonExistingPath().empty()) {
                fs::path testLocation = watchAs->getAutoscanDirectory()->getLocation();
                if (isSubDir(path, testLocation)) {
                    if (!pathBestMatch.empty()) {
                        if (pathBestMatch < testLocation) {
                            pathBestMatch = testLocation;
                            bestMatch = watchAs;
                        }
                    } else {
                        pathBestMatch = testLocation;
                        bestMatch = watchAs;
                    }
                }
            }
        }
    }
    return bestMatch;
}

#endif
