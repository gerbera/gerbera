/*GRB*
    Gerbera - https://gerbera.io/
    Copyright (C) 2026 Gerbera Contributors
    SPDX-License-Identifier: GPL-2.0-only
*/

#include "xml_playlist.h"

#include "exceptions.h"
#include "string_converter.h"
#include "tools.h"

#include <algorithm>
#include <optional>
#include <pugixml.hpp>

static constexpr auto xspfNamespace = "http://xspf.org/ns/0/";

struct PlaylistUri {
    std::string scheme;
    std::string authority;
    std::string path;
    std::string query;
    std::string fragment;
    bool hasAuthority {};
    bool hasQuery {};
    bool hasFragment {};
};

static PlaylistUri splitUri(std::string text)
{
    PlaylistUri uri;
    auto pos = text.find('#');
    if (pos != std::string::npos) {
        uri.hasFragment = true;
        uri.fragment = text.substr(pos + 1);
        text.resize(pos);
    }
    pos = text.find('?');
    if (pos != std::string::npos) {
        uri.hasQuery = true;
        uri.query = text.substr(pos + 1);
        text.resize(pos);
    }
    pos = text.find(':');
    if (pos != std::string::npos && pos < text.find('/')) {
        uri.scheme = toLower(text.substr(0, pos));
        text.erase(0, pos + 1);
    }
    if (text.compare(0, 2, "//") == 0) {
        uri.hasAuthority = true;
        pos = text.find('/', 2);
        uri.authority = text.substr(2, pos == std::string::npos ? pos : pos - 2);
        text = pos == std::string::npos ? "" : text.substr(pos);
    }
    uri.path = std::move(text);
    return uri;
}

// RFC 3986 section 5.2.4: preserve empty segments and trailing slashes.
static std::string removeDotSegments(std::string input)
{
    std::string output;
    while (!input.empty()) {
        if (input.compare(0, 3, "../") == 0)
            input.erase(0, 3);
        else if (input.compare(0, 2, "./") == 0)
            input.erase(0, 2);
        else if (input.compare(0, 3, "/./") == 0 || input == "/.")
            input.replace(0, 3, "/");
        else if (input.compare(0, 4, "/../") == 0 || input == "/..") {
            input.replace(0, 4, "/");
            auto slash = output.rfind('/');
            output.resize(slash == std::string::npos ? 0 : slash);
        } else if (input == "." || input == "..")
            input.clear();
        else {
            auto end = input.find('/', input[0] == '/' ? 1 : 0);
            output += input.substr(0, end);
            input = end == std::string::npos ? "" : input.substr(end);
        }
    }
    return output;
}

static std::string joinUri(const PlaylistUri& uri)
{
    return (uri.scheme.empty() ? "" : uri.scheme + ":")
        + (uri.hasAuthority ? "//" + uri.authority : "") + uri.path
        + (uri.hasQuery ? "?" + uri.query : "")
        + (uri.hasFragment ? "#" + uri.fragment : "");
}

static std::string resolveUri(const std::string& reference, const std::string& base)
{
    auto ref = splitUri(reference);
    auto parent = splitUri(base);
    if (ref.scheme.empty()) {
        ref.scheme = parent.scheme;
        if (!ref.hasAuthority) {
            ref.hasAuthority = parent.hasAuthority;
            ref.authority = parent.authority;
            if (ref.path.empty()) {
                ref.path = parent.path;
                if (!ref.hasQuery) {
                    ref.hasQuery = parent.hasQuery;
                    ref.query = parent.query;
                }
            } else if (ref.path[0] != '/') {
                auto slash = parent.path.rfind('/');
                ref.path = (slash == std::string::npos ? (parent.hasAuthority ? "/" : "") : parent.path.substr(0, slash + 1)) + ref.path;
            }
        }
    }
    ref.path = removeDotSegments(ref.path);
    return joinUri(ref);
}

