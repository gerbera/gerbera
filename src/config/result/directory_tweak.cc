/*GRB*

    Gerbera - https://gerbera.io/

    directory_tweak.cc - this file is part of Gerbera.

    Copyright (C) 2020-2024 Gerbera Contributors

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

/// \file directory_tweak.cc
#define GRB_LOG_FAC GrbLogFacility::content

#include "directory_tweak.h" // API

#include "config/config.h"
#include "upnp/clients.h"
#include "util/logger.h"

void DirectoryConfigList::add(const std::shared_ptr<DirectoryTweak>& dir, std::size_t index)
{
    AutoLock lock(mutex);
    _add(dir, index);
}

void DirectoryConfigList::_add(const std::shared_ptr<DirectoryTweak>& dir, std::size_t index)
{
    if (index == std::numeric_limits<std::size_t>::max()) {
        index = getEditSize();
        origSize = list.size() + 1;
        dir->setOrig(true);
    }
    if (std::any_of(list.begin(), list.end(), [loc = dir->getLocation()](auto&& d) { return d->getLocation() == loc; })) {
        log_error("Duplicate tweak entry[{}] {}", index, dir->getLocation().string());
        return;
    }
    list.push_back(dir);
    indexMap[index] = dir;
}

std::size_t DirectoryConfigList::getEditSize() const
{
    if (indexMap.empty()) {
        return 0;
    }
    return std::max_element(indexMap.begin(), indexMap.end(), [](auto a, auto b) { return (a.first < b.first); })->first + 1;
}

std::vector<std::shared_ptr<DirectoryTweak>> DirectoryConfigList::getArrayCopy() const
{
    AutoLock lock(mutex);
    return list;
}

std::shared_ptr<DirectoryTweak> DirectoryConfigList::get(std::size_t id, bool edit) const
{
    AutoLock lock(mutex);
    if (!edit) {
        if (id >= list.size())
            return nullptr;

        return list[id];
    }
    if (indexMap.find(id) != indexMap.end()) {
        return indexMap.at(id);
    }
    return nullptr;
}

std::shared_ptr<DirectoryTweak> DirectoryConfigList::get(const fs::path& location) const
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

void DirectoryConfigList::remove(std::size_t id, bool edit)
{
    AutoLock lock(mutex);

    if (!edit) {
        if (id >= list.size()) {
            log_debug("No such ID {}!", id);
            return;
        }

        list.erase(list.begin() + id);
        log_debug("ID {} removed!", id);
    } else {
        if (indexMap.find(id) == indexMap.end()) {
            log_debug("No such index ID {}!", id);
            return;
        }
        auto&& dir = indexMap[id];
        auto entry = std::find_if(list.begin(), list.end(), [loc = dir->getLocation()](auto&& item) { return loc == item->getLocation(); });
        list.erase(entry);
        if (id >= origSize) {
            indexMap.erase(id);
        }
        log_debug("ID {} removed!", id);
    }
}
