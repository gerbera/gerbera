/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    mysql_database.cc - this file is part of MediaTomb.
    
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

/// \file mysql_database.cc

#ifdef HAVE_MYSQL
#include "mysql_database.h"

#include <array>
#include <cstdlib>

#include <netinet/in.h>

#include "config/config_manager.h"
#include "util/thread_runner.h"
#include "util/tools.h"

//#define MYSQL_SET_NAMES "/*!40101 SET NAMES utf8 */"
//#define MYSQL_SELECT_DEBUG
//#define MYSQL_EXEC_DEBUG

#define MYSQL_SET_VERSION "INSERT INTO `mt_internal_setting` VALUES ('db_version','{}')"
#define MYSQL_UPDATE_VERSION "UPDATE `mt_internal_setting` SET `value`='{}' WHERE `key`='db_version' AND `value`='{}'"
#define MYSQL_ADD_RESOURCE_ATTR "ALTER TABLE `grb_cds_resource` ADD COLUMN `{}` varchar(255) default NULL"

MySQLDatabase::MySQLDatabase(std::shared_ptr<Config> config, std::shared_ptr<Mime> mime)
    : SQLDatabase(std::move(config), std::move(mime))
{
    table_quote_begin = '`';
    table_quote_end = '`';

    // if mysql.sql or mysql-upgrade.xml is changed hashies have to be updated, index 0 is used for create script
    hashies = { 3398516749, 928913698, 1984244483, 2241152998, 1748460509, 2860006966, 974692115, 70310290, 1863649106, 4238128129, 2979337694, 1512596496, 2030024486 };
}

MySQLDatabase::~MySQLDatabase()
{
    SqlAutoLock lock(sqlMutex); // just to ensure, that we don't close while another thread
    // is executing a query

    if (mysql_connection) {
        mysql_close(&db);
        mysql_connection = false;
    }
    log_debug("calling mysql_server_end...");
    mysql_server_end();
    log_debug("...ok");
}

void MySQLDatabase::checkMysqlThreadInit() const
{
    if (!mysql_connection)
        throw_std_runtime_error("mysql connection is not open or already closed");
    if (!pthread_getspecific(mysql_init_key)) {
        if (mysql_thread_init())
            throw_std_runtime_error("error while calling mysql_thread_init()");
        if (pthread_setspecific(mysql_init_key, reinterpret_cast<void*>(1)))
            throw_std_runtime_error("error while calling pthread_setspecific()");
    }
}

void MySQLDatabase::threadCleanup()
{
    if (pthread_getspecific(mysql_init_key)) {
        mysql_thread_end();
    }
}

