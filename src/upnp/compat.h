/*GRB*

    Gerbera - https://gerbera.io/

    compat.h - this file is part of Gerbera.

    Copyright (C) 2024-2025 Gerbera Contributors

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

// \file compat.h
// \brief Haandle version compatibility

#ifndef GRB_UPNP_COMPAT_H__
#define GRB_UPNP_COMPAT_H__
#include <upnp.h>

#include <map>
#include <string>

#if defined(USING_NPUPNP)

#ifndef UPNP_VERSION
#define UPNP_VERSION (NPUPNP_VERSION_MAJOR * 10000 + NPUPNP_VERSION_MINOR * 100 + NPUPNP_VERSION_PATCH)
#endif

#define GrbUpnpFileInfoSetContentType(i, mt) (i)->content_type = std::move((mt))
#define GrbUpnpGetHeaders(i) (i)->request_headers
#define GrbUpnpSetHeaders(i, h) std::copy((h).begin(), (h).end(), std::back_inserter((i)->response_headers))

#define UPNP_NEEDS_LITERAL_HOST_REDIRECT
extern "C" void UpnpSetAllowLiteralHostRedirection(int);

#define UPNP_NEEDS_JOBS_TOTAL
extern "C" void UpnpSetMaxJobsTotal(int mjt);

#if (UPNP_VERSION <= 60103)
// new method added
#define UPNP_NEEDS_CORS
extern "C" int UpnpSetWebServerCorsString(const char*);
#endif

#else // USING_NPUPNP

#define UPNP_NEEDS_NOTIFY_XML
int UpnpNotifyXML(UpnpDevice_Handle handle, const char* devID, const char* servName, const std::string& propertySet);
#define UPNP_NEEDS_SUBSCRIPTION_XML
int UpnpAcceptSubscriptionXML(UpnpDevice_Handle handle, const char* devID, const char* servName, const std::string& propertySet, const std::string& subsId);
#define UPNP_NEEDS_GET_HEADERS
std::map<std::string, std::string> UpnpGetHeadersCompat(const UpnpFileInfo* fileInfo);
#define UPNP_NEEDS_SET_HEADERS
void UpnpSetHeadersCompat(const UpnpFileInfo* fileInfo, const std::map<std::string, std::string>& headers);

#define GrbUpnpFileInfoSetContentType(i, mt) UpnpFileInfo_set_ContentType((i), (mt).c_str())
#define GrbUpnpGetHeaders(i) UpnpGetHeadersCompat(i)
#define GrbUpnpSetHeaders(i, h) UpnpSetHeadersCompat(i, h)

#if (UPNP_VERSION <= 11419) or (UPNP_VERSION > 170000 and UPNP_VERSION <= 170110)
// new method added
#define UPNP_NEEDS_CORS
extern "C" int UpnpSetWebServerCorsString(const char*);

#define UPNP_NEEDS_JOBS_TOTAL
extern "C" void UpnpSetMaxJobsTotal(int mjt);
#endif

#endif

#define GrbUpnpNotify(handle, udn, serviceId, xml) UpnpNotifyXML(handle, (udn).c_str(), (serviceId).c_str(), xml)
#define GrbUpnpAcceptSubscription(handle, udn, serviceId, xml, subsId) UpnpAcceptSubscriptionXML(handle, (udn).c_str(), (serviceId).c_str(), xml, subsId)

#ifndef UPNP_USING_CHUNKED
#define UPNP_USING_CHUNKED (-1)
#endif

#endif // GRB_UPNP_COMPAT_H__
