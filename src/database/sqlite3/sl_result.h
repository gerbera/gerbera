/*GRB*
    Gerbera - https://gerbera.io/

    sl_result.h - this file is part of Gerbera.

    Copyright (C) 2025 Gerbera Contributors

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

/// @file database/sqlite3/sl_result.h
/// @brief Definitions of the SLResult classes.

#ifndef __SQLITE3_RESULT_H__
#define __SQLITE3_RESULT_H__

#include "database/sql_result.h"

#include <memory>

/// @brief Represents a result of a sqlite3 select
class Sqlite3Result : public SQLResult {
public:
    Sqlite3Result();
    ~Sqlite3Result() override;

    Sqlite3Result(const Sqlite3Result&) = delete;
    Sqlite3Result& operator=(const Sqlite3Result&) = delete;

private:
    std::unique_ptr<SQLRow> nextRow() override;
    [[nodiscard]] unsigned long long getNumRows() const override { return nrow; }

    std::unique_ptr<char*, void (*)(char**)> table;
    char** row;

    int cur_row;

    int nrow;
    int ncolumn;

    friend class SLSelectTask;
};

/// @brief Represents a row of a result of a sqlite3 select
class Sqlite3Row : public SQLRow {
public:
    explicit Sqlite3Row(char** row);

private:
    char* col_c_str(int index) const override;
    char** row;
};

#endif // __SQLITE3_RESULT_H__