void MySQLDatabase::init()
{
    log_debug("start");
    SQLDatabase::init();

    std::unique_lock<decltype(sqlMutex)> lock(sqlMutex);

    if (!mysql_thread_safe()) {
        throw_std_runtime_error("mysql library is not thread safe");
    }

    /// \todo write destructor function
    int ret = pthread_key_create(&mysql_init_key, nullptr);
    if (ret) {
        throw_std_runtime_error("could not create pthread_key");
    }
    mysql_server_init(0, nullptr, nullptr);
    pthread_setspecific(mysql_init_key, reinterpret_cast<void*>(1));

    MYSQL* res_mysql = mysql_init(&db);
    if (!res_mysql) {
        throw_std_runtime_error("mysql_init() failed");
    }

    mysql_init_key_initialized = true;

    mysql_options(&db, MYSQL_SET_CHARSET_NAME, "utf8mb4");

    bool my_bool_var = true;
    mysql_options(&db, MYSQL_OPT_RECONNECT, &my_bool_var);

    std::string dbHost = config->getOption(CFG_SERVER_STORAGE_MYSQL_HOST);
    std::string dbName = config->getOption(CFG_SERVER_STORAGE_MYSQL_DATABASE);
    std::string dbUser = config->getOption(CFG_SERVER_STORAGE_MYSQL_USERNAME);
    auto dbPort = in_port_t(config->getIntOption(CFG_SERVER_STORAGE_MYSQL_PORT));
    std::string dbPass = config->getOption(CFG_SERVER_STORAGE_MYSQL_PASSWORD);
    std::string dbSock = config->getOption(CFG_SERVER_STORAGE_MYSQL_SOCKET);

    res_mysql = mysql_real_connect(&db,
        dbHost.c_str(),
        dbUser.c_str(),
        (dbPass.empty() ? nullptr : dbPass.c_str()),
        dbName.c_str(),
        dbPort, // port
        (dbSock.empty() ? nullptr : dbSock.c_str()), // socket
        0 // flags
    );
    if (!res_mysql) {
        throw_std_runtime_error("Connecting to database {}:{}/{} failed: {}", dbHost, dbPort, dbName, getError(&db));
    }

    mysql_connection = true;

    std::string dbVersion;
    try {
        dbVersion = getInternalSetting("db_version");
    } catch (const std::runtime_error& e) {
        log_debug("{}", e.what());
    }

    if (dbVersion.empty()) {
        log_info("Database doesn't seem to exist. Creating database...");
        auto sqlFilePath = config->getOption(CFG_SERVER_STORAGE_MYSQL_INIT_SQL_FILE);
        log_debug("Loading initialisation SQL from: {}", sqlFilePath.c_str());
        auto sql = readTextFile(sqlFilePath);
        auto&& myHash = stringHash(sql);

        if (myHash == hashies[0]) {
            for (auto&& statement : splitString(sql, ';')) {
                trimStringInPlace(statement);
                if (statement.empty()) {
                    continue;
                }
                log_debug("executing statement: '{}'", statement);
                ret = mysql_real_query(&db, statement.c_str(), statement.size());
                if (ret) {
                    std::string myError = getError(&db);
                    throw DatabaseException(myError, fmt::format("Mysql: error while creating db: {}", myError));
                }
            }
            _exec(fmt::format(MYSQL_SET_VERSION, DBVERSION).c_str());
        } else {
            log_warning("Wrong hash for create script {}: {} != {}", DBVERSION, myHash, hashies[0]);
            throw_std_runtime_error("Wrong hash for create script {}", DBVERSION);
        }
        dbVersion = getInternalSetting("db_version");
        if (dbVersion.empty()) {
            shutdown();
            throw_std_runtime_error("error while creating database");
        }
        log_info("Database created successfully!");
    }

    upgradeDatabase(dbVersion, hashies, CFG_SERVER_STORAGE_MYSQL_UPGRADE_FILE, MYSQL_UPDATE_VERSION, MYSQL_ADD_RESOURCE_ATTR);

    lock.unlock();

    log_debug("end");
}

std::shared_ptr<Database> MySQLDatabase::getSelf()
{
    return shared_from_this();
}

std::string MySQLDatabase::quote(std::string value) const
{
    /* note: mysql_real_escape_string returns a maximum of (length * 2 + 1)
     * chars; we need +1 for the first ' - the second ' will be written over
     * the \0; then the string won't be null-terminated, but that doesn't matter,
     * because we give the correct length to std::string()
     */
    auto q = new char[value.length() * 2 + 2];
    *q = '\'';
    auto size = mysql_real_escape_string(const_cast<MYSQL*>(&db), q + 1, value.c_str(), value.length());
    q[size + 1] = '\'';
    std::string ret(q, size + 2);
    delete[] q;
    return ret;
}

std::string MySQLDatabase::getError(MYSQL* db)
{
    auto res = fmt::format("mysql_error ({}): \"{}\"", mysql_errno(db), mysql_error(db));
    log_debug("{}", res);
    return res;
}

void MySQLDatabase::beginTransaction(const std::string_view& tName)
{
    log_debug("START TRANSACTION {} {}", tName, inTransaction);
    StdThreadRunner::waitFor(
        "MySqlDatabase", [this] { return !inTransaction; }, 100);
    inTransaction = true;
    log_debug("START TRANSACTION {}", tName);
    SqlAutoLock lock(sqlMutex);
    if (use_transaction)
        _exec("START TRANSACTION");
}

