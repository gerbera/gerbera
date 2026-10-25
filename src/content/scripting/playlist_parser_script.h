/*MT*

    MediaTomb - http://www.mediatomb.cc/

    playlist_parser_script.h - this file is part of MediaTomb.

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

/// @file content/scripting/playlist_parser_script.h

#ifndef __PLAYLIST_PARSER_SCRIPT_H__
#define __PLAYLIST_PARSER_SCRIPT_H__

#include "util/grb_fs.h"
#ifdef HAVE_JS
#include "parser_script.h"
#endif

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
};

/// Own the validated XML playlist positions independently of the optional JS runtime.
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

#ifdef HAVE_JS
class PlaylistParserScript : public ParserScript {
public:
    PlaylistParserScript(const std::shared_ptr<Content>& content, const std::string& parent);
    std::vector<int> processPlaylistObject(const std::shared_ptr<CdsObject>& obj, std::shared_ptr<GenericTask> task, const std::string& rootPath, bool& hasResult);
    std::optional<std::reference_wrapper<const XmlPlaylistEntry>> readPlaylistEntry();
    static void pushPlaylistEntry(duk_context* ctx, const XmlPlaylistEntry& entry);

    std::pair<std::shared_ptr<CdsObject>, int> createObject2cdsObject(const std::shared_ptr<CdsObject>& origObject, const std::string& rootPath) override;
    bool setRefId(const std::shared_ptr<CdsObject>& cdsObj, const std::shared_ptr<CdsObject>& origObject, int pcdId) override;

protected:
    std::string playlistFunction;
    bool linkObjects;
    bool followSymlinks;
    bool hidden;
    TextFileCharset charsetMode;
    std::unique_ptr<XmlPlaylist> xmlPlaylist;

    void handleObject2cdsItem(duk_context* ctx, const std::shared_ptr<CdsObject>& pcd, const std::shared_ptr<CdsItem>& item) override;
    void handleObject2cdsContainer(duk_context* ctx, const std::shared_ptr<CdsObject>& pcd, const std::shared_ptr<CdsContainer>& cont) override;
};

#endif // HAVE_JS

#endif // __PLAYLIST_PARSER_SCRIPT_H__
