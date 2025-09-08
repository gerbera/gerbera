/*GRB*

    Gerbera - https://gerbera.io/

    headers.h - this file is part of Gerbera.

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
#include <memory>
#include <upnp.h>
#include <vector>

class Headers {
public:
    void updateHeader(const std::string& key, const std::string& value);
    void addHeader(const std::string& key, const std::string& value);
    void writeHeaders(UpnpFileInfo* fileInfo) const;

    static std::map<std::string, std::string> readHeaders(const UpnpFileInfo* fileInfo);

private:
    static std::string stripInvalid(const std::string& value);

    std::map<std::string, std::string> headers;
};

#endif // GERBERA_HEADERS_H
