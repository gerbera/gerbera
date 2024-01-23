/*GRB*

Gerbera - https://gerbera.io/

    url_utils.cc - this file is part of Gerbera.

    Copyright (C) 2022-2024 Gerbera Contributors

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

/// \file url_utils.cc

#include "url_utils.h" // API

#include <fmt/format.h>
#include <sstream>

#include "common.h"
#include "exceptions.h"

namespace URLUtils {

/// \brief Splits the url into a path and parameters string.
/// Only '?' and '/' separators are allowed, otherwise an exception will
/// be thrown.
/// \param url URL that has to be processed
/// \return pair of path and parameters which reference the input-view of url
///
/// This function splits the url into its path and parameter components.
/// content/media SEPARATOR object_id=12345&transcode=wav would be transformed to:
/// path = "content/media"
/// parameters = "object_id=12345&transcode=wav"
std::pair<std::string_view, std::string_view> splitUrl(std::string_view url, char separator)
{
    std::size_t splitPos;
    switch (separator) {
    case '/':
        splitPos = url.rfind('/');
        break;
    case '?':
        splitPos = url.find('?');
        break;
    default:
        throw_std_runtime_error("Forbidden separator: {}", separator);
    }

    if (splitPos == std::string_view::npos)
        return { url, std::string_view() };

    return { url.substr(0, splitPos), url.substr(splitPos + 1) };
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

/// \brief Converts a string to a URL (meaning: %20 instead of space and so on)
/// \param str String to be converted.
/// \return string that contains the url-escaped representation of the original string.
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

/// \brief Opposite of urlEscape :)
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

static std::string dictEncode(const std::map<std::string, std::string>& dict, char sep1, char sep2)
{
    std::ostringstream buf;
    for (auto it = dict.begin(); it != dict.end(); ++it) {
        if (it != dict.begin())
            buf << sep1;
        buf << urlEscape(it->first) << sep2
            << urlEscape(it->second);
    }
    return buf.str();
}

std::string dictEncode(const std::map<std::string, std::string>& dict)
{
    return dictEncode(dict, '&', '=');
}

std::string dictEncodeSimple(const std::map<std::string, std::string>& dict)
{
    return dictEncode(dict, '/', '/');
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
                dict.try_emplace(urlUnescape(key), urlUnescape(value));
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

} // namespace URLUtils
