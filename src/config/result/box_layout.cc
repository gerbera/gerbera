/*GRB*

    Gerbera - https://gerbera.io/

    box_layout.cc - this file is part of Gerbera.

    Copyright (C) 2023-2024 Gerbera Contributors

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
#define GRB_LOG_FAC GrbLogFacility::content

#include "box_layout.h" // API

#include "config/config.h"
#include "util/logger.h"

#include <algorithm>

std::shared_ptr<BoxLayout> BoxLayoutList::getKey(const std::string_view& key) const
{
    AutoLock lock(mutex);
    auto entry = std::find_if(list.begin(), list.end(), [&](auto&& d) { return d->getKey() == key; });
    if (entry != list.end() && *entry) {
        return *entry;
    }
    return nullptr;
}
