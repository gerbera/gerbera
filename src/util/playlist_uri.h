/*GRB*
    Gerbera - https://gerbera.io/
    Copyright (C) 2026 Gerbera Contributors
    SPDX-License-Identifier: GPL-2.0-only

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

#ifndef GERBERA_PLAYLIST_URI_H
#define GERBERA_PLAYLIST_URI_H

#include "grb_fs.h"

#include <optional>
#include <string>

/// Encode a native playlist location as an absolute file URI.
std::string playlistPathUri(const fs::path& path);
/// Resolve a URI reference against a base according to RFC 3986 section 5.2.
std::string resolvePlaylistUri(const std::string& reference, const std::string& base);
/// Resolve a media reference to a local path or encoded HTTP(S) URI.
/// Native paths preserve literal percent, query and fragment characters.
std::optional<std::string> resolvePlaylistMediaReference(std::string text, const std::string& base, bool nativePath);

#endif
