/*GRB*

Gerbera - https://gerbera.io/

    upnp_compat.h - this file is part of Gerbera.

    Copyright (C) 2021 Gerbera Contributors

    Gerbera is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.

    Gerbera is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Gerbera.  If not, see <http://www.gnu.org/licenses/>.
*/

/// \file upnp_compat.h

#ifndef GERBERA_UPNP_COMPAT_H
#define GERBERA_UPNP_COMPAT_H

#include <upnp.h>
#include <upnpconfig.h>

#if UPNP_VERSION_MAJOR >= 1 && UPNP_VERSION_MINOR >= 16
#define HAS_UPNP_LIB 1
#define UPNP_LIB_PARAM UpnpLib *upnpLib,
#define UPNP_LIB_ARG upnpLib,
#define UPNP_LIB_ARG_SINGLE upnpLib
#define UPNP_LIB_MEMBER UpnpLib* upnpLib = nullptr;
#define UPNP_LIB_INIT_LIST , upnpLib(upnpLib)
#define UPNP_FUNPTR_COOKIE_CONST const

#define UpnpExtraHeaders UpnpHttpHeaders
#define UpnpExtraHeaders_get_node UpnpHttpHeaders_get_node
#define UpnpExtraHeaders_get_resp UpnpHttpHeaders_get_resp
#define UpnpExtraHeaders_new UpnpHttpHeaders_new
#define UpnpExtraHeaders_set_resp UpnpHttpHeaders_set_resp
#define UpnpFileInfo_get_ExtraHeadersList UpnpFileInfo_get_HttpHeadersList
#else
#define UPNP_LIB_PARAM
#define UPNP_LIB_ARG
#define UPNP_LIB_ARG_SINGLE
#define UPNP_LIB_MEMBER
#define UPNP_LIB_INIT_LIST
#define UPNP_FUNPTR_COOKIE_CONST
#endif

#endif // GERBERA_UPNP_COMPAT_H
