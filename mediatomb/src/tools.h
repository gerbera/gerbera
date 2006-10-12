/*  tools.h - this file is part of MediaTomb.

    Copyright (C) 2005 Gena Batyan <bgeradz@deadlock.dhs.org>,
                       Sergey Bostandzhyan <jin@deadlock.dhs.org>

    MediaTomb is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    MediaTomb is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with MediaTomb; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/


/// \file tools.h
#ifndef __TOOLS_H__
#define __TOOLS_H__

#include "common.h"
#include "rexp.h"
#include "io_handler.h"
#include "cds_objects.h"

#ifdef HAVE_MAGIC
// for older versions of filemagic
extern "C" {
#include <magic.h>
}

#endif

/// \brief splits the given string into array of strings using a separator character.
/// \param sep separator character
/// \return array of strings
zmm::Ref<zmm::Array<zmm::StringBase> > split_string(zmm::String str, char sep);

/// \brief splits the given file path into the path to the file and the filename.
/// \param str the path to split
/// \return an array with the path at position 0 and the filename at position 1.
zmm::Ref<zmm::Array<zmm::StringBase> > split_path(zmm::String str);

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
///
/// More or less the same as check_path, the only difference is,
/// that this function throws an exception if a path or directory
/// was not found or was not the desired type.
void check_path_ex(zmm::String path, bool needDir = false);
    
/// \brief Checks if the string contains any data.
/// \param str String to be checked.
/// \return true if ok
/// \return false if string was either nil or empty
/// 
/// Checks if str is nil or ""
bool string_ok(zmm::String str);

/// \brief Checks if the string contains any data.
/// \param str String to be checked.
/// 
/// Checks if str is nil or "" and throws an exception if that is the case.
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
zmm::String hex_encode(void *data, int len);

/// \brief Decodes hex encoded string.
/// \param encoded hex-encoded string.
/// \return decoded string
zmm::String hex_decode_string(zmm::String encoded);


/// \brief Generates random id.
/// \return String representing the newly created id.
zmm::String generate_random_id();

/// \brief Generates hex md5 sum of the given data.
zmm::String hex_md5(void *data, int length);

/// \brief Generates hex md5 sum of the given string.
zmm::String hex_string_md5(zmm::String str);

/// \brief Converts a string to a URL (meaning: %20 instead of space and so on)
/// \param str String to be converted.
/// \return string that contains the url-escaped representation of the original string.
zmm::String url_escape(zmm::String str);
 
/// \brief Opposite of url_escape :) 
zmm::String url_unescape(zmm::String str);

/// \brief Convert an array of strings to a CSV list, with additional protocol information
/// \param array that needs to be converted
/// \return string containing the CSV list
zmm::String mime_types_to_CSV(zmm::Ref<zmm::Array<zmm::StringBase> > mimeTypes);

/// \brief Reads the entire contents of a text file and returns it as a string.
zmm::String read_text_file(zmm::String path);

/// \brief writes a string into a text file
void write_text_file(zmm::String path, zmm::String contents);

typedef int (*COMPARATOR) (void *, void *);
typedef void * COMPARABLE;

int StringBaseComparatorAsc(void *, void *);

void quicksort(COMPARABLE *arr, int size, COMPARATOR comparator);

/// \brief Renders a string that can be used as the protocolInfo resource 
/// attribute: "http-get:*:mimetype:*"
/// 
/// \param mimetype the mimetype that should be inserted
/// \param protocol the protocol which should be inserted (default: "http-get")
/// \return The rendered protocolInfo String
zmm::String renderProtocolInfo(zmm::String mimetype, zmm::String protocol = _(PROTOCOL));

/// \brief Parses a protocolInfo string (see renderProtocolInfo).
/// 
/// \param protocolInfoStr the String from renderProtocolInfo.
/// \return Protocol (i.e. http-get).
zmm::String getProtocol(zmm::String protocolInfo);


/// \brief Converts a number of seconds to H+:MM:SS representation as required by
/// the UPnP spec
zmm::String secondsToHMS(int seconds);

#ifdef HAVE_MAGIC
/// \brief Extracts mimetype from a file using filemagic
zmm::String get_mime_type(magic_set *ms, zmm::Ref<RExp> reMimetype, zmm::String file);
#endif // HAVE_MAGIC

/// \brief Extracts resolution from a JPEG image
zmm::String get_jpeg_resolution(zmm::Ref<IOHandler> ioh);

/// \brief Sets resolution for a given resource index, item must be a JPEG image
void set_jpeg_resolution_resource(zmm::Ref<CdsItem> item, int res_num);

/// \brief Unescapes the given String.
/// \param string the string to unescape
/// \param escape the escape character (e.g. "\")
/// \return the unescaped string
zmm::String unescape(zmm::String string, char escape);

/// \brief Returns the first string if it isn't "nil", otherwise the fallback string.
/// \param first the string to return if it isn't nil
/// \param fallback fallback string to return if first is nil
/// \return return first if it isn't nil, otherwise fallback
zmm::String fallbackString(zmm::String first, zmm::String fallback);

/// \brief computes an (unsigned int) hash for the given string
/// \param str the string to compute the hash for
/// \return return the (unsigned int) hash value
unsigned int stringHash(zmm::String str);

#endif // __TOOLS_H__

