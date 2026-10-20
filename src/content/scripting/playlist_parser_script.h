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
/// Local references become absolute paths; HTTP(S) references remain encoded
/// URIs.
std::vector<XmlPlaylistEntry> readXmlPlaylist(const pugi::xml_document& doc,
    const fs::path& path,
    XmlPlaylistType type);

#ifdef HAVE_JS
#include "parser_script.h"
#include "util/asx_playlist.h"

#include <functional>
#include <optional>

class PlaylistParserScript : public ParserScript {
public:
    PlaylistParserScript(const std::shared_ptr<Content>& content, const std::string& parent);
    std::vector<int> processPlaylistObject(const std::shared_ptr<CdsObject>& obj, std::shared_ptr<GenericTask> task, const std::string& rootPath, bool& hasResult);
    std::optional<std::reference_wrapper<const XmlPlaylistEntry>> readPlaylistEntry();
    std::optional<std::reference_wrapper<const AsxPlaylistEntry>> readAsxPlaylistEntry();

    std::pair<std::shared_ptr<CdsObject>, int> createObject2cdsObject(const std::shared_ptr<CdsObject>& origObject, const std::string& rootPath) override;
    bool setRefId(const std::shared_ptr<CdsObject>& cdsObj, const std::shared_ptr<CdsObject>& origObject, int pcdId) override;

protected:
    std::string playlistFunction;
    bool linkObjects;
    bool followSymlinks;
    bool hidden;
    TextFileCharset charsetMode;
    std::vector<XmlPlaylistEntry> xmlEntries;
    std::size_t xmlPosition {};
    std::vector<AsxPlaylistEntry> asxEntries;
    std::size_t asxPosition {};

    void handleObject2cdsItem(duk_context* ctx, const std::shared_ptr<CdsObject>& pcd, const std::shared_ptr<CdsItem>& item) override;
    void handleObject2cdsContainer(duk_context* ctx, const std::shared_ptr<CdsObject>& pcd, const std::shared_ptr<CdsContainer>& cont) override;
};

#endif // HAVE_JS

#endif // __PLAYLIST_PARSER_SCRIPT_H__
