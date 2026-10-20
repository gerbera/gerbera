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

#ifndef GERBERA_XML_PLAYLIST_H
#define GERBERA_XML_PLAYLIST_H

#include "grb_fs.h"

#include <pugixml.hpp>
#include <string>
#include <vector>

enum class XmlPlaylistType { Wpl,
    Xspf };

struct XmlPlaylistEntry {
    // XSPF locations are alternatives for one position, not additional tracks.
    std::vector<std::string> locations;
    std::string title;
};

/// Validate the entire parsed XML before the caller changes playlist contents.
/// Local references become absolute paths; HTTP(S) references remain encoded URIs.
std::vector<XmlPlaylistEntry> readXmlPlaylist(const pugi::xml_document& doc, const fs::path& path, XmlPlaylistType type);

#endif
