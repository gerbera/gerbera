/*GRB*

Gerbera - https://gerbera.io/

    resolution.h - this file is part of Gerbera.

    Copyright (C) 2022-2026 Gerbera Contributors

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

/// @file metadata/resolution.h

#ifndef GERBERA_RESOLUTION_H
#define GERBERA_RESOLUTION_H

#include <cstdint>
#include <string>

class Resolution {
public:
    explicit Resolution(const std::string& string);
    Resolution(std::uint64_t x, std::uint64_t y);
    [[nodiscard]] std::uint64_t x() const;
    [[nodiscard]] std::uint64_t y() const;
    [[nodiscard]] std::string string() const;

private:
    std::uint64_t _x {};
    std::uint64_t _y {};
};

#endif // GERBERA_RESOLUTION_H
