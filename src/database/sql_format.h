/*GRB*
  Gerbera - https://gerbera.io/

  sql_format.h - this file is part of Gerbera.

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

  $Id$
*/

/// \file sql_format.h

#ifndef __SQL_FORMAT_H__
#define __SQL_FORMAT_H__

#include <fmt/format.h>
#include <string_view>

struct SQLIdentifier {
    SQLIdentifier(const std::string_view& name, char quote_begin, char quote_end)
        : name(name)
        , quote_begin(quote_begin)
        , quote_end(quote_end)
    {
    }
    std::string_view name;
    char quote_begin;
    char quote_end;
};

template <>
struct fmt::formatter<SQLIdentifier> : formatter<std::string_view> {
    template <typename FormatContext>
    auto format(const SQLIdentifier& tn, FormatContext& ctx) -> decltype(ctx.out())
    {
        return format_to(ctx.out(), "{}{}{}", tn.quote_begin, tn.name, tn.quote_end);
    }
};

#endif
