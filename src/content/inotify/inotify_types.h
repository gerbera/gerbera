/*GRB*

    Gerbera - https://gerbera.io/

    inotify_types.h - this file is part of Gerbera.

    Copyright (C) 2024-2025 Gerbera Contributors

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

/// \file inotify_types.h

#ifndef __INOTIFY_TYPES_H__
#define __INOTIFY_TYPES_H__

#ifdef HAVE_INOTIFY

#include <cinttypes>
#include <string>

struct inotify_event;
typedef uint32_t InotifyFlags;

class InotifyUtil {
public:
    /// @brief get string representation for InotifyFlags
    static std::string mapFlags(InotifyFlags flags);

    /// @brief make enum representation from combined string
    static InotifyFlags makeFlags(const std::string& optValue);

    /// @brief make enum representation from string
    static InotifyFlags remapFlag(const std::string& flag);
};

#endif
#endif
