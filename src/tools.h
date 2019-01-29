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

#include <memory>
#include <sstream>
#include <unordered_set>

#include <sys/time.h>

#include "cds_objects.h"
#include "common.h"
#include "io_handler.h"
#include "rexp.h"

#ifdef HAVE_MAGIC
// for older versions of filemagic
extern "C" {
#include <magic.h>
}

#endif

/// \brief splits the given string into array of strings using a separator character.
/// \param str String to split
/// \param sep separator character
/// \param treat subsequent separators as empty array elements
/// \return array of strings
zmm::Ref<zmm::Array<zmm::StringBase>> split_string(zmm::String str, char sep,
    bool empty = false);

/// \brief splits the given file path into the path to the file and the filename.
/// \param str the path to split
/// \return an array with the path at position 0 and the filename at position 1.
zmm::Ref<zmm::Array<zmm::StringBase>> split_path(zmm::String str);

/// \brief returns str with leading and trailing whitespace removed
zmm::String trim_string(zmm::String str);

/// \brief Checks existance of the specified file or path.
/// \param path file or directory to be checked.
/// \param needDir true when checked item has to be a directory.
/// \return true path or file exists
/// \return false path of file not found
bool check_path(zmm::String path, bool needDir = false);

/// \brief Checks existance of the specified file or path.
/// \param path file or directory to be checked.
/// \param needDir true when checked item has to be a directory.
/// \param existenceUnneeded do not throw exception if file was not found
/// \param filesize returns the size of the file
/// \return last modification time of the path or directory
///
/// More or less the same as check_path, the only difference is,
/// that this function throws an exception if a path or directory
/// was not found or was not the desired type. Additionally this function
/// returns the last modification time of the file or directory and, if
/// needed, also the filesize.
time_t check_path_ex(zmm::String path, bool needDir = false, bool existenceUnneeded = false, off_t* filesize = NULL);

/// \brief Checks if the given binary is executable by our process
/// \param path absolute path of the binary
/// \param err if not NULL err will contain the errno result of the check
/// \return true if the given binary is executable by our process, otherwise
/// false
bool is_executable(zmm::String path, int* err = NULL);

/// \brief Checks if the given executable exists in $PATH
/// \param exec filename of the executable that needs to be checked
/// \return aboslute path to the given executable or nullptr of it was not found
zmm::String find_in_path(zmm::String exec);

/// \brief Checks if the string contains any data.
/// \param str String to be checked.
/// \return true if ok
/// \return false if string was either nullptr or empty
///
/// Checks if str is nullptr or ""
bool string_ok(zmm::String str);

/// \brief Checks if the string contains any data.
/// \param str String to be checked.
///
/// Checks if str is nullptr or "" and throws an exception if that is the case.
void string_ok_ex(zmm::String str);

/// \brief Render HTML that is doing a redirect to the given ip, port and html page.
/// \param ip IP address as string.
/// \param port Port as string.
/// \oaram page HTML document to redirect to.
/// \return string representing the desired HTML document.
zmm::String http_redirect_to(zmm::String ip, zmm::String port, zmm::String page = _(""));

/// \brief Encodes arbitrary data to a hex string.
/// \param data Buffer that is holding the data
/// \param len Length of the buffer.
/// \return string of the data in hex representation.
zmm::String hex_encode(const void* data, int len);

/// \brief Decodes hex encoded string.
/// \param encoded hex-encoded string.
/// \return decoded string
zmm::String hex_decode_string(zmm::String encoded);

/// \brief Generates random id.
/// \return String representing the newly created id.
zmm::String generate_random_id();

/// \brief Generates hex md5 sum of the given data.
zmm::String hex_md5(const void* data, int length);

/// \brief Generates hex md5 sum of the given string.
zmm::String hex_string_md5(zmm::String str);

/// \brief Converts a string to a URL (meaning: %20 instead of space and so on)
/// \param str String to be converted.
/// \return string that contains the url-escaped representation of the original string.
zmm::String url_escape(zmm::String str);

/// \brief Opposite of url_escape :)
zmm::String urlUnescape(zmm::String str);

