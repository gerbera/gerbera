/*MT*

    MediaTomb - http://www.mediatomb.cc/

    tools.h - this file is part of MediaTomb.

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

/// @file util/tools.h
#ifndef __TOOLS_H__
#define __TOOLS_H__

#include <algorithm>
#include <cstdint>
#include <map>
#include <string>
#include <vector>

// forward declaration
class IOHandler;

// default protocol
constexpr auto PROTOCOL = "http-get";

/// @brief splits the given string into array of strings using a separator character.
/// @param str String to split
/// @param sep separator character
/// @param quote masking character encapsulating separators
/// @param empty treat subsequent separators as empty array elements
/// @return array of strings
std::vector<std::string> splitString(std::string_view str, char sep, char quote = '\0', bool empty = false);

/// @brief remove leading and trailing whitespace (in place)
void trimStringInPlace(std::string& str);

/// @brief returns str with leading and trailing whitespace removed (copy)
std::string trimString(std::string str);

/// @brief returns true if str starts with check
constexpr bool startswith(std::string_view str, std::string_view prefix)
{
#if __cpp_lib_starts_ends_with
    return str.starts_with(prefix);
#else
    return str.rfind(prefix, 0) == 0;
#endif
}

/// @brief returns true if str starts with check
constexpr bool endswith(std::string_view str, std::string_view suffix)
{
#if __cpp_lib_starts_ends_with
    return str.ends_with(suffix);
#else
    return str.size() >= suffix.size() && str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
#endif
}

/// @brief removes "-" and make following character uppercase
std::string camelCaseString(const std::string& str);

/// @brief returns uppercase of str
std::string toUpper(std::string str);

/// @brief returns lowercase of str
std::string toLower(std::string str);

/// @brief makes str lowercase in-place and returns a reference to it
std::string& toLowerInPlace(std::string& str);

/// @brief convert string to integer
std::int32_t stoiString(const std::string& str, std::int32_t def = 0, int base = 10);
std::int64_t stolString(const std::string& str, std::int64_t def = 0, int base = 10);
/// @brief convert string to unsigned long
unsigned long stoulString(const std::string& str, unsigned long def = 0, int base = 10);
/// @brief convert string to maximum width unsigned integer
uintmax_t stoumaxString(const std::string& str, uintmax_t def = 0, int base = 10);

/// @brief Used to replace potential multiple following //../ with single /
void reduceString(std::string& str, char ch);

/// @brief Used to replace parts of string with other value
void replaceString(std::string& str, std::string_view from, std::string_view to);
void replaceAllString(std::string& str, std::string_view from, std::string_view to);

/// @brief Used to expand number to fixed length for filesystem "natural" sorting
std::string expandNumbersString(std::string str, std::size_t size = 16);

/// @brief Render HTML that is doing a redirect to the given ip, port and html page.
/// @param addr IP address as string.
/// @param page HTML document to redirect to.
/// @return string representing the desired HTML document.
std::string httpRedirectTo(std::string_view addr, std::string_view page = "");

/// @brief Encodes arbitrary data to a hex string.
/// @param data Buffer that is holding the data
/// @param len Length of the buffer.
/// @return string of the data in hex representation.
std::string hexEncode(const void* data, std::size_t len);

/// @brief Decodes hex encoded string.
/// @param encoded hex-encoded string.
/// @return decoded string
std::string hexDecodeString(std::string_view encoded);

/// @brief Generates random id.
/// @return String representing the newly created id.
std::string generateRandomId();

/// @brief Generates hex md5 sum of the given data.
std::string hexMd5(const void* data, std::size_t length);

/// @brief Generates hex md5 sum of the given string.
std::string hexStringMd5(std::string_view str);

/// @brief Convert an array of strings to a CSV list, with additional protocol information
/// @param mimeTypes array that needs to be converted
/// @return string containing the CSV list
std::string mimeTypesToCsv(const std::vector<std::string>& mimeTypes);

/// @brief Renders a string that can be used as the protocolInfo resource
/// attribute: "http-get:*:mimetype:*"
///
/// @param mimetype the mimetype that should be inserted
/// @param protocol the protocol which should be inserted (default: "http-get")
/// @param extend extend the protocolInfo
/// @return The rendered protocolInfo String
std::string renderProtocolInfo(std::string_view mimetype, std::string_view protocol = PROTOCOL, std::string_view extend = "");

/// @brief Extracts mimetype from the protocol info string.
/// @param protocol info string as used in the protocolInfo attribute
/// @return mime-type in protocol
std::string getMTFromProtocolInfo(std::string_view protocol);

/// @brief Parses a protocolInfo string (see renderProtocolInfo).
///
/// @param protocolInfo the String from renderProtocolInfo.
/// @return Protocol (i.e. http-get).
std::string getProtocol(const std::string& protocolInfo);

/// @brief Adds escaping to special characters
///
/// @param string input text with characters to escape
/// @param escapeChar special character used for escaping, e.g., '\'
/// @param toEscape character in need for escaping, e.g., "
/// @return escaped string
std::string escape(std::string_view string, char escapeChar, char toEscape);

/// @brief Returns the first string if it isn't "nullptr", otherwise the fallback string.
/// @param first the string to return if it isn't nullptr
/// @param fallback fallback string to return if first is nullptr
/// @return return first if it isn't nullptr, otherwise fallback
std::string fallbackString(const std::string& first, const std::string& fallback);

/// @brief computes an (unsigned int) hash for the given string
/// @param str the string to compute the hash for
/// @return return the (unsigned int) hash value
unsigned int stringHash(std::string_view str);

/// @brief Get value of map, iff not key is not in map return defval
template <typename K, typename V>
V getValueOrDefault(const std::vector<std::pair<K, V>>& m, const K& key, const V& defval)
{
    static_assert(!std::is_const_v<V>);
    auto it = std::find_if(m.begin(), m.end(), [&](auto&& v) { return v.first == key; });
    return (it == m.end()) ? defval : it->second;
}

template <typename K, typename V>
V getValueOrDefault(const std::map<K, V>& m, const K& key, const V& defval)
{
    static_assert(!std::is_const_v<V>);
    auto it = m.find(key);
    return (it == m.end()) ? defval : it->second;
}

std::string getValueOrDefault(const std::vector<std::pair<std::string, std::string>>& m, const std::string& key, const std::string& defval = "");
std::string getValueOrDefault(const std::map<std::string, std::string>& m, const std::string& key, const std::string& defval = "");

/// @brief Parses a command line, splitting the arguments into an array and
/// substitutes %in and %out tokens with given strings.
///
/// This function splits a string into array parts, where space is used as the
/// separator. In addition special %in and %out tokens are replaced by given
/// strings.
/// @param line configured command with tokens
/// @param in replacement for %in
/// @param out replacement for %out
/// @param range replacement for %range
/// @param title replacement for %title
/// @return vector of strings containing command line items
std::vector<std::string> populateCommandLine(const std::string& line,
    const std::string& in = "",
    const std::string& out = "",
    const std::string& range = "",
    const std::string& title = "");

/// @brief Calculates a position where it is safe to cut an UTF-8 string.
/// @return Calculated position or -1 in case of an error.
ssize_t getValidUTF8CutPosition(std::string_view str, ssize_t cutpos);

/// @brief reduce size of string at length
std::string limitString(std::size_t stringLimit, const std::string& s);

/// @brief transliterate a string to ascii for display on restricted devices
/// @return transliterated string
std::string transliterate(const std::string& input);

#endif // __TOOLS_H__