void MySQLDatabase::rollback(const std::string_view& tName)
{
    log_debug("ROLLBACK {}", tName);
    if (use_transaction && inTransaction && mysql_rollback(&db)) {
        std::string myError = getError(&db);
        throw DatabaseException(myError, fmt::format("Mysql: error while rolling back db: {}", myError));
    }
    inTransaction = false;
}

void MySQLDatabase::commit(const std::string_view& tName)
{
    log_debug("COMMIT {}", tName);
    SqlAutoLock lock(sqlMutex);
    if (use_transaction && inTransaction && mysql_commit(&db)) {
        std::string myError = getError(&db);
        throw DatabaseException(myError, fmt::format("Mysql: error while commiting db: {}", myError));
    }
    inTransaction = false;
}

std::shared_ptr<SQLResult> MySQLDatabase::select(const char* query, size_t length)
{
#ifdef MYSQL_SELECT_DEBUG
    log_debug("{}", query);
    print_backtrace();
#endif

    checkMysqlThreadInit();
    SqlAutoLock lock(sqlMutex);
    bool myTransaction = false;
    if (!inTransaction) { // protect calls outside transactions
        inTransaction = true;
        myTransaction = true;
    }
    auto res = mysql_real_query(&db, query, length);
    if (res) {
        std::string myError = getError(&db);
        rollback("");
        throw DatabaseException(myError, fmt::format("Mysql: mysql_real_query() failed: {}; query: {}", myError, query));
    }

    MYSQL_RES* mysql_res = mysql_store_result(&db);
    if (!mysql_res && mysql_field_count(&db)) {
        std::string myError = getError(&db);
        rollback("");
        throw DatabaseException(myError, fmt::format("Mysql: mysql_store_result() failed: {}; query: {}", myError, query));
    }
    if (myTransaction) {
        inTransaction = false;
    }

    return std::make_shared<MysqlResult>(mysql_res);
}

int MySQLDatabase::exec(const char* query, size_t length, bool getLastInsertId)
{
#ifdef MYSQL_EXEC_DEBUG
    log_debug("{}", query);
    print_backtrace();
#endif

    checkMysqlThreadInit();
    SqlAutoLock lock(sqlMutex);
    auto res = mysql_real_query(&db, query, length);
    if (res) {
        std::string myError = getError(&db);
        rollback("");
        throw DatabaseException(myError, fmt::format("Mysql: mysql_real_query() failed: {}; query: {}", myError, query));
    }
    int insert_id = -1;
    if (getLastInsertId)
        insert_id = mysql_insert_id(&db);
    return insert_id;
}

void MySQLDatabase::shutdownDriver()
{
}

void MySQLDatabase::storeInternalSetting(const std::string& key, const std::string& value)
{
    std::string quotedValue = quote(value);
    std::ostringstream q;
    q << "INSERT INTO " << QTB << INTERNAL_SETTINGS_TABLE << QTE << " (`key`, `value`) "
                                                                    "VALUES ("
      << quote(key) << ", " << quotedValue << ") "
                                              "ON DUPLICATE KEY UPDATE `value` = "
      << quotedValue;
    SQLDatabase::exec(q.str());
}

void MySQLDatabase::_exec(const char* query, int length)
{
    if (mysql_real_query(&db, query, (length > 0 ? length : strlen(query)))) {
        std::string myError = getError(&db);
        throw DatabaseException(myError, fmt::format("Mysql: error while updating db: {}; query: {}", myError, query));
    }
}

/* MysqlResult */

MysqlResult::~MysqlResult()
{
    if (mysql_res) {
        if (!nullRead) {
            while ((mysql_fetch_row(mysql_res)))
                ; // read out data
        }
        mysql_free_result(mysql_res);
        mysql_res = nullptr;
    }
}

std::unique_ptr<SQLRow> MysqlResult::nextRow()
{
    if (auto mysql_row = mysql_fetch_row(mysql_res); mysql_row) {
        return std::make_unique<MysqlRow>(mysql_row);
    }
    nullRead = true;
    mysql_free_result(mysql_res);
    mysql_res = nullptr;
    return nullptr;
}

/* MysqlRow */

MysqlRow::MysqlRow(MYSQL_ROW mysql_row)
    : mysql_row(mysql_row)
{
}

#endif // HAVE_MYSQL
