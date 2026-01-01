/*GRB*
    Gerbera - https://gerbera.io/

    pg_result.cc - this file is part of Gerbera.

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

/// @file database/postgres/pg_result.cc
/// @brief Implementation of the SLResult classes.

#define GRB_LOG_FAC GrbLogFacility::postgres

#ifdef HAVE_PGSQL

#include "pg_result.h"

PostgresSQLResult::PostgresSQLResult(const pqxx::result& r)
    : result(r)
    , row(0)
{
}

PostgresSQLResult::~PostgresSQLResult()
{
    result.clear();
}

std::unique_ptr<SQLRow> PostgresSQLResult::nextRow()
{
    if (result.size() == 0 || row >= result.size())
        return nullptr;
    row++;
    return std::make_unique<PostgresSQLRow>(result.at(row - 1));
}

#endif