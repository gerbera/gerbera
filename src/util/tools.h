/*MT*

    MediaTomb - http://www.mediatomb.cc/

    tools.h - this file is part of MediaTomb.

    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>

    Copyright (C) 2006-2010 Gena Batyan <bgeradz@mediatomb.cc>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>,
                            Leonhard Wimmer <leo@mediatomb.cc>

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

/// \file tools.h
#ifndef __TOOLS_H__
#define __TOOLS_H__

#include <chrono>
#include <map>
#include <optional>
#include <vector>

#include "common.h"

// forward declaration
class Config;
class IOHandler;

/// \brief splits the given string into array of strings using a separator character.
/// \param str String to split
/// \param sep separator character
/// \param treat subsequent separators as empty array elements
/// \return array of strings
std::vector<std::string> splitString(std::string_view str, char sep, bool empty = false);

/// \brief remove leading and trailing whitespace (in place)
void trimStringInPlace(std::string& str);

/// \brief returns str with leading and trailing whitespace removed (copy)
std::string trimString(std::string str);

/// \brief returns true if str starts with check
constexpr bool startswith(std::string_view str, std::string_view check)
{
#if __cpp_lib_starts_ends_with
    return str.starts_with(check);
#else
    return str.rfind(check, 0) == 0;
#endif
}

/// \brief returns uppercase of str
std::string toUpper(std::string str);

/// \brief returns lowercase of str
std::string toLower(std::string str);

/// \brief makes str lowercase in-place and returns a reference to it
std::string& toLowerInPlace(std::string& str);

/// \brief convert string to integer
int stoiString(const std::string& str, int def = 0, int base = 10);
/// \brief convert string to unsigned long
unsigned long stoulString(const std::string& str, int def = 0, int base = 10);

/// \brief  Used to replace potential multiple following //../ with single /
void reduceString(std::string& str, char ch);

/// \brief  Used to replace parts of string with other value
void replaceString(std::string& str, std::string_view from, std::string_view to);
void replaceAllString(std::string& str, std::string_view from, std::string_view to);

/// \brief Render browser friendly uri (handle IPv6)
std::string renderWebUri(std::string_view ip, int port);

/// \brief Render HTML that is doing a redirect to the given ip, port and html page.
/// \param ip IP address as string.
/// \param port Port as string.
/// \param page HTML document to redirect to.
/// \return string representing the desired HTML document.
std::string httpRedirectTo(std::string_view addr, std::string_view page = "");

/// \brief Encodes arbitrary data to a hex string.
/// \param data Buffer that is holding the data
/// \param len Length of the buffer.
/// \return string of the data in hex representation.
std::string hexEncode(const void* data, std::size_t len);

/// \brief Decodes hex encoded string.
/// \param encoded hex-encoded string.
/// \return decoded string
std::string hexDecodeString(std::string_view encoded);

/// \brief Generates random id.
/// \return String representing the newly created id.
std::string generateRandomId();

/// \brief Generates hex md5 sum of the given data.
std::string hexMd5(const void* data, std::size_t length);

/// \brief Generates hex md5 sum of the given string.
std::string hexStringMd5(std::string_view str);

/// \brief Converts a string to a URL (meaning: %20 instead of space and so on)
/// \param str String to be converted.
/// \return string that contains the url-escaped representation of the original string.
std::string urlEscape(std::string_view str);

/// \brief Opposite of urlEscape :)
std::string urlUnescape(std::string_view str);

std::string dictEncode(const std::map<std::string, std::string>& dict);
std::string dictEncodeSimple(const std::map<std::string, std::string>& dict);
std::map<std::string, std::string> dictDecode(std::string_view url, bool unEscape = true);
std::map<std::string, std::string> pathToMap(std::string_view url);

/// \brief Convert an array of strings to a CSV list, with additional protocol information
/// \param array that needs to be converted
/// \return string containing the CSV list
std::string mimeTypesToCsv(const std::vector<std::string>& mimeTypes);

/// \brief Renders a string that can be used as the protocolInfo resource
/// attribute: "http-get:*:mimetype:*"
///
/// \param mimetype the mimetype that should be inserted
/// \param protocol the protocol which should be inserted (default: "http-get")
/// \return The rendered protocolInfo String
std::string renderProtocolInfo(std::string_view mimetype, std::string_view protocol = PROTOCOL, std::string_view extend = "");

/// \brief Extracts mimetype from the protocol info string.
/// \param protocol info string as used in the protocolInfo attribute
std::string getMTFromProtocolInfo(std::string_view protocol);

/// \brief Parses a protocolInfo string (see renderProtocolInfo).
///
/// \param protocolInfoStr the String from renderProtocolInfo.
/// \return Protocol (i.e. http-get).
std::string_view getProtocol(std::string_view protocolInfo);

template <typename TP>
std::chrono::seconds to_seconds(TP tp)
{
    auto asSystemTime = std::chrono::time_point_cast<std::chrono::system_clock::duration>(tp - TP::clock::now()
        + std::chrono::system_clock::now());
    return std::chrono::duration_cast<std::chrono::seconds>(asSystemTime.time_since_epoch());
}

/// \brief Converts a number of milliseconds to "H*:MM:SS.F*" representation as required by the UPnP duration spec
std::string millisecondsToHMSF(int milliseconds);

/// \brief converts a "H*:MM:SS.F*" representation to milliseconds
int HMSFToMilliseconds(std::string_view time);

/// \brief Extracts resolution from a JPEG image
/// \param ioh the IOHandler must be opened. The function will read data and close the handler.
std::string get_jpeg_resolution(std::unique_ptr<IOHandler> ioh);

/// \brief checks if the given string has the format xr x yr (i.e. 320x200 etc.)
std::pair<unsigned int, unsigned int> checkResolution(std::string_view resolution);

std::string escape(std::string_view string, char escapeChar, char toEscape);

/*
/// \brief Unescape &amp; &quot; and similar XML sequences.
///
/// This function will silently ignore and pass on any invalid combinations.
/// \param string that should be unescaped
/// \return the unescaped string
std::string xml_unescape(std::string string);
*/

