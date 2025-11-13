/*GRB*
    Gerbera - https://gerbera.io/

    sl_result.cc - this file is part of Gerbera.

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

/// @file database/sqlite3/sl_result.cc
/// @brief Implementation of the SLResult classes.

#include "sl_result.h"

#include <sqlite3.h>

/* Sqlite3Row */

Sqlite3Row::Sqlite3Row(char** row)
    : row(row)
{
}

char* Sqlite3Row::col_c_str(int index) const
{
    return row[index];
}

/* Sqlite3Result */

Sqlite3Result::Sqlite3Result()
    : table(nullptr, sqlite3_free_table)
{
}

Sqlite3Result::~Sqlite3Result() = default;

std::unique_ptr<SQLRow> Sqlite3Result::nextRow()
{
    if (nrow) {
        row += ncolumn;
        cur_row++;
        if (cur_row <= nrow) {
            return std::make_unique<Sqlite3Row>(row);
        }
        return nullptr;
    }
    return nullptr;
}
