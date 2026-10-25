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

#include <map>
#include <memory>
#include <pugixml.hpp>
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
};

/// @brief Own the validated XML playlist positions independently of the optional JS runtime.
class XmlPlaylist {
public:
    virtual ~XmlPlaylist() = default;
    static std::unique_ptr<XmlPlaylist> create(XmlPlaylistType type);
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

class AsxPlaylist : public XmlPlaylist {
public:
    std::optional<std::reference_wrapper<const XmlPlaylistEntry>> readPlaylistEntry() override;

protected:
    void readEntries(pugi::xml_node root, const fs::path& path) override;

private:
    pugi::xml_document document;
    pugi::xml_node node;
    std::vector<pugi::xml_node> parents;
    fs::path source;
    std::string base;
    XmlPlaylistEntry pendingEntry;
    XmlPlaylistEntry currentEntry;
};

#endif // __GRB_XML_PLAYLIST__
