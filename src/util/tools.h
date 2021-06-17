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

#include <filesystem>
#include <map>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>
namespace fs = std::filesystem;

#include <netinet/in.h>

#include "common.h"

// forward declaration
class Config;
class CdsItem;
class IOHandler;

/// \brief splits the given string into array of strings using a separator character.
/// \param str String to split
/// \param sep separator character
/// \param treat subsequent separators as empty array elements
/// \return array of strings
std::vector<std::string> splitString(std::string_view str, char sep, bool empty = false);

/// \brief trim from start (in place)
void leftTrimStringInPlace(std::string& str);

/// \brief trim from end (in place)
void rightTrimStringInPlace(std::string& str);

/// \brief remove leading and trailing whitespace (in place)
void trimStringInPlace(std::string& str);

/// \brief returns str with leading and trailing whitespace removed (copy)
std::string trimString(std::string str);

/// \brief returns true if str starts with check
bool startswith(std::string_view str, std::string_view check);

/// \brief returns lowercase of str
std::string toLower(std::string_view str);

/// \brief convert string to integer
int stoiString(const std::string& str, int def = 0, int base = 10);
/// \brief convert string to unsigned long
unsigned long stoulString(const std::string& str, int def = 0, int base = 10);

/// \brief  Used to replace potential multiple following //../ with single /
void reduceString(std::string& str, char ch);

/// \brief  Used to replace parts of string with other value
void replaceString(std::string& str, std::string_view from, std::string_view to);
void replaceAllString(std::string& str, std::string_view from, std::string_view to);

/// \brief Checks if the given file is a regular file (imitate same behaviour as std::filesystem::is_regular_file)
bool isRegularFile(const fs::path& path, std::error_code& ec) noexcept;
bool isRegularFile(const fs::directory_entry& dirEnt, std::error_code& ec) noexcept;

/// \brief Returns file size of give file, if it does not exist it will throw an exception
off_t getFileSize(const fs::directory_entry& dirEnt);

/// \brief Checks if the given binary is executable by our process
/// \param path absolute path of the binary
/// \param err if not NULL err will contain the errno result of the check
/// \return true if the given binary is executable by our process, otherwise false
bool isExecutable(const fs::path& path, int* err = nullptr);

/// \brief Checks if the given executable exists in $PATH
/// \param exec filename of the executable that needs to be checked
/// \return aboslute path to the given executable or nullptr of it was not found
fs::path findInPath(const fs::path& exec);

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
std::string hexEncode(const void* data, int len);

/// \brief Decodes hex encoded string.
/// \param encoded hex-encoded string.
/// \return decoded string
std::string hexDecodeString(std::string_view encoded);

/// \brief Generates random id.
/// \return String representing the newly created id.
std::string generateRandomId();

/// \brief Generates hex md5 sum of the given data.
std::string hexMd5(const void* data, int length);

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
void dictDecode(std::string_view url, std::map<std::string, std::string>* dict, bool unEscape = true);
void dictDecodeSimple(std::string_view url, std::map<std::string, std::string>* dict);

/// \brief Convert an array of strings to a CSV list, with additional protocol information
/// \param array that needs to be converted
/// \return string containing the CSV list
std::string mimeTypesToCsv(const std::vector<std::string>& mimeTypes);

/// \brief Reads the entire contents of a text file and returns it as a string.
std::string readTextFile(const fs::path& path);

/// \brief writes a string into a text file
void writeTextFile(const fs::path& path, std::string_view contents);

/// \brief Reads entire contents of a binary file into a buffer.
/// \return an empty optional if file can't be open and throws if read fails.
std::optional<std::vector<std::byte>> readBinaryFile(const fs::path& path);

/// \brief Writes data into a file. Throws if file can't be open or if write fails.
void writeBinaryFile(const fs::path& path, const std::byte* data, std::size_t size);

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
std::string getProtocol(const std::string& protocolInfo);

/// \brief Converts a number of milliseconds to "H*:MM:SS.F*" representation as required by the UPnP duration spec
std::string millisecondsToHMSF(int milliseconds);

/// \brief converts a "H*:MM:SS.F*" representation to milliseconds
int HMSFToMilliseconds(const std::string& time);

/// \brief Extracts resolution from a JPEG image
std::string get_jpeg_resolution(const std::unique_ptr<IOHandler>& ioh);

