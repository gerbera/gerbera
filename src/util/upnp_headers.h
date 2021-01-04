/*GRB*

    Gerbera - https://gerbera.io/

    upnp_headers.h - this file is part of Gerbera.

    Copyright (C) 2016-2021 Gerbera Contributors

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

/// \file upnp_headers.h

#ifndef GERBERA_HEADERS_H
#define GERBERA_HEADERS_H

#include <map>
#include <memory>
#include <upnp.h>
#include <vector>

class Headers {
public:
    void addHeader(const std::string& key, const std::string& value);
    void writeHeaders(UpnpFileInfo* fileInfo) const;

    static std::unique_ptr<std::map<std::string, std::string>> readHeaders(UpnpFileInfo* fileInfo);

private:
    static std::string formatHeader(const std::pair<std::string, std::string>& header, bool crlf);
    static std::pair<std::string, std::string> parseHeader(const std::string& header);
    static std::string stripInvalid(const std::string& value);

    std::unique_ptr<std::map<std::string, std::string>> headers;
};

#endif //GERBERA_HEADERS_H
