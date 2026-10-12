/*GRB*

    Gerbera - https://gerbera.io/

    fmt_compat.h - this file is part of Gerbera.

    Copyright (C) 2026 Gerbera Contributors

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
/// @file util/fmt_compat.h

#ifndef __GRB_FMT_COMPAT__
#define __GRB_FMT_COMPAT__

#include <fmt/core.h>
#include <fmt/format.h>
#if FMT_VERSION >= 100202
#include <fmt/ranges.h>
#endif
#if FMT_VERSION >= 110000
#include <fmt/std.h>
#define EX(ex) (ex)
#else
#define EX(ex) (ex.what())
#endif
#include <fmt/chrono.h>

#endif