/// \brief Convert an array of strings to a CSV list, with additional protocol information
/// \param array that needs to be converted
/// \return string containing the CSV list
zmm::String mime_types_to_CSV(zmm::Ref<zmm::Array<zmm::StringBase>> mimeTypes);

/// \brief a wrapper for the reentrant strerror_r() function
/// \param mt_errno the errno to get the error string from
/// \return the error string
zmm::String mt_strerror(int mt_errno);

/// \brief Reads the entire contents of a text file and returns it as a string.
zmm::String read_text_file(zmm::String path);

/// \brief writes a string into a text file
void write_text_file(zmm::String path, zmm::String contents);

/// \brief copies a file
/// \param from the path to the file to copy from
/// \param to the path to the file to copy to
void copy_file(zmm::String from, zmm::String to);

typedef int (*COMPARATOR)(void*, void*);
typedef void* COMPARABLE;

int StringBaseComparatorAsc(void*, void*);

void quicksort(COMPARABLE* arr, int size, COMPARATOR comparator);

/// \brief Renders a string that can be used as the protocolInfo resource
/// attribute: "http-get:*:mimetype:*"
///
/// \param mimetype the mimetype that should be inserted
/// \param protocol the protocol which should be inserted (default: "http-get")
/// \return The rendered protocolInfo String
zmm::String renderProtocolInfo(zmm::String mimetype, zmm::String protocol = _(PROTOCOL), zmm::String extend = nullptr);

/// \brief Extracts mimetype from the protocol info string.
/// \param protocol info string as used in the protocolInfo attribute
zmm::String getMTFromProtocolInfo(zmm::String protocol);

/// \brief Parses a protocolInfo string (see renderProtocolInfo).
///
/// \param protocolInfoStr the String from renderProtocolInfo.
/// \return Protocol (i.e. http-get).
zmm::String getProtocol(zmm::String protocolInfo);

/// \brief Converts a number of seconds to H+:MM:SS representation as required by
/// the UPnP spec
zmm::String secondsToHMS(int seconds);

/// \brief converts a "H:MM:SS:" representation to seconds
int HMSToSeconds(zmm::String time);

#ifdef HAVE_MAGIC
/// \brief Extracts mimetype from a file using filemagic
zmm::String getMIMETypeFromFile(zmm::String file);
/// \brief Extracts mimetype from a buffer using filemagic
zmm::String getMIMETypeFromBuffer(const void *buffer, size_t length);
/// \brief Extracts mimetype from a filepath OR buffer using filemagic
zmm::String getMIME(zmm::String filepath, const void *buffer, size_t length);

#endif // HAVE_MAGIC

#ifdef SOPCAST
/// \brief Finds a free port between range_min and range_max on localhost.
int find_local_port(unsigned short range_min,
    unsigned short range_max);
#endif
/// \brief Extracts resolution from a JPEG image
zmm::String get_jpeg_resolution(zmm::Ref<IOHandler> ioh);

/// \brief Sets resolution for a given resource index, item must be a JPEG image
void set_jpeg_resolution_resource(zmm::Ref<CdsItem> item, int res_num);

/// \brief checks if the given string has the format xr x yr (i.e. 320x200 etc.)
bool check_resolution(zmm::String resolution, int* x = NULL, int* y = NULL);

zmm::String escape(zmm::String string, char escape_char, char to_escape);

/// \brief Unescapes the given String.
/// \param string the string to unescape
/// \param escape the escape character (e.g. "\")
/// \return the unescaped string
zmm::String unescape(zmm::String string, char escape);

/*
/// \brief Unescape &amp; &quot; and similar XML sequences.
///
/// This function will silently ignore and pass on any invalid combinations.
/// \param string that should be unescaped
/// \return the unescaped string
zmm::String xml_unescape(zmm::String string);
*/

/// brief Unescape all &amp; occurances in a given string.
///
/// This function will look for &amp; in a string and convert it to '&',
/// all other tokens are not touched.
/// \param string that should be unescaped
/// \return the unescaped string
zmm::String unescape_amp(zmm::String string);

/// \brief Returns the first string if it isn't "nullptr", otherwise the fallback string.
/// \param first the string to return if it isn't nullptr
/// \param fallback fallback string to return if first is nullptr
/// \return return first if it isn't nullptr, otherwise fallback
zmm::String fallbackString(zmm::String first, zmm::String fallback);

