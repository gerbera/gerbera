/*GRB*

    Gerbera - https://gerbera.io/

    upnp_headers.cc - this file is part of Gerbera.

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

/// \file upnp_headers.cc

#include "upnp_headers.h" // API

#include <string>

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

    if (headers == nullptr) {
        headers = std::make_unique<std::map<std::string, std::string>>();
    }

    log_debug("Adding header: '{}: {}'", cleanKey.c_str(), cleanValue.c_str());
    headers->insert(std::make_pair(cleanKey, cleanValue));
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
    return std::make_pair(first, second);
}

void Headers::writeHeaders(UpnpFileInfo* fileInfo) const
{
    if (headers == nullptr)
        return;

#ifdef UPNP_1_12_LIST
    auto head = const_cast<UpnpListHead*>(UpnpFileInfo_get_ExtraHeadersList(fileInfo));
    for (auto iter : *headers) {
        UpnpExtraHeaders* h = UpnpExtraHeaders_new();
        UpnpExtraHeaders_set_resp(h, formatHeader(iter, false).c_str());
        UpnpListInsert(head, UpnpListEnd(head), const_cast<UpnpListHead*>(UpnpExtraHeaders_get_node(h)));
    }
#elif UPNP_HAS_EXTRA_HEADERS_LIST
    auto head = const_cast<list_head*>(UpnpFileInfo_get_ExtraHeadersList(fileInfo));
    for (auto iter : *headers) {
        UpnpExtraHeaders* h = UpnpExtraHeaders_new();
        UpnpExtraHeaders_set_resp(h, formatHeader(iter, false).c_str());
        UpnpExtraHeaders_add_to_list_node(h, head);
    }
#else
    std::string result;
    for (auto iter : *headers) {
        result += formatHeader(*iter, true);
    }
    UpnpFileInfo_set_ExtraHeaders(fileInfo, ixmlCloneDOMString(result.c_str()));
#endif
}

std::unique_ptr<std::map<std::string, std::string>> Headers::readHeaders(UpnpFileInfo* fileInfo)
{
    auto ret = std::make_unique<std::map<std::string, std::string>>();

#ifdef UPNP_1_12_LIST
    auto head = const_cast<UpnpListHead*>(UpnpFileInfo_get_ExtraHeadersList(fileInfo));
    UpnpListIter pos;
    for (pos = UpnpListBegin(head); pos != UpnpListEnd(head); pos = UpnpListNext(head, pos)) {
        UpnpExtraHeaders* extra = (UpnpExtraHeaders*)pos;
        std::string header = UpnpExtraHeaders_get_resp(extra);
        auto add = parseHeader(header);
        ret->insert(add);
    }
#elif UPNP_HAS_EXTRA_HEADERS_LIST
    auto head = const_cast<list_head*>(UpnpFileInfo_get_ExtraHeadersList(fileInfo));
    list_head* pos;
    list_for_each(pos, head)
    {
        UpnpExtraHeaders* extra = (UpnpExtraHeaders*)pos;
        std::string header = UpnpExtraHeaders_get_resp(extra);
        auto add = parseHeader(header);
        ret->insert(add);
    }
#else
    // each header line should be followed by "\r\n"
    std::string str = UpnpFileInfo_get_ExtraHeaders_cstr();
    auto header_list = split_string(str, '\n');
    for (const auto& header : header_list) {
        auto add = parseHeader(header);
        ret->insert(add);
    }
#endif

    return ret;
}