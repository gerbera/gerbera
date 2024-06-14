/*GRB*

Gerbera - https://gerbera.io/

    URLUtil.h - this file is part of Gerbera.

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

/// \file URLUtil.h

#ifndef GERBERA_URL_UTILS_H
#define GERBERA_URL_UTILS_H

#include "common.h"

#include <map>
#include <string_view>
#include <utility>
#include <vector>

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
std::pair<std::string_view, std::string_view> splitUrl(std::string_view url, char separator);
std::string joinUrl(const std::vector<std::string>& components, bool addToEnd = false, std::string_view separator = URL_PARAM_SEPARATOR);
std::map<std::string, std::string> parseParameters(std::string_view filename, std::string_view baseLink);

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

} // namespace URLUtils

#endif // GERBERA_URL_UTILS_H
