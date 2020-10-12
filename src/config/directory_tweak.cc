/*GRB*

    Gerbera - https://gerbera.io/

    directory_tweak.cc - this file is part of Gerbera.

    Copyright (C) 2020 Gerbera Contributors

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
#include "content_manager.h"
#include "util/upnp_clients.h"
#include "util/upnp_quirks.h"

#include <utility>

void DirectoryConfigList::add(const std::shared_ptr<DirectoryTweak>& client, size_t index)
{
    AutoLock lock(mutex);
    _add(client, index);
}

void DirectoryConfigList::_add(const std::shared_ptr<DirectoryTweak>& client, size_t index)
{
    if (index == SIZE_MAX) {
        index = getEditSize();
        origSize = list.size() + 1;
        client->setOrig(true);
    }
    list.push_back(client);
    indexMap[index] = client;
}

size_t DirectoryConfigList::getEditSize() const
{
    if (indexMap.empty()) {
        return 0;
    }
    return (*std::max_element(indexMap.begin(), indexMap.end(), [&] (auto a, auto b) { return (a.first < b.first);})).first + 1;
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
        auto entry = std::find_if(list.begin(), list.end(), [=](const auto& item) { return dir->getLocation() == item->getLocation();});
        list.erase(entry);
        if (id >= origSize) {
            indexMap.erase(id);
        }
        log_debug("ID {} removed!", id);
    }
}
