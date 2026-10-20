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

#include "playlist_uri.h"

#include "string_converter.h"
#include "tools.h"

#include <algorithm>

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

std::string resolvePlaylistUri(const std::string& reference, const std::string& base)
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

std::string playlistPathUri(const fs::path& path)
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
    std::size_t i = 0;
    while (i < text.size()) {
        unsigned char c = text[i];
        if (c == '%') {
            if (text.size() - i < 3)
                return std::nullopt;
            int hi = hex(text[i + 1]);
            int lo = hex(text[i + 2]);
            if (hi < 0 || lo < 0)
                return std::nullopt;
            c = static_cast<unsigned char>((hi << 4) | lo);
            i += 3;
        } else
            ++i;
        if (c == 0)
            return std::nullopt;
        result += static_cast<char>(c);
    }
    StringConverter utf8("UTF-8", "UTF-8");
    return utf8.validate(result) ? std::make_optional(result) : std::nullopt;
}

std::optional<std::string> resolvePlaylistMediaReference(std::string text, const std::string& base, bool nativePath)
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
    auto uri = splitUri(resolvePlaylistUri(text, base));
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
