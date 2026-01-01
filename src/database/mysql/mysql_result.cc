/*GRB*
    Gerbera - https://gerbera.io/

    mysql_result.cc - this file is part of Gerbera.

    Copyright (C) 2025-2026 Gerbera Contributors

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

/// @file database/mysql/mysql_result.cc

#ifdef HAVE_MYSQL

#include "mysql_result.h"

MysqlResult::MysqlResult(MYSQL_RES* mysqlRes)
    : mysqlRes(mysqlRes)
{
}

MysqlResult::~MysqlResult()
{
    if (mysqlRes) {
        if (!nullRead) {
            while (mysql_fetch_row(mysqlRes))
                ; // read out data
        }
        mysql_free_result(mysqlRes);
        mysqlRes = nullptr;
    }
}

std::unique_ptr<SQLRow> MysqlResult::nextRow()
{
    if (auto m = mysql_fetch_row(mysqlRes)) {
        return std::make_unique<MysqlRow>(m);
    }
    nullRead = true;
    mysql_free_result(mysqlRes);
    mysqlRes = nullptr;
    return nullptr;
}

/* MysqlRow */

MysqlRow::MysqlRow(MYSQL_ROW mysqlRow)
    : mysqlRow(mysqlRow)
{
}

char* MysqlRow::col_c_str(int index) const
{
    return mysqlRow[index];
}

#endif // HAVE_MYSQL