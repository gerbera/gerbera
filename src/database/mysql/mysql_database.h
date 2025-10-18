/*MT*

    MediaTomb - http://www.mediatomb.cc/

    mysql_database.h - this file is part of MediaTomb.

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

/// @file database/mysql/mysql_database.h

#ifdef HAVE_MYSQL

#ifndef __MYSQL_DATABASE_H__
#define __MYSQL_DATABASE_H__

#include "database/sql_database.h"

#include "config/config_val.h"

#include <mysql.h>
#include <vector>

/// @brief The Database class for using MySQL
class MySQLDatabase
    : public SQLDatabase,
      public std::enable_shared_from_this<SQLDatabase> {
public:
    explicit MySQLDatabase(const std::shared_ptr<Config>& config, const std::shared_ptr<Mime>& mime, const std::shared_ptr<ConverterManager>& converterManager);
    ~MySQLDatabase() override;

    MySQLDatabase(const MySQLDatabase&) = delete;
    MySQLDatabase& operator=(const MySQLDatabase&) = delete;

    void dropTables() override;

protected:
    void _exec(const std::string& query) override;
    void checkMysqlThreadInit() const;
    std::string prepareDatabase();

    static std::string getError(MYSQL* db);

    MYSQL db {};

private:
    void run() override;
    void init() override;
    void shutdownDriver() override;
    std::shared_ptr<Database> getSelf() override;

    std::string quote(const std::string& value) const override;

    std::shared_ptr<SQLResult> select(const std::string& query) override;
    void del(std::string_view tableName, const std::string& clause, const std::vector<int>& ids) override;
    void execOnTable(std::string_view tableName, const std::string& query, int objId) override;
    int exec(const std::string& query, const std::string& getLastInsertId = "") override;
    void execOnly(const std::string& query) override;

    void storeInternalSetting(const std::string& key, const std::string& value) override;

    bool mysql_connection {};

    void threadCleanup() override;
    bool threadCleanupRequired() const override { return true; }

    pthread_key_t mysql_init_key {};
    const int mysql_init_val = 1;
};

/// @brief The Database class for using MySQL with transactions
class MySQLDatabaseWithTransactions : public SqlWithTransactions, public MySQLDatabase {
public:
    MySQLDatabaseWithTransactions(const std::shared_ptr<Config>& config, const std::shared_ptr<Mime>& mime, const std::shared_ptr<ConverterManager>& converterManager);

    void beginTransaction(std::string_view tName) override;
    void rollback(std::string_view tName) override;
    void commit(std::string_view tName) override;

    std::shared_ptr<SQLResult> select(const std::string& query) override;
};

#endif // __MYSQL_DATABASE_H__

#endif // HAVE_MYSQL
