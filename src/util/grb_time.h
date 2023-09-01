/*GRB*
Gerbera - https://gerbera.io/

    grb_time.h - this file is part of Gerbera.

    Copyright (C) 2022-2023 Gerbera Contributors

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

/// \file grb_time.h
/// \brief time handling

#ifndef __GRB_TIME_H__
#define __GRB_TIME_H__

#include <chrono>
#include <string>
#include <string_view>

template <typename TP>
std::chrono::seconds toSeconds(TP tp)
{
    auto asSystemTime = std::chrono::time_point_cast<std::chrono::system_clock::duration>(tp - TP::clock::now()
        + std::chrono::system_clock::now());
    return std::chrono::duration_cast<std::chrono::seconds>(asSystemTime.time_since_epoch());
}

/// \brief Converts a number of milliseconds to "H*:MM:SS.F*" representation as required by the UPnP duration spec
std::string millisecondsToHMSF(long long milliseconds);

/// \brief converts a "H*:MM:SS.F*" representation to milliseconds
long long HMSFToMilliseconds(std::string_view time);

std::chrono::seconds currentTime();
std::chrono::milliseconds currentTimeMS();

std::chrono::milliseconds getDeltaMillis(std::chrono::milliseconds ms);
std::chrono::milliseconds getDeltaMillis(std::chrono::milliseconds first, std::chrono::milliseconds second);

bool parseTime(int& value, std::string& timeValue, bool seconds = true);

#endif // __GRB_TIME_H__
