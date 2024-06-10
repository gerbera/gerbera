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

using LogFacilityIterator = EnumIterator<GrbLogFacility, GrbLogFacility::thread, GrbLogFacility::log_MAX>;

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

    if (toLower(GrbLogger::facilities[GrbLogFacility::log_MAX].data()) == toLower(flag)) {
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
std::map<GrbLogFacility, std::string_view> GrbLogger::facilities = {
    { GrbLogFacility::thread, "Thread" },
    { GrbLogFacility::sqlite3, "Sqlite3" },
    { GrbLogFacility::cds, "CDS" },
    { GrbLogFacility::server, "Server" },
    { GrbLogFacility::config, "Config" },
    { GrbLogFacility::content, "Content" },
    { GrbLogFacility::update, "Update" },
    { GrbLogFacility::mysql, "MySQL" },
    { GrbLogFacility::sqldatabase, "SQL" },
    { GrbLogFacility::proc, "Proc" },
    { GrbLogFacility::autoscan, "Autoscan" },
    { GrbLogFacility::script, "Script" },
    { GrbLogFacility::web, "Web" },
    { GrbLogFacility::layout, "Layout" },
    { GrbLogFacility::exif, "Exif" },
    { GrbLogFacility::exiv2, "Exiv2" },
    { GrbLogFacility::transcoding, "Transcoding" },
    { GrbLogFacility::taglib, "Taglib" },
    { GrbLogFacility::ffmpeg, "FFMpeg" },
    { GrbLogFacility::wavpack, "WavPack" },
    { GrbLogFacility::requests, "Requests" },
    { GrbLogFacility::device, "Device" },
    { GrbLogFacility::connmgr, "ConnMgr" },
    { GrbLogFacility::mrregistrar, "MRRegistrar" },
    { GrbLogFacility::xml, "XML" },
    { GrbLogFacility::clients, "Clients" },
    { GrbLogFacility::iohandler, "IoHandler" },
    { GrbLogFacility::online, "Online" },
    { GrbLogFacility::metadata, "Metadata" },
    { GrbLogFacility::matroska, "Matroska" },

    { GrbLogFacility::log_MAX, "All" },
};
#endif
