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

#include <stdint.h>

#include "compat.h"

#if (UPNP_VERSION > 170000 and UPNP_VERSION <= 170110)
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
