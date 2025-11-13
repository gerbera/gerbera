/*GRB*

    Gerbera - https://gerbera.io/

    upnp/headers.h - this file is part of Gerbera.

    Copyright (C) 2016-2025 Gerbera Contributors

    Gerbera is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.

    Gerbera is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Gerbera.  If not, see <http://www.gnu.org/licenses/>.

    $Id$
*/

/// @file upnp/headers.h

#ifndef GERBERA_HEADERS_H
#define GERBERA_HEADERS_H

#include <map>
#include <string>
#include <upnp.h>

/// @brief Utility class for HTTP headers in UPnP requests
class Headers {
public:
    Headers(const UpnpFileInfo* fileInfo);
    Headers() = default;

    /// @brief update header value
    void updateHeader(const std::string& key, const std::string& value);
    /// @brief add header value
    void addHeader(const std::string& key, const std::string& value);
    /// @brief transfer headers to UPnP file info
    void writeHeaders(UpnpFileInfo* fileInfo) const;
    /// @brief Is header value set
    bool hasHeader(const std::string& key) const;
    /// @brief get header value
    std::string getHeader(const std::string& key) const;
    /// @brief get stored header values
    std::map<std::string, std::string> getHeaders() const { return headers; }

private:
    /// @brief strip end of line characters
    static std::string stripInvalid(const std::string& value);

    std::map<std::string, std::string> headers;
};

#endif // GERBERA_HEADERS_H
