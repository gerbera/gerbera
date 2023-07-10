/*GRB*

    Gerbera - https://gerbera.io/

    box_layout.cc - this file is part of Gerbera.

    Copyright (C) 2023 Gerbera Contributors

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

/// \file box_layout.cc

#include "box_layout.h" // API

#include "config/config.h"
#include "content/content_manager.h"

void BoxLayoutList::add(const std::shared_ptr<BoxLayout>& layout, std::size_t index)
{
    AutoLock lock(mutex);
    _add(layout, index);
}

void BoxLayoutList::_add(const std::shared_ptr<BoxLayout>& layout, std::size_t index)
{
    if (index == std::numeric_limits<std::size_t>::max()) {
        index = getEditSize();
        origSize = list.size() + 1;
        layout->setOrig(true);
    }
    if (std::any_of(list.begin(), list.end(), [key = layout->getKey()](auto&& d) { return d->getKey() == key; })) {
        log_error("Duplicate BoxLayout entry[{}] {}", index, layout->getKey());
        return;
    }
    list.push_back(layout);
    indexMap[index] = layout;
}

std::size_t BoxLayoutList::getEditSize() const
{
    if (indexMap.empty()) {
        return 0;
    }
    return std::max_element(indexMap.begin(), indexMap.end(), [](auto a, auto b) { return (a.first < b.first); })->first + 1;
}

std::vector<std::shared_ptr<BoxLayout>> BoxLayoutList::getArrayCopy() const
{
    AutoLock lock(mutex);
    return list;
}

std::shared_ptr<BoxLayout> BoxLayoutList::get(std::size_t id, bool edit) const
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

std::shared_ptr<BoxLayout> BoxLayoutList::get(const std::string& key) const
{
    AutoLock lock(mutex);
    auto entry = std::find_if(list.begin(), list.end(), [&](auto&& d) { return d->getKey() == key; });
    if (entry != list.end() && *entry) {
        return *entry;
    }
    return nullptr;
}

void BoxLayoutList::remove(std::size_t id, bool edit)
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
        auto&& layout = indexMap[id];
        auto entry = std::find_if(list.begin(), list.end(), [key = layout->getKey()](auto&& item) { return key == item->getKey(); });
        list.erase(entry);
        if (id >= origSize) {
            indexMap.erase(id);
        }
        log_debug("ID {} removed!", id);
    }
}
