/*GRB*

    Gerbera - https://gerbera.io/

    directory_tweak.cc - this file is part of Gerbera.

    Copyright (C) 2020-2026 Gerbera Contributors

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

/// @file config/result/directory_tweak.cc
#define GRB_LOG_FAC GrbLogFacility::content

#include "directory_tweak.h" // API

#include "config/config.h"

#include <algorithm>

std::shared_ptr<DirectoryTweak> DirectoryConfigList::getKey(const fs::path& location) const
{
    AutoLock lock(mutex);
    auto&& myLocation = location.has_filename() ? location.parent_path() : location;
    for (auto testLoc = myLocation; testLoc.has_parent_path() && testLoc != "/"; testLoc = testLoc.parent_path()) {
        auto entry = std::find_if(list.begin(), list.end(), [&](auto&& d) { return (d->getLocation() == myLocation || d->getInherit()) && d->getLocation() == testLoc; });
        if (entry != list.end() && *entry) {
            return *entry;
        }
    }
    return nullptr;
}
