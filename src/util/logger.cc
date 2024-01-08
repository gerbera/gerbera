/*GRB*

    Gerbera - https://gerbera.io/

    logger.cc - this file is part of Gerbera.

    Copyright (C) 2022-2024 Gerbera Contributors

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

/// \file logger.cc

#ifdef GRBDEBUG

#include "util/logger.h" // API

#include <numeric>

#include "util/enum_iterator.h"
#include "util/tools.h"

using LogFacilityIterator = EnumIterator<log_facility_t, log_facility_t::thread, log_facility_t::log_MAX>;

void GrbLogger::init(int debugMode)
{
    if (debugMode > 0) {
        for (auto&& bit : LogFacilityIterator()) {
            GrbLogger::Logger.hasDebugging[to_underlying(bit)] = debugMode & (1 << to_underlying(bit));
            if (GrbLogger::Logger.hasDebugging[to_underlying(bit)])
                log_info("Enable Logging for {}", facilities[bit]);
        }
        if (spdlog::get_level() > spdlog::level::trace)
            spdlog::set_level(spdlog::level::debug);
        spdlog::set_pattern("%Y-%m-%d %X.%e %6l: [%s:%#] %!(): %v");
    }
}

int GrbLogger::remapFacility(const std::string& flag)
{
    for (auto&& bit : LogFacilityIterator()) {
        if (toLower(GrbLogger::facilities[bit].data()) == toLower(flag)) {
            return 1 << to_underlying(bit);
        }
    }

    if (toLower(GrbLogger::facilities[log_facility_t::log_MAX].data()) == toLower(flag)) {
        int result = 0;
        for (auto&& bit : LogFacilityIterator())
            result |= 1 << to_underlying(bit);
        return result;
    }
    return stoiString(flag, 0, 0);
}

std::string GrbLogger::printFacility(int facility)
{
    std::vector<std::string> myFlags;

    for (auto [bit, label] : facilities) {
        if (facility & (1 << to_underlying(bit))) {
            myFlags.emplace_back(label.data());
            facility &= ~(1 << to_underlying(bit));
        }
    }

    if (facility) {
        myFlags.push_back(fmt::format("{:#04x}", facility));
    }

    return fmt::format("{}", fmt::join(myFlags, " | "));
}

int GrbLogger::makeFacility(const std::string& optValue)
{
    std::vector<std::string> flagsVector = splitString(optValue, '|', false);
    return std::accumulate(flagsVector.begin(), flagsVector.end(), 0, [](auto flg, auto&& i) { return flg | GrbLogger::remapFacility(trimString(i)); });
}

GrbLogger GrbLogger::Logger;
std::map<log_facility_t, std::string_view> GrbLogger::facilities = {
    { log_facility_t::thread, "Thread" },
    { log_facility_t::sqlite3, "Sqlite3" },
    { log_facility_t::cds, "CDS" },
    { log_facility_t::server, "Server" },
    { log_facility_t::content, "Content" },
    { log_facility_t::update, "Update" },
    { log_facility_t::mysql, "MySQL" },
    { log_facility_t::sqldatabase, "SQL" },
    { log_facility_t::proc, "Proc" },
    { log_facility_t::autoscan, "Autoscan" },
    { log_facility_t::script, "Script" },
    { log_facility_t::web, "Web" },
    { log_facility_t::layout, "Layout" },
    { log_facility_t::exif, "Exif" },
    { log_facility_t::transcoding, "Transcoding" },
    { log_facility_t::taglib, "Taglib" },
    { log_facility_t::ffmpeg, "FFMpeg" },
    { log_facility_t::wavpack, "WavPack" },
    { log_facility_t::requests, "Requests" },
    { log_facility_t::connmgr, "ConnMgr" },
    { log_facility_t::mrregistrar, "MRRegistrar" },
    { log_facility_t::xml, "XML" },
    { log_facility_t::clients, "Clients" },
    { log_facility_t::iohandler, "IoHandler" },
    { log_facility_t::online, "Online" },
    { log_facility_t::metadata, "Metadata" },
    { log_facility_t::matroska, "Matroska" },

    { log_facility_t::log_MAX, "All" },
};
#endif
