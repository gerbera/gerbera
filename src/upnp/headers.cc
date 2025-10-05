/*GRB*

    Gerbera - https://gerbera.io/

    headers.cc - this file is part of Gerbera.

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

/// @file upnp/headers.cc
#define GRB_LOG_FAC GrbLogFacility::clients

#include "headers.h" // API

#include "upnp/compat.h"
#include "util/logger.h"

Headers::Headers(const UpnpFileInfo* fileInfo)
    : headers(GrbUpnpGetHeaders(fileInfo))
{
}

std::string Headers::stripInvalid(const std::string& value)
{
    std::string result = value;
    std::size_t found = value.find_first_of('\r');
    if (found != std::string::npos) {
        result = value.substr(0, found);
    }
    found = result.find_first_of('\n');
    if (found != std::string::npos) {
        result = value.substr(0, found);
    }
    return result;
}

void Headers::addHeader(const std::string& key, const std::string& value)
{
    if (key.empty() || value.empty()) {
        return;
    }

    std::string cleanKey = stripInvalid(key);
    std::string cleanValue = stripInvalid(value);
    if (cleanKey.empty() || cleanValue.empty()) {
        return;
    }

    log_debug("Adding header: '{}: {}'", cleanKey, cleanValue);
    headers.try_emplace(cleanKey, cleanValue);
}

void Headers::updateHeader(const std::string& key, const std::string& value)
{
    if (key.empty() || value.empty()) {
        return;
    }

    std::string cleanKey = stripInvalid(key);
    std::string cleanValue = stripInvalid(value);
    if (cleanKey.empty() || cleanValue.empty()) {
        return;
    }

    log_debug("Adding header: '{}: {}'", cleanKey, cleanValue);
    headers[cleanKey] = cleanValue;
}

void Headers::writeHeaders(UpnpFileInfo* fileInfo) const
{
    GrbUpnpSetHeaders(fileInfo, headers);
}

bool Headers::hasHeader(const std::string& key) const
{
    return headers.find(key) != headers.end();
}

std::string Headers::getHeader(const std::string& key) const
{
    return headers.find(key)->second;
}
