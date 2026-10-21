/*GRB*

Gerbera - https://gerbera.io/

    url_utils.cc - this file is part of Gerbera.

    Copyright (C) 2022-2026 Gerbera Contributors

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

/// @file util/url_utils.cc
#define GRB_LOG_FAC GrbLogFacility::content
#include "url_utils.h" // API

#include "exceptions.h"
#include "upnp/compat.h"
#include "util/fmt_compat.h"

#include "util/string_converter.h"
#include "util/tools.h"

#include <algorithm>
#include <cstring>
#include <sstream>

// URL FORMATTING CONSTANTS
#define URL_UI_PARAM_SEPARATOR '?'
#define _URL_PARAM_SEPARATOR '/'

namespace URLUtils {

/// @brief Splits the url into a path and parameters string.
/// Only '?' and '/' separators are allowed, otherwise an exception will
/// be thrown.
/// This function splits the url into its path and parameter components.
/// content/media SEPARATOR object_id=12345&transcode=wav would be transformed to:
/// path = "content/media"
/// parameters = "object_id=12345&transcode=wav"
std::pair<std::string_view, std::string_view> splitUrl(std::string_view url, char separator)
{
    std::size_t splitPos;
    switch (separator) {
    case _URL_PARAM_SEPARATOR:
        splitPos = url.rfind(_URL_PARAM_SEPARATOR);
        break;
    case URL_UI_PARAM_SEPARATOR:
        splitPos = url.find(URL_UI_PARAM_SEPARATOR);
        break;
    default:
        throw_std_runtime_error("Forbidden separator: {}", separator);
    }

    if (splitPos == std::string_view::npos)
        return { url, std::string_view() };

    return { url.substr(0, splitPos), url.substr(splitPos + 1) };
}

std::string_view getQuery(std::string_view url)
{
    return splitUrl(url, URL_UI_PARAM_SEPARATOR).second;
}

std::string_view getFile(std::string_view url)
{
    return splitUrl(url, URL_UI_PARAM_SEPARATOR).first;
}

std::string joinUrl(const std::vector<std::string>& components, bool addToEnd, std::string_view separator)
{
    if (components.empty())
        return std::string(separator);
    return fmt::format("{}{}{}", separator, fmt::join(components, separator), (addToEnd ? separator : ""));
}

std::map<std::string, std::string> parseParameters(std::string_view filename, std::string_view baseLink)
{
    const auto parameters = filename.substr(baseLink.size());
    return pathToMap(parameters);
}

static constexpr auto hexCharS2 = "0123456789ABCDEF";

/// @brief Converts a string to a URL (meaning: %20 instead of space and so on)
/// @param str String to be converted.
/// @return string that contains the url-escaped representation of the original string.
std::string urlEscape(std::string_view str)
{
    std::ostringstream buf;
    for (std::size_t i = 0; i < str.length();) {
        auto c = str[i];
        int cplen = 1;
        if ((c & 0xf8) == 0xf0)
            cplen = 4;
        else if ((c & 0xf0) == 0xe0)
            cplen = 3;
        else if ((c & 0xe0) == 0xc0)
            cplen = 2;
        if ((i + cplen) > str.length())
            cplen = 1;

        if ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_' || c == '-' || c == '.') {
            buf << static_cast<char>(c);
        } else {
            int hi = c >> 4;
            int lo = c & 15;
            if (cplen > 1)
                buf << str.substr(i, cplen);
            else
                buf << '%' << hexCharS2[hi] << hexCharS2[lo];
        }
        i += cplen;
    }
    return buf.str();
}

/// @brief Opposite of urlEscape :)
std::string urlUnescape(std::string_view str)
{
    auto data = str.data();
    std::size_t len = str.length();
    std::ostringstream buf;

    std::size_t i = 0;
    while (i < len) {
        const char c = data[i++];
        if (c == '%') {
            if (i + 2 > len)
                break; // avoid buffer overrun
            char chi = data[i++];
            char clo = data[i++];

            auto pos = std::strchr(hexCharS2, chi);
            int hi = pos ? pos - hexCharS2 : 0;

            pos = std::strchr(hexCharS2, clo);
            int lo = pos ? pos - hexCharS2 : 0;

            int ascii = (hi << 4) | lo;
            buf << static_cast<char>(ascii);
        } else if (c == '+') {
            buf << ' ';
        } else {
            buf << c;
        }
    }
    return buf.str();
}

static std::string dictEncode(const std::map<std::string, std::string>& dict, std::string_view sep1, char sep2)
{
    std::vector<std::string> items;
    items.reserve(dict.size());

    for (const auto& [k, v] : dict)
        items.push_back(fmt::format("{}{}{}", urlEscape(k), sep2, urlEscape(v)));

    return fmt::format("{}", fmt::join(items, sep1));
}

std::string dictEncode(const std::map<std::string, std::string>& dict)
{
    return dictEncode(dict, "&", '=');
}

std::string dictEncodeSimple(const std::map<std::string, std::string>& dict)
{
    return dictEncode(dict, "/", '/');
}

std::map<std::string, std::string> dictDecode(std::string_view url, bool unEscape)
{
    std::map<std::string, std::string> dict;
    const char* data = url.data();
    const char* dataEnd = data + url.length();
    while (data < dataEnd) {
        const char* ampPos = std::strchr(data, '&');
        if (!ampPos) {
            ampPos = dataEnd;
        }
        const char* eqPos = std::strchr(data, '=');
        if (eqPos && eqPos < ampPos) {
            auto key = std::string_view(data, eqPos - data);
            auto value = std::string_view(eqPos + 1, ampPos - eqPos - 1);
            if (unEscape) {
                dict.try_emplace(GrbUrlUnescape(key), GrbUrlUnescape(value));
            } else {
                dict.emplace(key, value);
            }
        }
        data = ampPos + 1;
    }
    return dict;
}

std::map<std::string, std::string> pathToMap(std::string_view url)
{
    std::map<std::string, std::string> out;
    std::size_t pos;
    std::size_t lastPos = 0;
    std::size_t size = url.size();
    do {
        pos = url.find('/', lastPos);
        if (pos == std::string_view::npos)
            pos = size;

        std::string key = urlUnescape(url.substr(lastPos, pos - lastPos));
        lastPos = pos == size ? size : pos + 1;
        pos = url.find('/', lastPos);
        if (pos == std::string::npos)
            pos = size;

        std::string value = urlUnescape(url.substr(lastPos, pos - lastPos));
        lastPos = pos + 1;

        out.emplace(key, value);
    } while (lastPos < size);

    return out;
}

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
    return (uri.scheme.empty() ? "" : uri.scheme + ":") + (uri.hasAuthority ? "//" + uri.authority : "") + uri.path + (uri.hasQuery ? "?" + uri.query : "") + (uri.hasFragment ? "#" + uri.fragment : "");
}

std::string resolveUriReference(const std::string& reference,
    const std::string& base)
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
                ref.path = (slash == std::string::npos ? (parent.hasAuthority ? "/" : "")
                                                       : parent.path.substr(0, slash + 1))
                    + ref.path;
            }
        }
    }
    ref.path = removeDotSegments(ref.path);
    return joinUri(ref);
}

std::string filePathToUri(const fs::path& path)
{
    static constexpr char hex[] = "0123456789ABCDEF";
    std::string result = "file://";
    auto text = fs::absolute(path).generic_string();
    if (text[0] != '/')
        result += '/';
    for (unsigned char c : text) {
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || std::string_view("/-._~:").find(c) != std::string_view::npos)
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

std::optional<std::string> resolveMediaReference(std::string text,
    const std::string& base,
    bool nativePath)
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
        return (path.is_absolute() ? path : fs::path(*folder).parent_path() / path)
            .lexically_normal()
            .string();
    }
    auto uri = splitUri(resolveUriReference(text, base));
    if (uri.scheme == "http" || uri.scheme == "https")
        return uri.hasAuthority && !uri.authority.empty()
            ? std::make_optional(joinUri(uri))
            : std::nullopt;
    if (uri.scheme != "file" || uri.hasQuery || uri.hasFragment || (!uri.authority.empty() && toLower(uri.authority) != "localhost"))
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
    return fs::path(*decoded).is_absolute()
        ? std::make_optional(
              fs::path(*decoded).lexically_normal().string())
        : std::nullopt;
}

} // namespace URLUtils
