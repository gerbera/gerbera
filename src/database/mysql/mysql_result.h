/*GRB*
    Gerbera - https://gerbera.io/

    mysql_result.h - this file is part of Gerbera.

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

/// @file database/mysql/mysql_result.h
/// @brief Definitions of the MysqlResult classes.

#ifndef __MYSQL_RESULT_H__
#define __MYSQL_RESULT_H__

#include "database/sql_result.h"

#include <mysql.h>

class MysqlResult : public SQLResult {
public:
    explicit MysqlResult(MYSQL_RES* mysqlRes);
    ~MysqlResult() override;

    MysqlResult(const MysqlResult&) = delete;
    MysqlResult& operator=(const MysqlResult&) = delete;

private:
    int nullRead {};
    std::unique_ptr<SQLRow> nextRow() override;
    unsigned long long getNumRows() const override { return mysql_num_rows(mysqlRes); }
    MYSQL_RES* mysqlRes;

    friend class MysqlRow;
    friend class MySQLDatabase;
};

class MysqlRow : public SQLRow {
public:
    explicit MysqlRow(MYSQL_ROW mysqlRow);

private:
    char* col_c_str(int index) const override;

    MYSQL_ROW mysqlRow;
};

#endif // __MYSQL_RESULT_H__
