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

/// @file database/sql_result.h
/// @brief Definitions of the SQLResult classes.

#ifndef __SQL_RESULT_H__
#define __SQL_RESULT_H__

#include <memory>
#include <string>

class SQLRow {
public:
    SQLRow() = default;
    virtual ~SQLRow() = default;
    SQLRow(const SQLRow&) = delete;
    SQLRow& operator=(const SQLRow&) = delete;
    /// @brief Returns true if the column index contains the value NULL
    bool isNullOrEmpty(int index) const
    {
        const char* c = col_c_str(index);
        return c == nullptr || *c == '\0';
    }
    /// @brief Return the value of column index as a string value
    std::string col(int index) const
    {
        const char* c = col_c_str(index);
        if (!c)
            return {};
        return { c };
    }
    /// @brief Return the value of column index as an integer value
    int col_int(int index, int null_value) const
    {
        const char* c = col_c_str(index);
        if (!c || *c == '\0')
            return null_value;
        return std::atoi(c);
    }
    /// @brief Return the value of column index as an integer value
    long long col_long(int index, long long null_value) const
    {
        const char* c = col_c_str(index);
        if (!c || *c == '\0')
            return null_value;
        return std::atoll(c);
    }
    virtual char* col_c_str(int index) const = 0;
};

class SQLResult {
public:
    SQLResult(const SQLResult&) = delete;
    SQLResult& operator=(const SQLResult&) = delete;
    SQLResult() = default;
    virtual ~SQLResult() = default;
    virtual std::unique_ptr<SQLRow> nextRow() = 0;
    virtual unsigned long long getNumRows() const = 0;
};

#endif // __SQL_RESULT_H__
