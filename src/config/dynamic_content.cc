/*GRB*

    Gerbera - https://gerbera.io/

    dynamic_content.cc - this file is part of Gerbera.

    Copyright (C) 2021-2022 Gerbera Contributors

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

/// \file dynamic_content.cc

#include "dynamic_content.h" // API

#include "content/content_manager.h"

void DynamicContentList::add(const std::shared_ptr<DynamicContent>& cont, std::size_t index)
{
    AutoLock lock(mutex);
    _add(cont, index);
}

void DynamicContentList::_add(const std::shared_ptr<DynamicContent>& cont, std::size_t index)
{
    if (index == std::numeric_limits<std::size_t>::max()) {
        index = getEditSize();
        origSize = list.size() + 1;
        cont->setOrig(true);
    }
    while (std::any_of(list.begin(), list.end(), [loc = cont->getLocation()](auto&& d) { return d->getLocation() == loc; })) {
        log_error("Duplicate container entry[{}] {}", index, cont->getLocation().string());
        cont->setLocation(fmt::format("{}_2", cont->getLocation().string()));
    }
    list.push_back(cont);
    indexMap[index] = cont;
}

std::size_t DynamicContentList::getEditSize() const
{
    if (indexMap.empty()) {
        return 0;
    }
    return std::max_element(indexMap.begin(), indexMap.end(), [](auto a, auto b) { return (a.first < b.first); })->first + 1;
}

std::vector<std::shared_ptr<DynamicContent>> DynamicContentList::getArrayCopy() const
{
    AutoLock lock(mutex);
    return list;
}

std::shared_ptr<DynamicContent> DynamicContentList::get(std::size_t id, bool edit) const
{
    AutoLock lock(mutex);
    if (!edit) {
        if (id >= list.size())
            return nullptr;

        return list[id];
    }
    return getValueOrDefault(indexMap, id, std::shared_ptr<DynamicContent>());
}

std::shared_ptr<DynamicContent> DynamicContentList::get(const fs::path& location) const
{
    AutoLock lock(mutex);
    auto entry = std::find_if(list.begin(), list.end(), [=](auto&& c) { return c->getLocation() == location; });
    if (entry != list.end() && *entry) {
        return *entry;
    }
    return nullptr;
}

void DynamicContentList::remove(std::size_t id, bool edit)
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
