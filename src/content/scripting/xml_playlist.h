/*GRB*

    Gerbera - https://gerbera.io/

    xml_playlist.h - this file is part of Gerbera.

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

    $Id$
*/

/// @file content/scripting/xml_playlist.h

#ifndef __GRB_XML_PLAYLIST__
#define __GRB_XML_PLAYLIST__

#include "util/grb_fs.h"

#include <cstddef>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <pugixml.hpp>
#include <string>
#include <vector>

enum class XmlPlaylistType { Wpl,
    Xspf,
    Asx };

struct XmlPlaylistEntry {
    // XSPF locations are alternatives for one position, not additional tracks.
    std::vector<std::string> locations;
    std::string title;
    // ASX preserves the existing script entry values without changing metadata.
    std::string description {};
    std::string mimetype {};
    std::string protocol {};
    std::string size { "-1" };
    std::string writeThrough { "-1" };
    std::map<std::string, std::string> extra {};
    std::vector<int> writeThroughs {};
    std::map<std::string, std::string> defaults {};
};

/// Own the validated XML playlist positions independently of the optional JS runtime.
class XmlPlaylist {
public:
    struct Document {
        /// The loader resolves references with the shared URI policy and returns
        /// a canonical local path, including symlink resolution for cycle checks.
        fs::path source;
        std::unique_ptr<pugi::xml_document> xml;
        /// A missing/unsupported child is skipped with this diagnostic. Invalid
        /// XML must throw instead so no partial structural plan can be imported.
        std::string warning;
    };

    using Loader = std::function<Document(
        const std::string& href, const fs::path& source)>;

    virtual ~XmlPlaylist() = default;
    virtual unsigned int getParseFlags() const { return pugi::parse_default | pugi::parse_doctype; }
    static std::unique_ptr<XmlPlaylist> create(XmlPlaylistType type);
    static std::unique_ptr<XmlPlaylist> create(XmlPlaylistType type, Loader loader);
    /// Validate the document structure before the script changes playlist contents.
    void read(const pugi::xml_document& doc, const fs::path& path);
    /// Returned entries remain valid until the next iterator call or read.
    virtual std::optional<std::reference_wrapper<const XmlPlaylistEntry>> readPlaylistEntry();
    std::vector<XmlPlaylistEntry> entries;

protected:
    virtual void readEntries(pugi::xml_node root, const fs::path& path) = 0;
    std::size_t entryPosition {};
    bool readComplete {};
};

class WplPlaylist : public XmlPlaylist {
protected:
    void readEntries(pugi::xml_node root, const fs::path& path) override;
};

class XspfPlaylist : public XmlPlaylist {
protected:
    void readEntries(pugi::xml_node root, const fs::path& path) override;
};

#endif // __GRB_XML_PLAYLIST__
