/*GRB*

Gerbera - https://gerbera.io/

    url_utils.cc - this file is part of Gerbera.

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

/// \file url_utils.cc

#include "common.h"
#include "exceptions.h"
#include "tools.h"
#include <fmt/format.h>

#include "url_utils.h"

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

} // namespace URLUtils
