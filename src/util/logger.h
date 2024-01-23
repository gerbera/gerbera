/*MT*

    MediaTomb - http://www.mediatomb.cc/

    logger.h - this file is part of MediaTomb.

    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>

    Copyright (C) 2006-2010 Gena Batyan <bgeradz@mediatomb.cc>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>,
                            Leonhard Wimmer <leo@mediatomb.cc>

    Copyright (C) 2016-2024 Gerbera Contributors

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

#include <array>
#include <fmt/format.h>
#include <map>
#include <spdlog/spdlog.h>
#include <type_traits>

#define log_debug SPDLOG_TRACE
#define log_dbg SPDLOG_TRACE
#define log_faci SPDLOG_DEBUG
#define log_info SPDLOG_INFO
#define log_warning SPDLOG_WARN
#define log_error SPDLOG_ERROR

enum class log_facility_t {
    thread,
    sqlite3,
    cds,
    server,
    content,
    update,
    mysql,
    sqldatabase,
    proc,
    autoscan,
    script,
    web,
    layout,
    exif,
    transcoding,
    taglib,
    ffmpeg,
    wavpack,
    requests,
    connmgr,
    mrregistrar,
    xml,
    clients,
    iohandler,
    online,
    metadata,
    matroska,

    log_MAX,
};

#ifndef __cpp_lib_to_underlying
template <typename E>
constexpr auto to_underlying(E e) noexcept
{
    return static_cast<std::underlying_type_t<E>>(e);
}
#else
using std::to_underlying;
#endif

#ifdef GRBDEBUG
class GrbLogger {
public:
    static GrbLogger Logger;

    void init(int debugMode);
    bool isDebugging(log_facility_t facility)
    {
        return GrbLogger::Logger.hasDebugging[to_underlying(facility)];
    }
    static std::string_view mapFacility(log_facility_t facility)
    {
        return GrbLogger::facilities[facility];
    }
    static std::string printFacility(int facility);
    static int remapFacility(const std::string& flag);
    static int makeFacility(const std::string& optValue);

private:
    static std::map<log_facility_t, std::string_view> facilities;

    std::array<bool, to_underlying(log_facility_t::log_MAX)> hasDebugging {};
};

#define log_facility(fac, ...) GrbLogger::Logger.isDebugging((fac)) ? log_faci(__VA_ARGS__) : log_dbg(__VA_ARGS__)

#ifdef LOG_FAC

#undef log_debug
#define log_debug(...) log_facility(LOG_FAC, __VA_ARGS__)

#endif

#else

#define log_facility(fac, ...) log_debug(__VA_ARGS__)

#endif

#define log_threading(...) log_facility(log_facility_t::thread, __VA_ARGS__)

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
