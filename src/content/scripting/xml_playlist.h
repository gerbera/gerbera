/*MT*

    MediaTomb - http://www.mediatomb.cc/

    xml_playlist.h - this file is part of MediaTomb.

    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>

    Copyright (C) 2006-2010 Gena Batyan <bgeradz@mediatomb.cc>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>,
                            Leonhard Wimmer <leo@mediatomb.cc>

    Copyright (C) 2016-2026 Gerbera Contributors

    MediaTomb is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.

    MediaTomb is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    version 2 along with MediaTomb; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.

    $Id$
*/

/// @file content/scripting/xml_playlist.h

#ifndef __GRB_XML_PLAYLIST__
#define __GRB_XML_PLAYLIST__

#include "util/asx_playlist.h"
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
    virtual ~XmlPlaylist() = default;
    virtual unsigned int getParseFlags() const { return pugi::parse_default | pugi::parse_doctype; }
    static std::unique_ptr<XmlPlaylist> create(XmlPlaylistType type);
    static std::unique_ptr<XmlPlaylist> create(XmlPlaylistType type, AsxPlaylistLoader loader);
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
    explicit AsxPlaylist(AsxPlaylistLoader loader);
    unsigned int getParseFlags() const override { return pugi::parse_default; }
    std::optional<std::reference_wrapper<const XmlPlaylistEntry>> readPlaylistEntry() override;

protected:
    void readEntries(pugi::xml_node root, const fs::path& path) override;

private:
    AsxPlaylistLoader loader;
    AsxPlaylistPlan plan;
    XmlPlaylistEntry currentEntry;
};

#endif // __GRB_XML_PLAYLIST__
