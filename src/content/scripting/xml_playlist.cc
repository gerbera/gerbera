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

#include "exceptions.h"
#include "util/logger.h"
#include "util/tools.h"
#include "util/url_utils.h"

#include <utility>

static constexpr auto xspfNamespace = "http://xspf.org/ns/0/";

std::unique_ptr<XmlPlaylist> XmlPlaylist::create(XmlPlaylistType type)
{
    if (type == XmlPlaylistType::Asx)
        return std::make_unique<AsxPlaylist>();
    if (type == XmlPlaylistType::Wpl)
        return std::make_unique<WplPlaylist>();
    return std::make_unique<XspfPlaylist>();
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

static pugi::xml_attribute asxAttribute(pugi::xml_node node, const char* name)
{
    pugi::xml_attribute result;
    for (auto attribute : node.attributes()) {
        if (toLower(attribute.name()) == name)
            result = attribute;
    }
    return result;
}

void AsxPlaylist::readEntries(pugi::xml_node root, const fs::path& path)
{
    document.reset();
    node = {};
    parents.clear();
    source = path;
    base.clear();
    pendingEntry = {};
    currentEntry = {};
    if (toLower(root.name()) != "asx")
        throw_std_runtime_error("ASX requires an asx root element");
    // Keep the iterator independent of the caller's pugixml document lifetime.
    node = document.append_copy(root);
    if (!node)
        throw_std_runtime_error("Cannot copy ASX XML document");
}

std::optional<std::reference_wrapper<const XmlPlaylistEntry>> AsxPlaylist::readPlaylistEntry()
{
    currentEntry = {};
    if (!readComplete)
        return std::nullopt;
    while (node || !parents.empty()) {
        if (!node) {
            node = parents.back().next_sibling();
            parents.pop_back();
            currentEntry = std::move(pendingEntry);
            pendingEntry = {};
            if (currentEntry.locations.empty())
                continue;
            // Legacy ASX uses a root BASE prefix and the last REF in an ENTRY.
            auto reference = currentEntry.locations.back();
            if (!base.empty())
                reference = base + '/' + reference;
            std::optional<std::string> location;
            try {
                location = URLUtils::resolvePlaylistReference(reference, source);
            } catch (const std::runtime_error& e) {
                log_warning("Cannot resolve playlist reference {}: {}", reference, EX(e));
                continue;
            }
            if (!location)
                continue;
            currentEntry.locations = { *location };
            return std::cref(currentEntry);
        }
        const auto name = toLower(node.name());
        if (name == "asx" || name == "entry") {
            parents.push_back(node);
            node = node.first_child();
            continue;
        }
        const auto href = asxAttribute(node, "href").as_string();
        if (name == "ref" && *href) {
            pendingEntry.locations = { href };
            pendingEntry.writeThrough = asxAttribute(node, "writethrough").as_string("-1");
        } else if (name == "base" && *href && parents.size() < 2) {
            base = href;
        } else if (name == "title") {
            pendingEntry.title = node.text().as_string();
        } else if (name == "abstract") {
            pendingEntry.description = node.text().as_string();
        } else if (name == "param") {
            const std::string parameter = asxAttribute(node, "name").as_string();
            const std::string value = asxAttribute(node, "value").as_string();
            if (parameter == "size")
                pendingEntry.size = value;
            else if (parameter == "mimetype")
                pendingEntry.mimetype = value;
            else if (parameter == "protocol")
                pendingEntry.protocol = value;
            else
                pendingEntry.extra[parameter] = value;
        }
        node = node.next_sibling();
    }
    return std::nullopt;
}
