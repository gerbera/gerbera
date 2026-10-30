/*GRB*

    Gerbera - https://gerbera.io/

    xml_playlist.cc - this file is part of Gerbera.

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

/// @file content/scripting/xml_playlist.cc

#define GRB_LOG_FAC GrbLogFacility::script

#include "xml_playlist.h"

#include "asx_playlist.h"
#include "exceptions.h"
#include "util/url_utils.h"

#include <utility>

static constexpr auto xspfNamespace = "http://xspf.org/ns/0/";

std::unique_ptr<XmlPlaylist> XmlPlaylist::create(XmlPlaylistType type)
{
    if (type == XmlPlaylistType::Asx)
        throw_std_runtime_error("ASX requires an explicit ENTRYREF loader");
    if (type == XmlPlaylistType::Wpl)
        return std::make_unique<WplPlaylist>();
    return std::make_unique<XspfPlaylist>();
}

std::unique_ptr<XmlPlaylist> XmlPlaylist::create(XmlPlaylistType type, Loader loader)
{
    if (type == XmlPlaylistType::Asx)
        return std::make_unique<AsxPlaylist>(std::move(loader));
    return create(type);
}

void XmlPlaylist::read(const pugi::xml_document& doc,
    const fs::path& path)
{
    entries.clear();
    entryPosition = 0;
    readComplete = false;
    int roots = 0;
    for (auto node : doc.children()) {
        if (node.type() == pugi::node_doctype)
            throw_std_runtime_error("XML playlists must not contain a DOCTYPE");
        if (node.type() == pugi::node_element)
            ++roots;
    }
    if (roots != 1)
        throw_std_runtime_error("XML playlists must have exactly one root element");
    readEntries(doc.document_element(), path);
    readComplete = true;
}

void WplPlaylist::readEntries(pugi::xml_node root, const fs::path& path)
{
    if (!URLUtils::isNamed(root, "smil"))
        throw_std_runtime_error("WPL requires a smil root element");
    auto body = root.child("body");
    auto seq = body.child("seq");
    if (!URLUtils::isNamed(body, "body") || !URLUtils::isNamed(seq, "seq") || body.next_sibling("body") || seq.next_sibling("seq"))
        throw_std_runtime_error("WPL requires body/seq");
    for (auto node : seq.children()) {
        if (URLUtils::isNamed(node, "smartPlaylist"))
            throw_std_runtime_error(
                "Dynamic WPL smartPlaylist queries are not supported");
        if (!URLUtils::isNamed(node, "media"))
            continue;
        if (auto reference = URLUtils::resolveMediaReference(
                node.attribute("src").value(), URLUtils::nodeBase(node, path), URLUtils::MediaReferenceType::NativePath))
            entries.push_back({ { *reference }, {} });
    }
}

void XspfPlaylist::readEntries(pugi::xml_node root, const fs::path& path)
{
    if (!URLUtils::isNamed(root, "playlist", xspfNamespace) || std::string(root.attribute("version").value()) != "1")
        throw_std_runtime_error(
            "XSPF requires playlist version 1 in the XSPF namespace");
    pugi::xml_node trackList;
    for (auto node : root.children()) {
        if (URLUtils::isNamed(node, "trackList", xspfNamespace)) {
            if (trackList)
                throw_std_runtime_error("XSPF requires exactly one trackList");
            trackList = node;
        }
    }
    if (!trackList)
        throw_std_runtime_error("XSPF requires exactly one trackList");
    for (auto track : trackList.children()) {
        if (!URLUtils::isNamed(track, "track", xspfNamespace))
            continue;
        XmlPlaylistEntry entry;
        for (auto node : track.children()) {
            if (URLUtils::isNamed(node, "title", xspfNamespace))
                entry.title = node.text().as_string();
            if (URLUtils::isNamed(node, "location", xspfNamespace)) {
                if (auto reference = URLUtils::resolveMediaReference(
                        node.text().as_string(), URLUtils::nodeBase(node, path), URLUtils::MediaReferenceType::Uri))
                    entry.locations.push_back(*reference);
            }
        }
        if (!entry.locations.empty())
            entries.push_back(std::move(entry));
    }
}

std::optional<std::reference_wrapper<const XmlPlaylistEntry>> XmlPlaylist::readPlaylistEntry()
{
    if (!readComplete || entryPosition >= entries.size())
        return std::nullopt;
    return std::cref(entries[entryPosition++]);
}
