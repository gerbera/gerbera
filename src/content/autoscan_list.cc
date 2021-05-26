/*GRB*

    Gerbera - https://gerbera.io/

    autoscan_list.cc - this file is part of Gerbera.

    Copyright (C) 2021 Gerbera Contributors

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

/// \file autoscan_list.cc
#include "autoscan_list.h"

#include "autoscan.h"
#include "database/database.h"

AutoscanList::AutoscanList(std::shared_ptr<Database> database)
    : database(std::move(database))
{
}

void AutoscanList::updateLMinDB()
{
    AutoLock lock(mutex);
    for (size_t i = 0; i < list.size(); i++) {
        log_debug("i: {}", i);
        auto ad = list[i];
        database->updateAutoscanDirectory(ad);
    }
}

int AutoscanList::add(const std::shared_ptr<AutoscanDirectory>& dir, size_t index)
{
    AutoLock lock(mutex);
    return _add(dir, index);
}

int AutoscanList::_add(const std::shared_ptr<AutoscanDirectory>& dir, size_t index)
{
    if (std::any_of(list.begin(), list.end(), [loc = dir->getLocation()](auto&& item) { return loc == item->getLocation(); })) {
        throw_std_runtime_error("Attempted to add same autoscan path twice");
    }
    if (index == std::numeric_limits<std::size_t>::max()) {
        index = getEditSize();
        origSize = list.size() + 1;
        dir->setOrig(true);
    } else {
        dir->setPersistent(true);
    }
    dir->setScanID(index);
    list.push_back(dir);
    indexMap[dir->getScanID()] = dir;

    return dir->getScanID();
}

void AutoscanList::addList(const std::shared_ptr<AutoscanList>& list)
{
    AutoLock lock(mutex);

    for (auto&& dir : list->list) {
        _add(dir, std::numeric_limits<std::size_t>::max());
    }
}

size_t AutoscanList::getEditSize() const
{
    if (indexMap.empty()) {
        return 0;
    }
    return (*std::max_element(indexMap.begin(), indexMap.end(), [&](auto a, auto b) { return (a.first < b.first); })).first + 1;
}

std::vector<std::shared_ptr<AutoscanDirectory>> AutoscanList::getArrayCopy()
{
    AutoLock lock(mutex);

    return list;
}

std::shared_ptr<AutoscanDirectory> AutoscanList::get(size_t id, bool edit)
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

std::shared_ptr<AutoscanDirectory> AutoscanList::getByObjectID(int objectID)
{
    AutoLock lock(mutex);

    auto it = std::find_if(list.begin(), list.end(), [=](auto&& item) { return objectID == item->getObjectID(); });
    return it != list.end() ? *it : nullptr;
}

std::shared_ptr<AutoscanDirectory> AutoscanList::get(const fs::path& location)
{
    AutoLock lock(mutex);

    auto it = std::find_if(list.begin(), list.end(), [=](auto&& item) { return location == item->getLocation(); });
    return it != list.end() ? *it : nullptr;
}

void AutoscanList::remove(size_t id, bool edit)
{
    AutoLock lock(mutex);

    if (!edit) {
        if (id >= list.size()) {
            log_debug("No such ID {}!", id);
            return;
        }
        auto dir = list[id];
        dir->setScanID(INVALID_SCAN_ID);

        list.erase(list.begin() + id);
        log_debug("ID {} removed!", id);
    } else {
        if (indexMap.find(id) == indexMap.end()) {
            log_debug("No such index ID {}!", id);
            return;
        }
        auto&& dir = indexMap[id];
        auto entry = std::find_if(list.begin(), list.end(), [loc = dir->getScanID()](auto&& item) { return loc == item->getScanID(); });
        dir->setScanID(INVALID_SCAN_ID);
        list.erase(entry);

        if (id >= origSize) {
            indexMap.erase(id);
        }
        log_debug("ID {} removed!", id);
    }
}

std::shared_ptr<AutoscanList> AutoscanList::removeIfSubdir(const fs::path& parent, bool persistent)
{
    AutoLock lock(mutex);

    auto rm_id_list = std::make_shared<AutoscanList>(database);

    for (auto it = list.begin(); it != list.end(); /*++it*/) {
        auto& dir = *it;

        if (startswith(dir->getLocation(), parent)) {
            if (dir->persistent() && !persistent) {
                ++it;
                continue;
            }
            indexMap[dir->getScanID()] = nullptr;
            auto copy = std::make_shared<AutoscanDirectory>();
            dir->copyTo(copy);
            copy->setScanID(dir->getScanID());
            rm_id_list->add(copy);

            dir->setScanID(INVALID_SCAN_ID);
            it = list.erase(it);
        } else
            ++it;
    }

    return rm_id_list;
}

void AutoscanList::notifyAll(Timer::Subscriber* sub)
{
    if (sub == nullptr)
        return;
    AutoLock lock(mutex);

    for (auto&& i : list) {
        sub->timerNotify(i->getTimerParameter());
    }
}
