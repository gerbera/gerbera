/*GRB*
Gerbera - https://gerbera.io/

    grb_time.cc - this file is part of Gerbera.

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

/// @file util/grb_time.cc
#define GRB_LOG_FAC GrbLogFacility::content
#include "grb_time.h" // API

#include "logger.h"
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

std::string millisecondsToString(long long milliseconds, bool all)
{
    auto ms = milliseconds % 1000;
    milliseconds /= 1000;

    auto seconds = milliseconds % 60;
    milliseconds /= 60;

    auto minutes = milliseconds % 60;
    auto hours = milliseconds / 60;
    auto fmtStr = hours > 0 ? "{0:01}:{1:02}:{2:02}" : "{1:02}:{2:02}";

    return fmt::format(fmtStr, hours, minutes, seconds) + (all ? fmt::format(".{:03}", ms) : "");
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

long long HMSFToMilliseconds(const std::string& time)
{
    if (time.empty()) {
        log_warning("Could not convert time representation to seconds!");
        return 0;
    }

    long long hours = 0;
    long long minutes = 0;
    long long seconds = 0;
    long long ms = 0;
    if (sscanf(time.c_str(), "%lld:%lld:%lld.%lld", &hours, &minutes, &seconds, &ms) > 3)
        return ((hours * 3600) + (minutes * 60) + seconds) * 1000 + ms;

    log_warning("Could not parse time '{}'!", time);
    return 0;
}

static const auto timeFactors = std::map<GrbTimeType, std::vector<int>> {
    { GrbTimeType::Seconds, { 1, 60, 3600, 24 * 3600 } },
    { GrbTimeType::Minutes, { 1, 60, 24 * 60 } },
    { GrbTimeType::Hours, { 1, 24 } },
};

std::string makeSimpleDate(std::string& s)
{
    constexpr auto date_time_format = "%Y-%m-%dT%H:%M:%S";
    std::istringstream ss { s };
    std::tm dt {};

    ss >> std::get_time(&dt, date_time_format);
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
    log_warning("Failed to convert {}", s);
    replaceAllString(s, "+0100", "Z");
    replaceAllString(s, "+0200", "Z");
    return s;
}

bool parseDate(const char* value, std::tm& tmWork)
{
    if (strptime(value, "%Y-%m-%dT%T.000000%Z", &tmWork)) {
        // convert creation_time to local time
    } else if (strptime(value, "%Y-%m-%d", &tmWork)) {
        ; // use the value as is
        tmWork.tm_hour = 13;
        tmWork.tm_min = 0;
        tmWork.tm_sec = 0;
        tmWork.tm_isdst = false;
    } else if (strptime(value, "%Y%m%d", &tmWork)) {
        ; // use the value as is
        tmWork.tm_hour = 13;
        tmWork.tm_min = 0;
        tmWork.tm_sec = 0;
        tmWork.tm_isdst = false;
    } else if (strptime(value, "%Y", &tmWork)) {
        // convert the value to "XXXX-01-01"
        tmWork.tm_mon = 0; // Month (0-11)
        tmWork.tm_mday = 1; // Day of the month (1-31)
        tmWork.tm_hour = 13;
        tmWork.tm_min = 0;
        tmWork.tm_sec = 0;
        tmWork.tm_isdst = false;
    } else
        return false;
    std::time_t utcTime = timegm(&tmWork);
    if (utcTime == -1) {
        return false;
    }
    localtime_r(&utcTime, &tmWork);
    return true;
}

bool parseSimpleDate(const std::string& s, std::chrono::seconds& date)
{
    constexpr auto date_time_format = "%Y-%m-%dT%H:%M:%S";
    std::istringstream ss { s };
    std::tm dt {};

    ss >> std::get_time(&dt, date_time_format);
    if (!ss.fail()) {
        auto t = std::mktime(&dt);
        date = std::chrono::seconds(t);
        log_debug("{} -> {:%FT%H:%M:%SZ}", s, dt);
        return true;
    }
    log_warning("Failed to convert {}", s);
    return false;
}

bool parseTime(long long& value, std::string& timeValue, GrbTimeType type)
{
    static auto re = std::regex("^([-0-9:]+)$");
    value = 0;

    if (!std::regex_search(timeValue, re))
        return false;

    auto list = splitString(timeValue, ':');
    std::reverse(list.begin(), list.end());

    const auto& factors = timeFactors.at(type);
    if (list.size() > factors.size())
        return false;
    std::size_t index = 0;
    for (auto&& item : list) {
        auto iVal = stolString(item, -1);
        if (iVal < 0 && list.size() > 1) {
            value = 0;
            return false;
        }
        value += iVal * factors.at(index);
        index++;
    }
    if (!list.empty())
        timeValue = fmt::to_string(value);
    return !list.empty();
}

std::string grbLocaltime(const std::string& format, const std::chrono::seconds& t)
{
    std::time_t time = t.count();
    std::tm tm;
    localtime_r(&time, &tm);
    return fmt::format(format, tm);
}
