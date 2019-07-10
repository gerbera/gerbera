/*GRB*

Gerbera - https://gerbera.io/

    http_protocol_helper.cc - this file is part of Gerbera.

    Copyright (C) 2016-2019 Gerbera Contributors

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

/// \file http_protocol_helper.cc

#include "headers.h"
#include <string>
#include <tools.h>

void Headers::addHeader(zmm::String header)
{
    if (string_ok(header))
        addHeader(std::string(header.c_str()));
}

void Headers::addHeader(const std::string& header)
{
    if (headers == nullptr) {
        headers = std::make_unique<std::vector<std::string>>();
    }

    if (header.empty()) {
        return;
    }

    std::string result = header;
    std::size_t found = header.find_first_of('\r');
    if (found != std::string::npos) {
        result = header.substr(0, found);
    }
    found = result.find_first_of('\n');
    if (found != std::string::npos) {
        result = header.substr(0, found);
    }
    // Only newlines
    if (result.size() == 0) {
        return;
    }
    log_debug("Adding header: '%s'\n", header.c_str());

    headers->push_back(result);
}

const void Headers::writeHeaders(UpnpFileInfo *fileInfo)
{

    std::string result;
    if (headers != nullptr) {
        for (const std::string& header : *headers) {
            result += header;
            result += "\r\n";
        }
    }

    log_debug("Generated Headers: %s\n", result.c_str());
    UpnpFileInfo_set_ExtraHeaders(fileInfo, ixmlCloneDOMString(result.c_str()));
}