/// \brief Returns the first string if it isn't "nullptr", otherwise the fallback string.
/// \param first the string to return if it isn't nullptr
/// \param fallback fallback string to return if first is nullptr
/// \return return first if it isn't nullptr, otherwise fallback
std::string fallbackString(const std::string& first, const std::string& fallback);

/// \brief computes an (unsigned int) hash for the given string
/// \param str the string to compute the hash for
/// \return return the (unsigned int) hash value
unsigned int stringHash(std::string_view str);

/// \brief Iterator over values of a sequential enum between begin and end
template <typename C, C beginVal, C endVal>
class EnumIterator {
private:
    using val_t = std::underlying_type_t<C>;
    val_t val;

public:
    explicit EnumIterator(const C& f)
        : val(static_cast<val_t>(f))
    {
    }
    EnumIterator()
        : val(static_cast<val_t>(beginVal))
    {
    }

    EnumIterator operator++()
    {
        ++val;
        return *this;
    }

    C operator*() { return static_cast<C>(val); }

    EnumIterator begin() const { return *this; }
    EnumIterator end() const
    {
        static const EnumIterator endIter = EnumIterator(endVal);
        return endIter;
    }
    bool operator!=(const EnumIterator& i) const
    {
        return val != i.val;
    }
};

/// \brief Get value of map, iff not key is not in map return defval
template <typename K, typename V>
V getValueOrDefault(const std::vector<std::pair<K, V>>& m, const K& key, const V& defval)
{
    auto it = std::find_if(m.begin(), m.end(), [&](auto&& v) { return v.first == key; });
    return (it == m.end()) ? defval : it->second;
}

template <typename K, typename V>
V getValueOrDefault(const std::map<K, V>& m, const K& key, const V& defval)
{
    auto it = m.find(key);
    return (it == m.end()) ? defval : it->second;
}
std::string getValueOrDefault(const std::vector<std::pair<std::string, std::string>>& m, const std::string& key, const std::string& defval = "");
std::string getValueOrDefault(const std::map<std::string, std::string>& m, const std::string& key, const std::string& defval = "");

std::string getPropertyMapValue(const std::map<std::string, std::string>& propMap, const std::string& search);

std::chrono::seconds currentTime();
std::chrono::milliseconds currentTimeMS();

std::chrono::milliseconds getDeltaMillis(std::chrono::milliseconds ms);
std::chrono::milliseconds getDeltaMillis(std::chrono::milliseconds first, std::chrono::milliseconds second);

/// \brief Parses a command line, splitting the arguments into an array and
/// substitutes %in and %out tokens with given strings.
///
/// This function splits a string into array parts, where space is used as the
/// separator. In addition special %in and %out tokens are replaced by given
/// strings.
/// \todo add escaping
std::vector<std::string> populateCommandLine(const std::string& line,
    const std::string& in = "",
    const std::string& out = "",
    const std::string& range = "",
    const std::string& title = "");

/// \brief Calculates a position where it is safe to cut an UTF-8 string.
/// \return Caclulated position or -1 in case of an error.
ssize_t getValidUTF8CutPosition(std::string_view str, ssize_t cutpos);

#endif // __TOOLS_H__
