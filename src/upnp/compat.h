/*GRB*

    Gerbera - https://gerbera.io/

    compat.h - this file is part of Gerbera.

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

// \file compat.h
// \brief Haandle version compatibility

#ifndef GRB_UPNP_COMPAT_H__
#define GRB_UPNP_COMPAT_H__
#include <upnp.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(USING_NPUPNP)

#define UPNP_NEEDS_LITERAL_HOST_REDIRECT
void UpnpSetAllowLiteralHostRedirection(int);

#if (UPNP_VERSION <= 60102)
#define UPNP_NEEDS_CORS
int UpnpSetWebServerCorsString(const char*);
#endif

#else // USING_NPUPNP

#if (UPNP_VERSION <= 11419) or (UPNP_VERSION > 170000 and UPNP_VERSION <= 170110)

#define UPNP_NEEDS_CORS
int UpnpSetWebServerCorsString(const char*);
#endif

#endif

#ifdef __cplusplus
}
#endif
#endif // GRB_UPNP_COMPAT_H__
