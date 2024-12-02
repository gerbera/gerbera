/*GRB*

    Gerbera - https://gerbera.io/

    duk_compat.h - this file is part of Gerbera.

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

/// \file duk_compat.h

#ifndef __GRB_DUK_COMPAT_H__
#define __GRB_DUK_COMPAT_H__

#include <duktape.h>

#if DUK_VERSION < 20400
#define duk_safe_to_stacktrace duk_safe_to_string
#endif

#endif // __GRB_DUK_COMPAT_H__
