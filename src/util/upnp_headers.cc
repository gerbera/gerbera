/*GRB*

    Gerbera - https://gerbera.io/

    upnp_headers.cc - this file is part of Gerbera.

    Copyright (C) 2016-2023 Gerbera Contributors

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

/// \file upnp_headers.cc

#include "upnp_headers.h" // API

#if !defined(USING_NPUPNP)
#include <UpnpExtraHeaders.h>
#endif

#include "common.h"
#include "util/tools.h"

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

std::string Headers::formatHeader(const std::pair<std::string, std::string>& header)
{
    return fmt::format("{}: {}", header.first, header.second);
}

std::pair<std::string, std::string> Headers::parseHeader(const std::string& header)
{
    std::string first = header;
    std::string second;
    std::size_t found = header.find_first_of(':');
    if (found != std::string::npos) {
        first = header.substr(0, found);
        second = header.substr(found + 1);
    }

    trimStringInPlace(first);
    trimStringInPlace(second);
    return { first, second };
}

void Headers::writeHeaders(UpnpFileInfo* fileInfo) const
{
#if defined(USING_NPUPNP)
    std::copy(headers.begin(), headers.end(), std::back_inserter(fileInfo->response_headers));
#else
    auto head = const_cast<UpnpListHead*>(UpnpFileInfo_get_ExtraHeadersList(fileInfo));
    for (auto&& iter : headers) {
        UpnpExtraHeaders* h = UpnpExtraHeaders_new();
        UpnpExtraHeaders_set_resp(h, formatHeader(iter).c_str());
        UpnpListInsert(head, UpnpListEnd(head), const_cast<UpnpListHead*>(UpnpExtraHeaders_get_node(h)));
    }
#endif
}

std::map<std::string, std::string> Headers::readHeaders(UpnpFileInfo* fileInfo)
{
#if defined(USING_NPUPNP)
    return fileInfo->request_headers;
#else
    std::map<std::string, std::string> ret;

    auto head = const_cast<UpnpListHead*>(UpnpFileInfo_get_ExtraHeadersList(fileInfo));
    for (auto pos = UpnpListBegin(head); pos != UpnpListEnd(head); pos = UpnpListNext(head, pos)) {
        auto extra = reinterpret_cast<UpnpExtraHeaders*>(pos);
        std::string header = UpnpExtraHeaders_get_resp(extra);
        ret.insert(parseHeader(header));
    }

    return ret;
#endif
}
