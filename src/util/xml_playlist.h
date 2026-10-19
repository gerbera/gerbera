/*GRB*
    Gerbera - https://gerbera.io/
    Copyright (C) 2026 Gerbera Contributors
    SPDX-License-Identifier: GPL-2.0-only
*/

#ifndef GERBERA_XML_PLAYLIST_H
#define GERBERA_XML_PLAYLIST_H

#include "grb_fs.h"

#include <string>
#include <vector>

enum class XmlPlaylistType { Wpl,
    Xspf };

struct XmlPlaylistEntry {
    // XSPF locations are alternatives for one position, not additional tracks.
    std::vector<std::string> locations;
    std::string title;
};

/// Read and validate the entire XML before the caller changes playlist contents.
/// Local references become absolute paths; HTTP(S) references remain encoded URIs.
std::vector<XmlPlaylistEntry> readXmlPlaylist(const fs::path& path, XmlPlaylistType type);

#endif
