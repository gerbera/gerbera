/*MT*

    MediaTomb - http://www.mediatomb.cc/

    logger.h - this file is part of MediaTomb.

    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>

    Copyright (C) 2006-2010 Gena Batyan <bgeradz@mediatomb.cc>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>,
                            Leonhard Wimmer <leo@mediatomb.cc>

    MediaTomb is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.

    MediaTomb is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    version 2 along with MediaTomb; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.

    $Id$
*/

/// \file logger.h

#ifndef __LOGGER_H__
#define __LOGGER_H__

#include <fmt/format.h>
#include <spdlog/spdlog.h>
#include <type_traits>

#define log_debug SPDLOG_DEBUG
#define log_info SPDLOG_INFO
#define log_warning SPDLOG_WARN
#define log_error SPDLOG_ERROR
#define log_js SPDLOG_INFO

#ifndef __cpp_lib_to_underlying
template <typename E>
constexpr auto to_underlying(E e) noexcept
{
    return std::underlying_type_t<E>(e);
}
#else
using std::to_underlying;
#endif

#if FMT_VERSION >= 80100
template <typename T>
struct fmt::formatter<T, std::enable_if_t<std::is_enum_v<T>, char>>
    : formatter<std::underlying_type_t<T>> {
    template <typename FormatContext>
    auto format(const T& value, FormatContext& ctx) -> decltype(ctx.out())
    {
        return fmt::formatter<std::underlying_type_t<T>>::format(
            static_cast<std::underlying_type_t<T>>(value), ctx);
    }
};
#endif

#endif // __LOGGER_H__
