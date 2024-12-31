/*GRB*
    Gerbera - https://gerbera.io/

    sqlite_config.h - this file is part of Gerbera.

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

/// \file sqlite_config.h
///\brief Definitions of the SqliteConfig class.

#ifndef __SQLITE_CONFIG_H__
#define __SQLITE_CONFIG_H__

#include <string>

class SqliteConfig {
public:
    static bool CheckSqlLiteSyncValue(std::string& value);
    static std::string PrintSqlLiteSyncValue(int value);
    static bool CheckSqlJournalMode(std::string& value);
    static bool CheckSqlLiteRestoreValue(std::string& value);
};

#endif // __SQLITE_CONFIG_H__