/*MT*

    MediaTomb - http://www.mediatomb.cc/

    xml_playlist.cc - this file is part of MediaTomb.

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

/// @file content/scripting/xml_playlist.cc

#define GRB_LOG_FAC GrbLogFacility::script

#include "xml_playlist.h"

#include "exceptions.h"
#include "util/logger.h"
#include "util/tools.h"
#include "util/url_utils.h"

#include <regex>
#include <stdexcept>
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

std::unique_ptr<XmlPlaylist> XmlPlaylist::create(XmlPlaylistType type, AsxPlaylistLoader loader)
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

AsxPlaylist::AsxPlaylist(AsxPlaylistLoader loader)
    : loader(std::move(loader))
{
    if (!this->loader)
        throw_std_runtime_error("ASX requires an explicit ENTRYREF loader");
}

void AsxPlaylist::readEntries(pugi::xml_node root, const fs::path& path)
{
    plan = {};
    currentEntry = {};
    // The complete bounded plan owns its strings; no caller DOM is retained.
    plan = readAsxPlaylistPlan(root, fs::weakly_canonical(path), loader);
    for (const auto& warning : plan.warnings)
        log_warning("{}", warning);
}

std::optional<std::reference_wrapper<const XmlPlaylistEntry>> AsxPlaylist::readPlaylistEntry()
{
    currentEntry = {};
    if (!readComplete)
        return std::nullopt;
    // Preserve the existing ASX BASE extension's ASCII scheme policy.
    static const std::regex scheme("^[a-zA-Z][a-zA-Z0-9+.-]*:");
    while (entryPosition < plan.entries.size()) {
        const auto& entry = plan.entries[entryPosition++];
        currentEntry = {};
        currentEntry.title = entry.metadata.title;
        currentEntry.description = entry.metadata.description;
        if (!entry.metadata.author.empty())
            currentEntry.defaults["upnp:author"] = entry.metadata.author;
        if (!entry.metadata.copyright.empty())
            currentEntry.defaults["dc:rights"] = entry.metadata.copyright;
        for (const auto& [name, value] : entry.metadata.parameters) {
            if (name == "size")
                currentEntry.size = value;
            else if (name == "mimetype")
                currentEntry.mimetype = value;
            else if (name == "protocol")
                currentEntry.protocol = value;
            else
                currentEntry.extra[name] = value;
        }
        auto base = entry.base;
        while (!base.empty() && base.back() == '/')
            base.pop_back();
        for (const auto& reference : entry.references) {
            auto href = reference.href;
            if (!entry.base.empty() && href.front() != '/' && !std::regex_search(href, scheme))
                href = base + '/' + href;
            try {
                if (auto location = URLUtils::resolvePlaylistReference(href, entry.source)) {
                    currentEntry.locations.push_back(*location);
                    currentEntry.writeThroughs.push_back(reference.writeThrough);
                }
            } catch (const std::runtime_error& e) {
                log_warning("Cannot resolve playlist reference {}: {}", href, EX(e));
            }
        }
        if (!currentEntry.locations.empty())
            return std::cref(currentEntry);
    }
    return std::nullopt;
}
