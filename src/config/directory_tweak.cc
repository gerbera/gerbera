/*GRB*

    Gerbera - https://gerbera.io/

    directory_tweak.cc - this file is part of Gerbera.

    Copyright (C) 2020-2021 Gerbera Contributors

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

#include "directory_tweak.h" // API
#include "config/config.h"
#include "content_manager.h"
#include "util/upnp_clients.h"
#include "util/upnp_quirks.h"

#include <utility>

void AutoScanSetting::mergeOptions(const std::shared_ptr<Config>& config, const fs::path& location)
{
    auto tweak = config->getDirectoryTweakOption(CFG_IMPORT_DIRECTORIES_LIST)->get(location);
    if (tweak == nullptr)
        return;

    if (tweak->hasFollowSymlinks())
        followSymlinks = tweak->getFollowSymlinks();
    if (tweak->hasHidden())
        hidden = tweak->getHidden();
    if (tweak->hasRecursive())
        recursive = tweak->getRecursive();
    if (tweak->hasFanArtFile())
        resourcePatterns.push_back(tweak->getFanArtFile());
    if (tweak->hasSubTitleFile())
        resourcePatterns.push_back(tweak->getSubTitleFile());
    if (tweak->hasResourceFile())
        resourcePatterns.push_back(tweak->getResourceFile());
}

void DirectoryConfigList::add(const std::shared_ptr<DirectoryTweak>& dir, size_t index)
{
    AutoLock lock(mutex);
    _add(dir, index);
}

void DirectoryConfigList::_add(const std::shared_ptr<DirectoryTweak>& dir, size_t index)
{
    if (index == SIZE_MAX) {
        index = getEditSize();
        origSize = list.size() + 1;
        dir->setOrig(true);
    }
    if (std::any_of(list.begin(), list.end(), [=](const auto& d) { return (d->getLocation() == dir->getLocation()); })) {
        log_error("Duplicate tweak entry[{}] {}", index, dir->getLocation().string());
        return;
    }
    list.push_back(dir);
    indexMap[index] = dir;
}

size_t DirectoryConfigList::getEditSize() const
{
    if (indexMap.empty()) {
        return 0;
    }
    return (*std::max_element(indexMap.begin(), indexMap.end(), [&](auto a, auto b) { return (a.first < b.first); })).first + 1;
}

std::vector<std::shared_ptr<DirectoryTweak>> DirectoryConfigList::getArrayCopy()
{
    AutoLock lock(mutex);
    return list;
}

std::shared_ptr<DirectoryTweak> DirectoryConfigList::get(size_t id, bool edit)
{
    AutoLock lock(mutex);
    if (!edit) {
        if (id >= list.size())
            return nullptr;

        return list[id];
    }
    if (indexMap.find(id) != indexMap.end()) {
        return indexMap[id];
    }
    return nullptr;
}

std::shared_ptr<DirectoryTweak> DirectoryConfigList::get(const fs::path& location)
{
    AutoLock lock(mutex);
    const auto& myLocation = location.has_filename() ? location.parent_path() : location;
    for (auto testLoc = myLocation; testLoc.has_parent_path() && testLoc != "/"; testLoc = testLoc.parent_path()) {
        auto entry = std::find_if(list.begin(), list.end(), [=](const auto& d) { return (d->getLocation() == myLocation || d->getInherit()) && d->getLocation() == testLoc; });
        if (entry != list.end() && *entry != nullptr) {
            return *entry;
        }
    }
    return nullptr;
}

void DirectoryConfigList::remove(size_t id, bool edit)
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
        const auto& dir = indexMap[id];
        auto entry = std::find_if(list.begin(), list.end(), [=](const auto& item) { return dir->getLocation() == item->getLocation(); });
        list.erase(entry);
        if (id >= origSize) {
            indexMap.erase(id);
        }
        log_debug("ID {} removed!", id);
    }
}
