/*GRB*
    Gerbera - https://gerbera.io/

    Copyright (C) 2026 Gerbera Contributors

    This file is part of Gerbera, licensed under the GNU General Public
    License version 2 or later. See <http://www.gnu.org/licenses/>.
*/

/// @file util/asx_playlist.h

#ifndef GERBERA_ASX_PLAYLIST_H
#define GERBERA_ASX_PLAYLIST_H

#include "grb_fs.h"

#include <functional>
#include <map>
#include <memory>
#include <pugixml.hpp>
#include <string>
#include <vector>

struct AsxPlaylistReference {
    std::string href;
    int writeThrough { -1 };
};

struct AsxPlaylistMetadata {
    std::string title;
    std::string author;
    std::string copyright;
    std::string description;
    std::map<std::string, std::string> parameters;
};

struct AsxPlaylistEntry {
    fs::path source;
    std::string base;
    AsxPlaylistMetadata metadata;
    std::vector<AsxPlaylistReference> references;
};

struct AsxPlaylistPlan {
    AsxPlaylistMetadata metadata;
    std::vector<AsxPlaylistEntry> entries;
    std::vector<std::string> warnings;
};

struct AsxPlaylistDocument {
    /// The loader resolves references with the shared URI policy and returns
    /// a canonical local path, including symlink resolution for cycle checks.
    fs::path source;
    std::unique_ptr<pugi::xml_document> xml;
    /// A missing/unsupported child is skipped with this diagnostic. Invalid
    /// XML must throw instead so no partial structural plan can be imported.
    std::string warning;
};

using AsxPlaylistLoader = std::function<AsxPlaylistDocument(
    const std::string& href, const fs::path& source)>;

/// Builds a complete bounded plan without importing objects or fetching URLs.
/// REF alternatives remain ordered; the content layer chooses the first
/// usable candidate using its actual media importer.
AsxPlaylistPlan readAsxPlaylistPlan(const pugi::xml_node& root,
    const fs::path& source,
    const AsxPlaylistLoader& loader);

#endif // GERBERA_ASX_PLAYLIST_H
