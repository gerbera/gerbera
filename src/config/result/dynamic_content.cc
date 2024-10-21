/*GRB*

    Gerbera - https://gerbera.io/

    dynamic_content.cc - this file is part of Gerbera.

    Copyright (C) 2021-2024 Gerbera Contributors

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
#define GRB_LOG_FAC GrbLogFacility::content

#include "dynamic_content.h" // API

#include "util/logger.h"

#include <algorithm>

template <>
void EditHelper<DynamicContent>::_add(const std::shared_ptr<DynamicContent>& cont, std::size_t index)
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

std::shared_ptr<DynamicContent> DynamicContentList::getKey(const fs::path& location) const
{
    AutoLock lock(mutex);
    auto entry = std::find_if(list.begin(), list.end(), [=](auto&& c) { return c->getLocation() == location; });
    if (entry != list.end() && *entry) {
        return *entry;
    }
    return nullptr;
}
