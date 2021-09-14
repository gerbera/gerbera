/*MT*

    MediaTomb - http://www.mediatomb.cc/

    database.cc - this file is part of MediaTomb.

    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>

    Copyright (C) 2006-2010 Gena Batyan <bgeradz@mediatomb.cc>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>,
                            Leonhard Wimmer <leo@mediatomb.cc>

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

#include "database.h" // API

#include "config/config_manager.h"
#include "database/mysql/mysql_database.h"
#include "database/sqlite3/sqlite_database.h"
#include "util/tools.h"

std::shared_ptr<Database> Database::createInstance(const std::shared_ptr<Config>& config, const std::shared_ptr<Mime>& mime, const std::shared_ptr<Timer>& timer)
{
    std::string type = config->getOption(CFG_SERVER_STORAGE_DRIVER);
    bool use_transaction = config->getBoolOption(CFG_SERVER_STORAGE_USE_TRANSACTIONS);
    auto database = [&]() -> std::shared_ptr<Database> {
        if (type == "sqlite3" && use_transaction) {
            return std::make_shared<Sqlite3DatabaseWithTransactions>(config, mime, timer);
        }
        if (type == "sqlite3") {
            return std::make_shared<Sqlite3Database>(config, mime, timer);
        }
#ifdef HAVE_MYSQL
        if (type == "mysql" && use_transaction) {
            return std::make_shared<MySQLDatabaseWithTransactions>(config, mime);
        }
        if (type == "mysql") {
            return std::make_shared<MySQLDatabase>(config, mime);
        }
#endif
        // other database types...
        throw_std_runtime_error("Unknown database type: {}", type);
    }();

    database->init();

    return database;
}

void Database::stripAndUnescapeVirtualContainerFromPath(std::string virtualPath, std::string& first, std::string& last)
{
    if (virtualPath.at(0) != VIRTUAL_CONTAINER_SEPARATOR) {
        throw_std_runtime_error("Got non-absolute virtual path {}; needs to start with: {}", virtualPath, VIRTUAL_CONTAINER_SEPARATOR);
    }
    auto sep = virtualPath.rfind(VIRTUAL_CONTAINER_SEPARATOR);
    if (sep == 0) {
        first = "/";
        last = virtualPath.substr(1);
    } else {
        while (sep != std::string::npos) {
            char beforeSep = virtualPath.at(sep - 1);
            if (beforeSep != VIRTUAL_CONTAINER_ESCAPE) {
                break;
            }
            sep = virtualPath.rfind(VIRTUAL_CONTAINER_SEPARATOR, sep - 1);
        }
        if (sep == 0) {
            first = "/";
        } else {
            first = virtualPath.substr(0, sep);
        }
        last = unescape(virtualPath.substr(sep + 1), VIRTUAL_CONTAINER_ESCAPE);
    }
}
