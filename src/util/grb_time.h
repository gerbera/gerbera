/*GRB*
Gerbera - https://gerbera.io/

    grb_time.h - this file is part of Gerbera.

    Copyright (C) 2022-2026 Gerbera Contributors

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

/// @file util/grb_time.h
/// @brief time handling

#ifndef __GRB_TIME_H__
#define __GRB_TIME_H__

#include <chrono>
#include <string>

enum class GrbTimeType {
    Seconds,
    Minutes,
    Hours,
};

template <typename TP>
std::chrono::seconds toSeconds(TP tp)
{
    auto asSystemTime = std::chrono::time_point_cast<std::chrono::system_clock::duration>(tp - TP::clock::now()
        + std::chrono::system_clock::now());
    return std::chrono::duration_cast<std::chrono::seconds>(asSystemTime.time_since_epoch());
}

/// @brief Converts a number of milliseconds to "H*:MM:SS.F*" representation as required by the UPnP duration spec
std::string millisecondsToHMSF(long long milliseconds);

/// @brief converts a "H*:MM:SS.F*" representation to milliseconds
long long HMSFToMilliseconds(const std::string& time);
std::string millisecondsToString(long long milliseconds, bool all = false);

std::chrono::seconds currentTime();
std::chrono::milliseconds currentTimeMS();

std::chrono::milliseconds getDeltaMillis(std::chrono::milliseconds ms);
std::chrono::milliseconds getDeltaMillis(std::chrono::milliseconds first, std::chrono::milliseconds second);

bool parseSimpleDate(const std::string& s, std::chrono::seconds& date);
bool parseDate(const char* value, std::tm& tmWork);
bool parseTime(long long& value, std::string& timeValue, GrbTimeType type = GrbTimeType::Seconds);
std::string makeSimpleDate(std::string& s);

/// @brief Converts seconds to localtime and prints a formatted string
std::string grbLocaltime(const std::string& format, const std::chrono::seconds& t);

#endif // __GRB_TIME_H__
