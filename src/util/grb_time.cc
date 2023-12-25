/*GRB*
Gerbera - https://gerbera.io/

    grb_time.cc - this file is part of Gerbera.

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

/// \file grb_time.cc

#include "grb_time.h" // API

#include "common.h"
#include "tools.h"

#include <fmt/chrono.h>
#include <fmt/core.h>
#include <iomanip>
#include <regex>
#include <sstream>

std::chrono::seconds currentTime()
{
    return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch());
}

std::chrono::milliseconds currentTimeMS()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
}

std::chrono::milliseconds getDeltaMillis(std::chrono::milliseconds ms)
{
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
    return now - ms;
}

std::chrono::milliseconds getDeltaMillis(std::chrono::milliseconds first, std::chrono::milliseconds second)
{
    return second - first;
}

std::string millisecondsToHMSF(long long milliseconds)
{
    auto ms = milliseconds % 1000;
    milliseconds /= 1000;

    auto seconds = milliseconds % 60;
    milliseconds /= 60;

    auto minutes = milliseconds % 60;
    auto hours = milliseconds / 60;

    return fmt::format("{:01}:{:02}:{:02}.{:03}", hours, minutes, seconds, ms);
}

long long HMSFToMilliseconds(std::string_view time)
{
    if (time.empty()) {
        log_warning("Could not convert time representation to seconds!");
        return 0;
    }

    long long hours = 0;
    long long minutes = 0;
    long long seconds = 0;
    long long ms = 0;
    if (sscanf(time.data(), "%lld:%lld:%lld.%lld", &hours, &minutes, &seconds, &ms) > 3)
        return ((hours * 3600) + (minutes * 60) + seconds) * 1000 + ms;

    log_warning("Could not parse time '{}'!", time);
    return 0;
}

static const auto secondsFactors = std::vector { 1, 60, 3600, 24 * 3600 };
static const auto minutesFactors = std::vector { 1, 60, 24 * 60 };

std::string makeSimpleDate(std::string& s)
{
    std::string_view date_time_format { "%Y-%m-%dT%H:%M:%S" };
    std::istringstream ss { s };
    std::tm dt {};

    ss >> std::get_time(&dt, date_time_format.data());
    if (!ss.fail()) {
        int tz;
        ss >> tz;
        dt.tm_isdst = -1;
        dt.tm_hour -= tz / 100;
        dt.tm_min -= tz % 100;
        std::mktime(&dt);
        log_debug("{} -> {:%FT%H:%M:%SZ}... {}-{}", s, dt, -tz / 100, -tz % 100);
        return fmt::format("{:%FT%H:%M:%SZ}", dt);
    }
    log_debug("Failed to convert {}", s);
    replaceAllString(s, "+0100", "Z");
    replaceAllString(s, "+0200", "Z");
    return s;
}

bool parseTime(int& value, std::string& timeValue, bool seconds)
{
    static auto re = std::regex("^([-0-9:]+)$");
    auto factors = seconds ? secondsFactors : minutesFactors;
    value = 0;
    if (!std::regex_search(timeValue, re))
        return false;
    auto list = splitString(timeValue, ':');
    std::reverse(list.begin(), list.end());
    if (list.size() > factors.size())
        return false;
    int index = 0;
    for (auto&& item : list) {
        auto iVal = stoiString(item, -1);
        if (iVal < 0 && list.size() > 1) {
            value = 0;
            return false;
        }
        value += iVal * factors[index];
        index++;
    }
    if (list.size() > 0)
        timeValue = fmt::to_string(value);
    return list.size() > 0;
}
