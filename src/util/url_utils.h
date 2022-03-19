/*GRB*

Gerbera - https://gerbera.io/

    URLUtil.h - this file is part of Gerbera.

    Copyright (C) 2022 Gerbera Contributors

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

#include <map>
#include <string_view>
#include <utility>
#include <vector>

#include "common.h"

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
std::string joinUrl(const std::vector<std::string>& components, bool addToEnd = false, std::string_view separator = _URL_PARAM_SEPARATOR);
std::map<std::string, std::string> parseParameters(std::string_view filename, std::string_view baseLink);
};

#endif // GERBERA_URL_UTILS_H
