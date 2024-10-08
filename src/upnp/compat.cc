/*GRB*

    Gerbera - https://gerbera.io/

    compat.cc - this file is part of Gerbera.

    Copyright (C) 2024 Gerbera Contributors

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

/// \file compat.cc

#include <cinttypes>

#include "compat.h"
#include "exceptions.h"
#include "upnp_common.h"
#include "util/tools.h"

#if !defined(USING_NPUPNP)
#include <UpnpExtraHeaders.h>
#endif

#if (UPNP_VERSION > 170000 && UPNP_VERSION <= 170110)
#define Stringize(L) #L
#define MakeString(M, L) M(L)
#define $Version MakeString(Stringize, (UPNP_VERSION))
#pragma message("Alpine upnp version " $Version " crooked")
#endif

#ifdef UPNP_NEEDS_LITERAL_HOST_REDIRECT
void UpnpSetAllowLiteralHostRedirection(int _)
{
}
#endif

#ifdef UPNP_NEEDS_CORS
int UpnpSetWebServerCorsString(const char* _)
{
    return 0;
}
#endif

#ifdef UPNP_NEEDS_JOBS_TOTAL
void UpnpSetMaxJobsTotal(int mjt)
{
}
#endif

#ifdef UPNP_NEEDS_NOTIFY_XML
int UpnpNotifyXML(UpnpDevice_Handle deviceHandle, const char* devID, const char* servName, const std::string& propertySet)
{
    IXML_Document* event = nullptr;
    int err = ixmlParseBufferEx(propertySet.c_str(), &event);
    if (err != IXML_SUCCESS) {
        /// \todo add another error code
        throw UpnpException(UPNP_E_SUBSCRIPTION_FAILED, "Could not convert property set to ixml");
    }

    UpnpNotifyExt(deviceHandle, devID, servName, event);

    ixmlDocument_free(event);
    return err;
}
#endif

#ifdef UPNP_NEEDS_SUBSCRIPTION_XML
int UpnpAcceptSubscriptionXML(UpnpDevice_Handle deviceHandle, const char* devID, const char* servName, const std::string& propertySet, const std::string& subsId)
{
    IXML_Document* event = nullptr;
    int err = ixmlParseBufferEx(propertySet.c_str(), &event);
    if (err != IXML_SUCCESS) {
        throw UpnpException(UPNP_E_SUBSCRIPTION_FAILED, "Could not convert property set to ixml");
    }

    UpnpAcceptSubscriptionExt(deviceHandle, devID, servName, event, subsId.c_str());

    ixmlDocument_free(event);
    return err;
}
#endif

#ifdef UPNP_NEEDS_GET_HEADERS
static std::pair<std::string, std::string> parseHeader(const std::string& header)
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

std::map<std::string, std::string> UpnpGetHeadersCompat(const UpnpFileInfo* fileInfo)
{
    std::map<std::string, std::string> ret;

    auto head = const_cast<UpnpListHead*>(UpnpFileInfo_get_ExtraHeadersList(fileInfo));
    for (auto pos = UpnpListBegin(head); pos != UpnpListEnd(head); pos = UpnpListNext(head, pos)) {
        auto extra = reinterpret_cast<UpnpExtraHeaders*>(pos);
        std::string header = UpnpExtraHeaders_get_resp(extra);
        ret.insert(parseHeader(header));
    }

    return ret;
}
#endif

#ifdef UPNP_NEEDS_SET_HEADERS
static std::string formatHeader(const std::pair<std::string, std::string>& header)
{
    return fmt::format("{}: {}", header.first, header.second);
}

void UpnpSetHeadersCompat(const UpnpFileInfo* fileInfo, const std::map<std::string, std::string>& headers)
{
    auto head = const_cast<UpnpListHead*>(UpnpFileInfo_get_ExtraHeadersList(fileInfo));
    for (auto&& iter : headers) {
        UpnpExtraHeaders* h = UpnpExtraHeaders_new();
        UpnpExtraHeaders_set_resp(h, formatHeader(iter).c_str());
        UpnpListInsert(head, UpnpListEnd(head), const_cast<UpnpListHead*>(UpnpExtraHeaders_get_node(h)));
    }
}
#endif
