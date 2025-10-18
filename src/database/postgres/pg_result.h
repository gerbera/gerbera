/*GRB*
    Gerbera - https://gerbera.io/

    pg_result.h - this file is part of Gerbera.

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

/// @file database/postgres/pg_result.h
/// @brief Definitions of the PostgresResult classes.

#ifndef __POSTGRES_RESULT_H__
#define __POSTGRES_RESULT_H__

#include "database/sql_result.h"

#include <pqxx/pqxx>

/// @brief Represents a result of a Postgres select
class PostgresSQLResult : public SQLResult {
public:
    explicit PostgresSQLResult(const pqxx::result& r);
    PostgresSQLResult() = default;
    ~PostgresSQLResult() override;

    PostgresSQLResult(const PostgresSQLResult&) = delete;
    PostgresSQLResult& operator=(const PostgresSQLResult&) = delete;

private:
    std::unique_ptr<SQLRow> nextRow() override;
    unsigned long long getNumRows() const override { return result.size(); }

    pqxx::result result;
    pqxx::result::size_type row;
};

/// @brief Represents a row of a result of a Postgres select
class PostgresSQLRow : public SQLRow {
public:
    explicit PostgresSQLRow(const pqxx::row& r)
        : row(r)
    {
    }

private:
    char* col_c_str(int index) const override
    {
        return (index < row.size() && !row.at(index).is_null())
            ? const_cast<char*>(row.at(index).c_str())
            : nullptr;
    }
    pqxx::row row;
};

#endif // __POSTGRES_RESULT_H__