/// \brief computes an (unsigned int) hash for the given string
/// \param str the string to compute the hash for
/// \return return the (unsigned int) hash value
unsigned int stringHash(zmm::String str);

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

zmm::String toCSV(std::shared_ptr<std::unordered_set<int>> array);

//inline void getTimeval(struct timeval *now) { gettimeofday(now, NULL); }

void getTimespecNow(struct timespec* ts);

long getDeltaMillis(struct timespec* first);
long getDeltaMillis(struct timespec* first, struct timespec* second);

void getTimespecAfterMillis(long delta, struct timespec* ret, struct timespec* start = NULL);

/// \brief This function makes sure that there are no trailing slashes, no
/// consecutive slashes. If /../ or /..\0 is encountered an exception is
/// thrown.
zmm::String normalizePath(zmm::String path);

/// \brief Finds the IP address of the specified network interface.
/// \param interface i.e. eth0, lo, etc.
/// \return IP address or nullptr if interface was not found.
zmm::String interfaceToIP(zmm::String interface);

/// \brief Finds the Interface with the specified IP address.
/// \param ip i.e. 192.168.4.56.
/// \return Interface name or nullptr if IP was not found.
zmm::String ipToInterface(zmm::String interface);

/// \brief Returns true if the given string is eitehr "yes" or "no", otherwise
/// returns false.
bool validateYesNo(zmm::String value);

/// \brief Parses a command line, splitting the arguments into an array and
/// substitutes %in and %out tokens with given strings.
///
/// This function splits a string into array parts, where space is used as the
/// separator. In addition special %in and %out tokens are replaced by given
/// strings.
/// \todo add escaping
zmm::Ref<zmm::Array<zmm::StringBase>> parseCommandLine(zmm::String line,
    zmm::String in,
    zmm::String out,
    zmm::String range);

/// \brief this is the mkstemp routine from glibc, the only difference is that
/// it does not return an fd but just the name that we could use.
///
/// The reason behind this is, that we need to open a pipe, while mkstemp will
/// open a regular file.
zmm::String tempName(zmm::String leadPath, char* tmpl);

/// \brief Determines if the particular ogg file contains a video (theora)
bool isTheora(zmm::String ogg_filename);

/// \brief Gets an absolute filename as a parameter and returns the last parent
///
/// "/some/path/to/file.txt" -> "to"
zmm::String get_last_path(zmm::String location);

/// \brief Calculates a position where it is safe to cut an UTF-8 string.
/// \return Caclulated position or -1 in case of an error.
ssize_t getValidUTF8CutPosition(zmm::String str, ssize_t cutpos);

zmm::String getDLNAtransferHeader(zmm::String mimeType);
zmm::String getDLNAprofileString(zmm::String contentType);
zmm::String getDLNAcontentHeader(zmm::String contentType);

#ifndef HAVE_FFMPEG
/// \brief Fallback code to retrieve the used fourcc from an AVI file.
///
/// This code is based on offsets, so we will use it only if ffmpeg is not
/// available.
zmm::String getAVIFourCC(zmm::String avi_filename);
#endif

#ifdef TOMBDEBUG

struct profiling_t {
    struct timespec sum;
    struct timespec last_start;
    bool running;
    pthread_t thread;
};

#define PROFILING_T_INIT                          \
    {                                             \
        { 0, 0 }, { 0, 0 }, false, pthread_self() \
    }

void profiling_start(struct profiling_t* data);
void profiling_end(struct profiling_t* data);
void profiling_print(struct profiling_t* data);

#define PROF_INIT(var) profiling_t var = PROFILING_T_INIT
#define PROF_START(var) profiling_start(var)
#define PROF_END(var) profiling_end(var)
#define PROF_PRINT(var) profiling_print(var)

#else

#define profiling_t int
#define PROF_INIT(var) profiling_t var
#define PROF_START(var)
#define PROF_END(var)
#define PROF_PRINT(var)

/*
#define profiling_start()
#define profiling_end()
#define profiling_get_time()
*/

#endif

#endif // __TOOLS_H__
