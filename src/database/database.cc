/*MT*

    MediaTomb - http://www.mediatomb.cc/

    database.cc - this file is part of MediaTomb.

    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>

    Copyright (C) 2006-2010 Gena Batyan <bgeradz@mediatomb.cc>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>,
                            Leonhard Wimmer <leo@mediatomb.cc>

    Copyright (C) 2016-2025 Gerbera Contributors

    MediaTomb is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.

    MediaTomb is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    version 2 along with MediaTomb; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.

    $Id$
*/

/// \file database.cc

#define GRB_LOG_FAC GrbLogFacility::sqldatabase
#include "database.h" // API

#include "config/config_val.h"
#include "database/mysql/mysql_database.h"
#include "database/sqlite3/sqlite_database.h"
#include "exceptions.h"

Database::Database(std::shared_ptr<Config> config)
    : config(std::move(config))
{
}

std::shared_ptr<Database> Database::createInstance(const std::shared_ptr<Config>& config,
    const std::shared_ptr<Mime>& mime,
    const std::shared_ptr<ConverterManager>& converterManager,
    const std::shared_ptr<Timer>& timer)
{
    auto database = [&]() -> std::shared_ptr<Database> {
        std::string type = config->getOption(ConfigVal::SERVER_STORAGE_DRIVER);
        bool useTransaction = config->getBoolOption(ConfigVal::SERVER_STORAGE_USE_TRANSACTIONS);

        if (type == "sqlite3" && useTransaction) {
            return std::make_shared<Sqlite3DatabaseWithTransactions>(config, mime, converterManager, timer);
        }
        if (type == "sqlite3") {
            return std::make_shared<Sqlite3Database>(config, mime, converterManager, timer);
        }
#ifdef HAVE_MYSQL
        if (type == "mysql" && useTransaction) {
            return std::make_shared<MySQLDatabaseWithTransactions>(config, mime, converterManager);
        }
        if (type == "mysql") {
            return std::make_shared<MySQLDatabase>(config, mime, converterManager);
        }
#endif
        // other database types...
        throw_std_runtime_error("Unknown database type: {}", type);
    }();

    database->init();

    return database;
}
