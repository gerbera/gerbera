/*GRB*

    Gerbera - https://gerbera.io/

    layout_refresh.h - this file is part of Gerbera.

    Copyright (C) 2026 Gerbera Contributors

    Gerbera is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.

    Gerbera is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Gerbera.  If not, see <http://www.gnu.org/licenses/>.
*/

/// @file content/layout/layout_refresh.h

#ifndef __LAYOUT_REFRESH_H__
#define __LAYOUT_REFRESH_H__

#include <map>
#include <memory>
#include <vector>

class CdsObject;

/// @brief Plan in-place title changes between two virtual audio layouts.
/// @details Objects use graph-local IDs and the same physical reference IDs.
/// The root with ID 0 is implicit. Returned objects retain their original IDs.
/// @throws std::runtime_error If the layouts cannot be matched without changing identities.
std::vector<std::shared_ptr<CdsObject>> planAudioLayoutTitles(
    const std::map<int, std::shared_ptr<CdsObject>>& current,
    const std::map<int, std::shared_ptr<CdsObject>>& desired);

#endif // __LAYOUT_REFRESH_H__
