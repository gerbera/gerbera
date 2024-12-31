/*GRB*
    Gerbera - https://gerbera.io/

    sqlite_config.cc - this file is part of Gerbera.

    Copyright (C) 2023-2025 Gerbera Contributors

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

/// \file sqlite_config.cc

#define GRB_LOG_FAC GrbLogFacility::sqlite3
#include "sqlite_config.h" // API

#include <array>
#include <map>
#include <string_view>

#include "config/config_setup.h"
#include "util/tools.h"

static std::map<std::string, int> sqliteSync = {
    { "off", 0 },
    { "normal", 1 },
    { "full", 2 },
    { "extra", 3 },
};

std::string SqliteConfig::PrintSqlLiteSyncValue(int value)
{
    for (auto&& [key, mode] : sqliteSync) {
        if (value == mode) {
            return key;
        }
    }
    return fmt::to_string(value);
}

bool SqliteConfig::CheckSqlLiteSyncValue(std::string& value)
{
    for (auto&& [key, mode] : sqliteSync) {
        if (value == key || value == fmt::to_string(mode)) {
            value.assign(fmt::to_string(mode));
            return true;
        }
    }
    return false;
}

static constexpr auto sqliteJournalModes = std::array<std::string_view, 6> { "DELETE", "TRUNCATE", "PERSIST", "MEMORY", "WAL", "OFF" };

bool SqliteConfig::CheckSqlJournalMode(std::string& value)
{
    value.assign(toUpper(value));
    return std::find(sqliteJournalModes.begin(), sqliteJournalModes.end(), value) != sqliteJournalModes.end();
}

static std::map<std::string, bool> sqliteRestore = {
    { "restore", true },
    { YES, true },
    { "fail", false },
    { NO, false },
};

bool SqliteConfig::CheckSqlLiteRestoreValue(std::string& value)
{
    for (auto&& [key, mode] : sqliteRestore) {
        if (value == key) {
            value.assign(mode ? YES : NO);
            return true;
        }
    }

    return false;
}