static std::string pathUri(const fs::path& path)
{
    static constexpr char hex[] = "0123456789ABCDEF";
    std::string result = "file://";
    auto text = fs::absolute(path).generic_string();
    if (text[0] != '/')
        result += '/';
    for (unsigned char c : text) {
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')
            || (c >= '0' && c <= '9') || std::string_view("/-._~:").find(c) != std::string_view::npos)
            result += static_cast<char>(c);
        else {
            result += '%';
            result += hex[c >> 4];
            result += hex[c & 15];
        }
    }
    return result;
}

static std::optional<std::string> decodePath(const std::string& text)
{
    std::string result;
    auto hex = [](unsigned char c) -> int {
        if (c >= '0' && c <= '9')
            return c - '0';
        if (c >= 'a' && c <= 'f')
            return c - 'a' + 10;
        if (c >= 'A' && c <= 'F')
            return c - 'A' + 10;
        return -1;
    };
    for (std::size_t i = 0; i < text.size(); ++i) {
        unsigned char c = text[i];
        if (c == '%') {
            if (i + 2 >= text.size())
                return std::nullopt;
            int hi = hex(text[++i]);
            int lo = hex(text[++i]);
            if (hi < 0 || lo < 0)
                return std::nullopt;
            c = static_cast<unsigned char>((hi << 4) | lo);
        }
        if (c == 0)
            return std::nullopt;
        result += static_cast<char>(c);
    }
    StringConverter utf8("UTF-8", "UTF-8");
    return utf8.validate(result) ? std::make_optional(result) : std::nullopt;
}

static std::optional<std::string> mediaReference(std::string text, const std::string& base, bool nativePath)
{
    if (!nativePath)
        text = trimString(text);
    if (text.empty())
        return std::nullopt;
    bool drive = text.size() > 1 && text[1] == ':';
    bool unc = text.compare(0, 2, "\\\\") == 0;
#ifndef _WIN32
    if (drive || unc)
        return std::nullopt;
#endif
    if (nativePath && (drive || unc || splitUri(text).scheme.empty())) {
        std::replace(text.begin(), text.end(), '\\', '/');
        auto path = fs::path(text);
        auto parent = splitUri(base);
        auto folder = decodePath(parent.path);
        if (parent.scheme != "file" || !folder)
            return std::nullopt;
        return (path.is_absolute() ? path : fs::path(*folder).parent_path() / path).lexically_normal().string();
    }
    auto uri = splitUri(resolveUri(text, base));
    if (uri.scheme == "http" || uri.scheme == "https")
        return uri.hasAuthority && !uri.authority.empty() ? std::make_optional(joinUri(uri)) : std::nullopt;
    if (uri.scheme != "file" || uri.hasQuery || uri.hasFragment
        || (!uri.authority.empty() && toLower(uri.authority) != "localhost"))
        return std::nullopt;
    auto decoded = decodePath(uri.path);
    if (!decoded)
        return std::nullopt;
#ifdef _WIN32
    if (decoded->size() > 2 && (*decoded)[0] == '/' && (*decoded)[2] == ':')
        decoded->erase(0, 1);
#else
    if (decoded->size() > 2 && (*decoded)[0] == '/' && (*decoded)[2] == ':')
        return std::nullopt;
#endif
    return fs::path(*decoded).is_absolute() ? std::make_optional(fs::path(*decoded).lexically_normal().string()) : std::nullopt;
}

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
    auto result = pathUri(path);
    for (auto it = bases.rbegin(); it != bases.rend(); ++it)
        result = resolveUri(*it, result);
    return result;
}

std::vector<XmlPlaylistEntry> readXmlPlaylist(const fs::path& path, XmlPlaylistType type)
{
    pugi::xml_document doc;
    auto result = doc.load_file(path.c_str(), pugi::parse_default | pugi::parse_doctype);
    if (!result)
        throw_std_runtime_error("Invalid XML playlist: {}", result.description());
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
            if (auto reference = mediaReference(node.attribute("src").value(), nodeBase(node, path), true))
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
                    if (auto reference = mediaReference(node.text().as_string(), nodeBase(node, path), false))
                        entry.locations.push_back(*reference);
                }
            }
            if (!entry.locations.empty())
                entries.push_back(std::move(entry));
        }
    }
    return entries;
}
