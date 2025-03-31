/*MT*

    MediaTomb - http://www.mediatomb.cc/

    tools.cc - this file is part of MediaTomb.

    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>

    Copyright (C) 2006-2010 Gena Batyan <bgeradz@mediatomb.cc>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>,
                            Leonhard Wimmer <leo@mediatomb.cc>

    Copyright (C) 2016-2025 Gerbera Contributors

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

/// \file tools.cc
#define GRB_LOG_FAC GrbLogFacility::content
#include "tools.h" // API

#include "config/config.h"
#include "contrib/md5.h"
#include "util/logger.h"

#include <algorithm>
#include <numeric>
#include <queue>
#include <regex>
#include <sstream>

#ifdef __HAIKU__
#define _DEFAULT_SOURCE
#endif

#ifdef BSD_NATIVE_UUID
#include <uuid.h>
#else
#include <uuid/uuid.h>
#endif

#ifdef HAVE_ICU
#include <unicode/translit.h>
#include <unicode/ustream.h>
#endif

static constexpr auto hexChars = "0123456789abcdef";

std::vector<std::string> splitString(std::string_view str, char sep, bool empty)
{
    std::vector<std::string> ret;

    std::size_t pos = 0;
    while (pos < str.size()) {
        if (str[pos] == sep) {
            if (pos > 0 || empty)
                ret.emplace_back(str.substr(0, pos));
            str = str.substr(pos + 1);
            pos = 0;
        } else {
            ++pos;
        }
    }

    if (pos > 0 || empty)
        ret.emplace_back(str);

    return ret;
}

std::string camelCaseString(const std::string_view& str)
{
    std::string ret;

    std::size_t pos = 0;
    std::size_t end = 0;
    while (end < str.size()) {
        if (str.at(end) == '-') {
            if (pos < end) {
                ret.append(str.substr(pos, end - pos));
            }
            ++end;
            if (end < str.size())
                ret.push_back(::toupper(str.at(end)));
            pos = end + 1;
        }
        ++end;
    }

    if (pos < end)
        ret.append(str.substr(pos, end));

    return ret;
}

void trimStringInPlace(std::string& str)
{
    str.erase(str.begin(), std::find_if_not(str.begin(), str.end(), ::isspace));
    str.erase(std::find_if_not(str.rbegin(), str.rend(), ::isspace).base(), str.end());
}

std::string trimString(std::string str)
{
    if (str.empty())
        return str;

    trimStringInPlace(str);
    return str;
}

std::string& toLowerInPlace(std::string& str)
{
    std::transform(str.begin(), str.end(), str.begin(), ::tolower);
    return str;
}

std::string toLower(std::string str)
{
    std::transform(str.begin(), str.end(), str.begin(), ::tolower);
    return str;
}

std::string toUpper(std::string str)
{
    std::transform(str.begin(), str.end(), str.begin(), ::toupper);
    return str;
}

std::int32_t stoiString(const std::string& str, std::int32_t def, std::int32_t base)
{
    if (str.empty() || (str[0] == '-' && !std::isdigit(*str.substr(1).c_str())) || (str[0] != '-' && !std::isdigit(*str.c_str())))
        return def;

    try {
        std::size_t pos;
        return std::stoi(str, &pos, base);
    } catch (const std::exception& ex) {
        log_error("{} (input {})", ex.what(), str);
    }
    return def;
}

std::int64_t stolString(const std::string& str, std::int64_t def, std::int64_t base)
{
    if (str.empty() || (str[0] == '-' && !std::isdigit(*str.substr(1).c_str())) || (str[0] != '-' && !std::isdigit(*str.c_str())))
        return def;

    try {
        std::size_t pos;
        return std::stol(str, &pos, base);
    } catch (const std::exception& ex) {
        log_error("{} (input {})", ex.what(), str);
    }
    return def;
}

unsigned long stoulString(const std::string& str, unsigned long def, unsigned long base)
{
    if (str.empty() || (str[0] == '-' && !std::isdigit(*str.substr(1).c_str())) || (str[0] != '-' && !std::isdigit(*str.c_str())))
        return def;

    try {
        std::size_t pos;
        return std::stoul(str, &pos, base);
    } catch (const std::exception& ex) {
        log_error("{} (input {})", ex.what(), str);
    }
    return def;
}

void reduceString(std::string& str, char ch)
{
    auto newEnd = std::unique(str.begin(), str.end(),
        [=](char lhs, char rhs) { return (lhs == rhs) && (lhs == ch); });

    str.erase(newEnd, str.end());
}

void replaceString(std::string& str, std::string_view from, std::string_view to)
{
    auto startPos = str.find(from);
    if (startPos != std::string::npos)
        str.replace(startPos, from.length(), to);
}

void replaceAllString(std::string& str, std::string_view from, std::string_view to)
{
    auto startPos = str.find(from);
    while (startPos != std::string::npos) {
        str.replace(startPos, from.length(), to);
        startPos = str.find(from, startPos + to.length());
    }
}

std::string expandNumbersString(std::string str, std::size_t size)
{
    static const std::regex strRe("([0-9]+)");
    std::smatch strMatch;
    std::string result;
    while (std::regex_search(str, strMatch, strRe)) {
        std::string matchStr = strMatch[1].str();

        if (!matchStr.empty()) {
            auto matchInt = std::stoul(matchStr);
            result += fmt::format("{0}{1:0{2}}", strMatch.prefix().str(), matchInt, matchStr.size() < size ? size : matchStr.size());
            str = strMatch.suffix();
        }
    }
    result += str; // append trailing chars
    return result;
}

std::string httpRedirectTo(std::string_view addr, std::string_view page)
{
    return fmt::format(R"(<html><head><meta http-equiv="Refresh" content="0;URL={}/{}"></head><body bgcolor="#dddddd"></body></html>)", addr, page);
}

std::string hexEncode(const void* data, std::size_t len)
{
    auto buf = std::string(2 * len, '\0');

    auto chars = static_cast<const unsigned char*>(data);
    for (std::size_t i = 0; i < len; i++) {
        unsigned char c = chars[i];
        unsigned char hi = c >> 4;
        unsigned char lo = c & 0xF;
        buf[2 * i + 0] = hexChars[hi];
        buf[2 * i + 1] = hexChars[lo];
    }

    return buf;
}

std::string hexDecodeString(std::string_view encoded)
{
    const char* ptr = encoded.data();
    std::size_t len = encoded.length();

    auto buf = std::string(len / 2, '\0');
    for (std::size_t i = 0; i < len; i += 2) {
        auto chi = std::strchr(hexChars, ptr[i]);
        auto clo = std::strchr(hexChars, ptr[i + 1]);
        std::size_t hi = chi ? chi - hexChars : 0;
        std::size_t lo = clo ? clo - hexChars : 0;

        auto ch = static_cast<char>(hi << 4 | lo);
        buf[i / 2] = ch;
    }
    return buf;
}

std::string hexMd5(const void* data, std::size_t length)
{
    char md5buf[16];

    md5_state_t ctx;
    md5_init(&ctx);
    md5_append(&ctx, const_cast<unsigned char*>(static_cast<const unsigned char*>(data)), length);
    md5_finish(&ctx, reinterpret_cast<unsigned char*>(md5buf));

    return hexEncode(md5buf, 16);
}

std::string hexStringMd5(std::string_view str)
{
    return hexMd5(str.data(), str.length());
}

std::string generateRandomId()
{
#ifdef BSD_NATIVE_UUID
    char* uuidStr;
    std::uint32_t status;
#else
    char uuidStr[37];
#endif
    uuid_t uuid;

#ifdef BSD_NATIVE_UUID
    uuid_create(&uuid, &status);
    uuid_to_string(&uuid, &uuidStr, &status);
#else
    uuid_generate(uuid);
    uuid_unparse(uuid, uuidStr);
#endif

    log_debug("Generated: {}", uuidStr);
    auto uuidString = std::string(uuidStr);
#ifdef BSD_NATIVE_UUID
    free(uuidStr);
#endif

    return uuidString;
}

std::string mimeTypesToCsv(const std::vector<std::string>& mimeTypes)
{
    return mimeTypes.empty() ? "" : fmt::format("http-get:*:{}:*", fmt::join(mimeTypes, ":*,http-get:*:"));
}

std::string renderProtocolInfo(std::string_view mimetype, std::string_view protocol, std::string_view extend)
{
    if (!mimetype.empty() && !protocol.empty()) {
        if (!extend.empty())
            return fmt::format("{}:*:{}:{}", protocol, mimetype, extend);
        return fmt::format("{}:*:{}:*", protocol, mimetype);
    }

    return "http-get:*:*:*";
}

std::string getMTFromProtocolInfo(std::string_view protocol)
{
    std::vector<std::string> parts = splitString(protocol, ':');
    if (parts.size() > 2)
        return parts[2];

    return {};
}

std::string_view getProtocol(std::string_view protocolInfo)
{
    auto pos = protocolInfo.find(':');
    return (pos == std::string_view::npos || pos == 0) ? PROTOCOL : protocolInfo.substr(0, pos);
}

std::string escape(std::string_view string, char escapeChar, char toEscape)
{
    std::ostringstream buf;
    auto len = string.length();

    bool possibleMoreEsc = true;
    bool possibleMoreChar = true;

    std::size_t last = 0;
    do {
        auto nextEsc = std::string::npos;
        if (possibleMoreEsc) {
            nextEsc = string.find(escapeChar, last);
            if (nextEsc == std::string::npos)
                possibleMoreEsc = false;
        }

        auto next = std::string::npos;
        if (possibleMoreChar) {
            next = string.find(toEscape, last);
            if (next == std::string::npos)
                possibleMoreChar = false;
        }

        if (next == std::string::npos || (nextEsc != std::string::npos && nextEsc < next)) {
            next = nextEsc;
        }

        if (next == std::string::npos)
            next = len;
        auto cpLen = static_cast<int>(next - last);
        if (cpLen > 0) {
            buf.write(&string[last], cpLen);
        }
        if (next < len) {
            buf << '\\';
            buf << string.at(next);
        }
        last = next;
        last++;
    } while (last < len);

    return buf.str();
}

std::string fallbackString(const std::string& first, const std::string& fallback)
{
    return first.empty() ? fallback : first;
}

unsigned int stringHash(std::string_view str)
{
    return std::accumulate(str.begin(), str.end(), 5381U, [](auto h, auto ch) { return ((h << 5) + h) ^ ch; });
}

std::string getValueOrDefault(const std::vector<std::pair<std::string, std::string>>& m, const std::string& key, const std::string& defval)
{
    return getValueOrDefault<std::string, std::string>(m, key, defval);
}

std::string getValueOrDefault(const std::map<std::string, std::string>& m, const std::string& key, const std::string& defval)
{
    return getValueOrDefault<std::string, std::string>(m, key, defval);
}

std::vector<std::string> populateCommandLine(const std::string& line,
    const std::string& in,
    const std::string& out,
    const std::string& range,
    const std::string& title)
{
    log_debug("Template: '{}', in: '{}', out: '{}', range: '{}', title: '{}'", line, in, out, range, title);
    std::vector<std::string> params = splitString(line, ' ');
    if (in.empty() && out.empty())
        return params;

    for (auto&& param : params) {
        auto inPos = param.find("%in");
        if (inPos != std::string::npos) {
            std::string newParam = param.replace(inPos, 3, in);
        }

        auto outPos = param.find("%out");
        if (outPos != std::string::npos) {
            std::string newParam = param.replace(outPos, 4, out);
        }

        auto rangePos = param.find("%range");
        if (rangePos != std::string::npos) {
            std::string newParam = param.replace(rangePos, 6, range);
        }

        auto titlePos = param.find("%title");
        if (titlePos != std::string::npos) {
            std::string newParam = param.replace(titlePos, 6, title);
        }
    }
    return params;
}

ssize_t getValidUTF8CutPosition(std::string_view str, ssize_t cutpos)
{
    ssize_t pos = -1;
    auto len = str.length();

    if ((len == 0) || (cutpos > static_cast<ssize_t>(len)))
        return pos;

#if 0
    printf("Character at cut position: %0x\n", char(str.at(cutpos)));
    printf("Character at cut-1 position: %0x\n", char(str.at(cutpos - 1)));
    printf("Character at cut-2 position: %0x\n", char(str.at(cutpos - 2)));
    printf("Character at cut-3 position: %0x\n", char(str.at(cutpos - 3)));
#endif

    // > 0x7f, we are dealing with a non-ascii character
    if (str.at(cutpos) & 0x80) {
        // check if we are at byte 2
        if (((cutpos - 1) >= 0) && (((str.at(cutpos - 1) & 0xc2) == 0xc2) || ((str.at(cutpos - 1) & 0xe2) == 0xe2) || ((str.at(cutpos - 1) & 0xf0) == 0xf0)))
            pos = cutpos - 1;
        // check if we are at byte 3
        else if (((cutpos - 2) >= 0) && (((str.at(cutpos - 2) & 0xe2) == 0xe2) || ((str.at(cutpos - 2) & 0xf0) == 0xf0)))
            pos = cutpos - 2;
        // we must be at byte 4 then...
        else if ((cutpos - 3) >= 0)
            pos = cutpos - 3;
    } else
        pos = cutpos;

    return pos;
}

std::string transliterate(const std::string& input)
{
#ifdef HAVE_ICU
    // Thanks to https://github.com/sangohan/vimix/blob/8a36a94e732df0bab0dd69891c93dd3d8e9db6ec/BaseToolkit.cpp#L82-L113
    // and https://unicode-org.github.io/icu/userguide/transforms/general/#icu-transliterators
    // icu::Transliterator is slow, we keep a cache of already
    // transliterated texts to be faster during repeated calls
    static std::map<std::string, std::string> translitCache;
    auto existingentry = translitCache.find(input);

    if (existingentry == translitCache.end()) {

        auto unicodeString = icu::UnicodeString::fromUTF8(input);

        UErrorCode status = U_ZERO_ERROR;
        for (auto&& translit : {
                 "any-NFKD ; [:Nonspacing Mark:] Remove; NFKC; Latin",
                 "any-NFKD ; [:Nonspacing Mark:] Remove; [@!#$*%~] Remove; NFKC",
             }) {
            icu::Transliterator* trans = icu::Transliterator::createInstance(
                translit, UTRANS_FORWARD, status);
            trans->transliterate(unicodeString);
            delete trans;
        }

        std::ostringstream result;
        result << unicodeString;

        // remember for future
        translitCache[input] = result.str();
    }

    // return remembered transliterated text
    return translitCache.at(input);
#else
    return input;
#endif
}