/// \brief checks if the given string has the format xr x yr (i.e. 320x200 etc.)
bool checkResolution(std::string_view resolution, int* x = nullptr, int* y = nullptr);

std::string escape(std::string string, char escape_char, char to_escape);

/// \brief Unescapes the given String.
/// \param string the string to unescape
/// \param escape the escape character (e.g. "\")
/// \return the unescaped string
std::string unescape(std::string string, char escape);

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

template <typename C, typename D>
std::string join(const C& container, const D& delimiter)
{
    std::ostringstream buf;
    auto it = container.begin();
    while (it != container.end()) {
        buf << *it;
        if (std::next(it++) != container.end())
            buf << delimiter;
    }
    return buf.str();
}

/// \brief Get value of map, iff not key is not in map return defval
template <typename K, typename V>
V getValueOrDefault(const std::map<K, V>& m, const K& key, const V& defval)
{
    auto it = m.find(key);
    return (it == m.end()) ? defval : it->second;
}
std::string getValueOrDefault(const std::map<std::string, std::string>& m, const std::string& key, const std::string& defval = "");

std::string toCSV(const std::shared_ptr<std::unordered_set<int>>& array);

std::chrono::seconds currentTime();
std::chrono::milliseconds currentTimeMS();

std::chrono::milliseconds getDeltaMillis(std::chrono::milliseconds ms);
std::chrono::milliseconds getDeltaMillis(std::chrono::milliseconds first, std::chrono::milliseconds second);

void getTimespecAfterMillis(std::chrono::milliseconds delta, std::chrono::milliseconds& ret);

/// \brief Finds the Interface with the specified IP address.
/// \param ip i.e. 192.168.4.56.
/// \return Interface name or nullptr if IP was not found.
std::string ipToInterface(std::string_view ip);

/// \brief Returns true if the given string is eitehr "yes" or "no", otherwise
/// returns false.
bool validateYesNo(std::string_view value);

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

/// \brief this is the mkstemp routine from glibc, the only difference is that
/// it does not return an fd but just the name that we could use.
///
/// The reason behind this is, that we need to open a pipe, while mkstemp will
/// open a regular file.
fs::path tempName(const fs::path& leadPath, char* tmpl);

/// \brief Determines if the particular ogg file contains a video (theora)
bool isTheora(const fs::path& ogg_filename);

/// \brief Gets an absolute filename as a parameter and returns the last parent
///
/// "/some/path/to/file.txt" -> "to"
fs::path getLastPath(const fs::path& path);

/// \brief Calculates a position where it is safe to cut an UTF-8 string.
/// \return Caclulated position or -1 in case of an error.
ssize_t getValidUTF8CutPosition(std::string_view str, ssize_t cutpos);

std::string getDLNATransferHeader([[maybe_unused]] const std::shared_ptr<Config>& config, std::string_view mimeType);
std::string getDLNAprofileString(std::string_view contentType);
std::string getDLNAContentHeader([[maybe_unused]] const std::shared_ptr<Config>& config, std::string_view contentType);

#ifndef HAVE_FFMPEG
/// \brief Fallback code to retrieve the used fourcc from an AVI file.
///
/// This code is based on offsets, so we will use it only if ffmpeg is not
/// available.
std::string getAVIFourCC(std::string_view avi_filename);
#endif

/// \brief Compare sockaddr
/// inspired by: http://www.opensource.apple.com/source/postfix/postfix-197/postfix/src/util/sock_addr.c
#define SOCK_ADDR_IN_PTR(sa) reinterpret_cast<const struct sockaddr_in*>(sa)
#define SOCK_ADDR_IN_ADDR(sa) SOCK_ADDR_IN_PTR(sa)->sin_addr
#define SOCK_ADDR_IN6_PTR(sa) reinterpret_cast<const struct sockaddr_in6*>(sa)
#define SOCK_ADDR_IN6_ADDR(sa) SOCK_ADDR_IN6_PTR(sa)->sin6_addr
std::string getHostName(const struct sockaddr* addr);
int sockAddrCmpAddr(const struct sockaddr* sa, const struct sockaddr* sb);
std::string sockAddrGetNameInfo(const struct sockaddr* sa);

#ifdef SOPCAST
/// \brief Finds a free port between range_min and range_max on localhost.
int find_local_port(in_port_t range_min, in_port_t range_max);
#endif

#endif // __TOOLS_H__
