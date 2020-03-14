/*GRB*

    Gerbera - https://gerbera.io/

    http_protocol_helper.cc - this file is part of Gerbera.

    Copyright (C) 2016-2020 Gerbera Contributors

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

/// \file headers.cc

#include "headers.h" // API

#include <string>

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

void Headers::addHeader(const std::string& header, const std::string& value)
{
    if (headers == nullptr) {
        headers = std::make_unique<std::vector<std::pair<std::string, std::string>>>();
    }

    if (header.empty() || value.empty()) {
        return;
    }

    std::string cleanHeader = stripInvalid(header);
    std::string cleanValue = stripInvalid(value);
    // Only newlines
    if (cleanHeader.empty() || cleanValue.empty()) {
        return;
    }
    log_debug("Adding header: '{}: {}'", cleanHeader.c_str(), cleanValue.c_str());

    headers->push_back({ cleanHeader, cleanValue });
}

std::string Headers::formatHeader(const std::pair<std::string, std::string>& header, bool crlf)
{
    std::string headerValue;
    headerValue += header.first;
    headerValue += ": ";
    headerValue += header.second;
    if (crlf)
        headerValue += "\r\n";
    return headerValue;
}

void Headers::writeHeaders(UpnpFileInfo* fileInfo) const
{
#ifdef UPNP_1_12_LIST
    if (headers != nullptr) {
        auto head = const_cast<UpnpListHead*>(UpnpFileInfo_get_ExtraHeadersList(fileInfo));
        for (auto iter : *headers) {
            UpnpExtraHeaders* h = UpnpExtraHeaders_new();
            UpnpExtraHeaders_set_resp(h, formatHeader(iter, false).c_str());
            UpnpListInsert(head, UpnpListEnd(head), const_cast<UpnpListHead*>(UpnpExtraHeaders_get_node(h)));
        }
    }
#elif UPNP_HAS_EXTRA_HEADERS_LIST
    if (headers != nullptr) {
        auto head = const_cast<list_head*>(UpnpFileInfo_get_ExtraHeadersList(fileInfo));
        for (auto iter : *headers) {
            UpnpExtraHeaders* h = UpnpExtraHeaders_new();
            UpnpExtraHeaders_set_resp(h, formatHeader(iter, false).c_str());
            UpnpExtraHeaders_add_to_list_node(h, head);
        }
    }
#else
    std::string result;
    if (headers != nullptr) {
        // Reverse as map is sorted by insertion order latest first
        for (auto iter = headers->rbegin(); iter != headers->rend(); ++iter) {
            result += formatHeader(*iter, true);
        }
    }
    UpnpFileInfo_set_ExtraHeaders(fileInfo, ixmlCloneDOMString(result.c_str()));
#endif
}
