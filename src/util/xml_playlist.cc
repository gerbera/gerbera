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

#include "xml_playlist.h"

#include "exceptions.h"
#include "playlist_uri.h"
#include "tools.h"

#include <pugixml.hpp>

static constexpr auto xspfNamespace = "http://xspf.org/ns/0/";

static std::string namespaceUri(pugi::xml_node node)
{
    std::string name = node.name();
    auto colon = name.find(':');
    auto attribute = colon == std::string::npos ? "xmlns" : "xmlns:" + name.substr(0, colon);
    for (auto parent = node; parent; parent = parent.parent()) {
        if (auto declaration = parent.attribute(attribute.c_str()))
            return declaration.value();
    }
    return {};
}

static bool named(pugi::xml_node node, const std::string& local, const std::string& ns = "")
{
    std::string name = node.name();
    auto colon = name.find(':');
    return name.substr(colon == std::string::npos ? 0 : colon + 1) == local && namespaceUri(node) == ns;
}

static std::string nodeBase(pugi::xml_node node, const fs::path& path)
{
    std::vector<std::string> bases;
    for (auto parent = node; parent; parent = parent.parent()) {
        if (auto base = parent.attribute("xml:base"))
            bases.emplace_back(base.value());
    }
    auto result = playlistPathUri(path);
    for (auto it = bases.rbegin(); it != bases.rend(); ++it)
        result = resolvePlaylistUri(*it, result);
    return result;
}

std::vector<XmlPlaylistEntry> readXmlPlaylist(const pugi::xml_document& doc, const fs::path& path, XmlPlaylistType type)
{
    int roots = 0;
    for (auto node : doc.children()) {
        if (node.type() == pugi::node_doctype)
            throw_std_runtime_error("XML playlists must not contain a DOCTYPE");
        if (node.type() == pugi::node_element)
            ++roots;
    }
    if (roots != 1)
        throw_std_runtime_error("XML playlists must have exactly one root element");
    std::vector<XmlPlaylistEntry> entries;
    auto root = doc.document_element();
    if (type == XmlPlaylistType::Wpl) {
        if (!named(root, "smil"))
            throw_std_runtime_error("WPL requires a smil root element");
        auto body = root.child("body");
        auto seq = body.child("seq");
        if (!named(body, "body") || !named(seq, "seq") || body.next_sibling("body") || seq.next_sibling("seq"))
            throw_std_runtime_error("WPL requires body/seq");
        for (auto node : seq.children()) {
            if (named(node, "smartPlaylist"))
                throw_std_runtime_error("Dynamic WPL smartPlaylist queries are not supported");
            if (!named(node, "media"))
                continue;
            if (auto reference = resolvePlaylistMediaReference(node.attribute("src").value(), nodeBase(node, path), true))
                entries.push_back({ { *reference }, {} });
        }
    } else {
        if (!named(root, "playlist", xspfNamespace) || std::string(root.attribute("version").value()) != "1")
            throw_std_runtime_error("XSPF requires playlist version 1 in the XSPF namespace");
        pugi::xml_node trackList;
        for (auto node : root.children()) {
            if (named(node, "trackList", xspfNamespace)) {
                if (trackList)
                    throw_std_runtime_error("XSPF requires exactly one trackList");
                trackList = node;
            }
        }
        if (!trackList)
            throw_std_runtime_error("XSPF requires exactly one trackList");
        for (auto track : trackList.children()) {
            if (!named(track, "track", xspfNamespace))
                continue;
            XmlPlaylistEntry entry;
            for (auto node : track.children()) {
                if (named(node, "title", xspfNamespace))
                    entry.title = node.text().as_string();
                if (named(node, "location", xspfNamespace)) {
                    if (auto reference = resolvePlaylistMediaReference(node.text().as_string(), nodeBase(node, path), false))
                        entry.locations.push_back(*reference);
                }
            }
            if (!entry.locations.empty())
                entries.push_back(std::move(entry));
        }
    }
    return entries;
}
