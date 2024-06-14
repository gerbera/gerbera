/*GRB*

Gerbera - https://gerbera.io/

    resolution.cc - this file is part of Gerbera.

    Copyright (C) 2022-2024 Gerbera Contributors

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

/// \file resolution.cc
#define GRB_LOG_FAC GrbLogFacility::metadata

#include "resolution.h" // API

#include "exceptions.h"
#include "util/tools.h"

Resolution::Resolution(const std::string& string)
{
    std::vector<std::string> parts = splitString(string, 'x');
    if (parts.size() != 2) {
        throw_std_runtime_error("Failed to parse '{}' to valid resolution", string);
    }

    if (!parts[0].empty()) {
        auto x = stoulString(trimString(parts[0]));
        if (x == 0 || x == std::numeric_limits<unsigned long>::max()) {
            throw_std_runtime_error("Failed to parse '{}' to valid resolution", string);
        }
        _x = x;
    }

    if (!parts[1].empty()) {
        auto y = stoulString(trimString(parts[1]));
        if (y == 0 || y == std::numeric_limits<unsigned long>::max()) {
            throw_std_runtime_error("Failed to parse '{}' to valid resolution", string);
        }
        _y = y;
    }
}

Resolution::Resolution(std::uint64_t x, std::uint64_t y)
    : _x(x)
    , _y(y)
{
    if (x == 0 || x == std::numeric_limits<std::uint64_t>::max()) {
        throw_std_runtime_error("Failed to set '{}x{}' as resolution", x, y);
    }
    if (y == 0 || y == std::numeric_limits<std::uint64_t>::max()) {
        throw_std_runtime_error("Failed to set '{}x{}' as resolution", x, y);
    }
}

std::string Resolution::string() const
{
    return fmt::format("{}x{}", _x, _y);
}
std::uint64_t Resolution::x() const
{
    return _x;
}
std::uint64_t Resolution::y() const
{
    return _y;
}
