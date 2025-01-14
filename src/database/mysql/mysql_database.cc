/*MT*

    MediaTomb - http://www.mediatomb.cc/

    mysql_database.cc - this file is part of MediaTomb.

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

/// \file mysql_database.cc
#define GRB_LOG_FAC GrbLogFacility::mysql

#ifdef HAVE_MYSQL
#include "mysql_database.h"

#include "config/config_val.h"
#include "exceptions.h"
#include "util/thread_runner.h"
#include "util/tools.h"

#include <netinet/in.h>

#define MYSQL_SET_VERSION "INSERT INTO `mt_internal_setting` VALUES ('db_version','{}')"
static constexpr auto mysqlUpdateVersion = std::string_view("UPDATE `mt_internal_setting` SET `value`='{}' WHERE `key`='db_version' AND `value`='{}'");
static constexpr auto mysqlAddResourceAttr = std::string_view("ALTER TABLE `grb_cds_resource` ADD COLUMN `{}` varchar(255) default NULL");

MySQLDatabase::MySQLDatabase(const std::shared_ptr<Config>& config, const std::shared_ptr<Mime>& mime, const std::shared_ptr<ConverterManager>& converterManager)
    : SQLDatabase(config, mime, converterManager)
{
    table_quote_begin = '`';
    table_quote_end = '`';

    // if mysql.sql or mysql-upgrade.xml is changed hashies have to be updated
    hashies = { 684441550, // index 0 is used for create script mysql.sql = Version 1
        928913698, 1984244483, 2241152998, 1748460509, 2860006966, 974692115, 70310290, 1863649106, 4238128129, 2979337694, // upgrade 2-11
        1512596496, 507706380, 3545156190, 31528140, 372163748, 4097073836, 751952276, 3893982139, 798767550, 3731206823, // upgrade 12-21
        3643149536, 4280737637 };
}

MySQLDatabase::~MySQLDatabase()
{
    SqlAutoLock lock(sqlMutex); // just to ensure, that we don't close while another thread
    // is executing a query

    if (mysql_connection) {
        mysql_close(&db);
        mysql_connection = false;
    }
    log_debug("calling mysql_library_end ...");
    mysql_library_end();
    log_debug("...ok");
}

void MySQLDatabase::checkMysqlThreadInit() const
{
    if (!mysql_connection)
        throw_std_runtime_error("mysql connection is not open or already closed");
    if (!pthread_getspecific(mysql_init_key)) {
        if (mysql_thread_init())
            throw_std_runtime_error("error while calling mysql_thread_init()");
        if (pthread_setspecific(mysql_init_key, &mysql_init_val))
            throw_std_runtime_error("error while calling pthread_setspecific()");
    }
}

void MySQLDatabase::threadCleanup()
{
    if (pthread_getspecific(mysql_init_key)) {
        mysql_thread_end();
    }
}

void MySQLDatabase::connect()
{
    if (!mysql_thread_safe()) {
        throw_std_runtime_error("mysql library is not thread safe");
    }

    // Desctructor function not necessary because no memory is allocated
    if (pthread_key_create(&mysql_init_key, nullptr)) {
        throw_std_runtime_error("could not create pthread_key");
    }
    mysql_library_init(0, nullptr, nullptr);
    pthread_setspecific(mysql_init_key, &mysql_init_val);

    {
        MYSQL* resMysql = mysql_init(&db);
        if (!resMysql) {
            throw_std_runtime_error("mysql_init() failed");
        }

        mysql_options(&db, MYSQL_SET_CHARSET_NAME, "utf8mb4");

        bool myBoolVar = true;
        mysql_options(&db, MYSQL_OPT_RECONNECT, &myBoolVar);
    }

    {
        std::string dbHost = config->getOption(ConfigVal::SERVER_STORAGE_MYSQL_HOST);
        std::string dbName = config->getOption(ConfigVal::SERVER_STORAGE_MYSQL_DATABASE);
        std::string dbUser = config->getOption(ConfigVal::SERVER_STORAGE_MYSQL_USERNAME);
        auto dbPort = in_port_t(config->getIntOption(ConfigVal::SERVER_STORAGE_MYSQL_PORT));
        std::string dbPass = config->getOption(ConfigVal::SERVER_STORAGE_MYSQL_PASSWORD);
        std::string dbSock = config->getOption(ConfigVal::SERVER_STORAGE_MYSQL_SOCKET);

        MYSQL* resMysql = mysql_real_connect(&db,
            dbHost.c_str(),
            dbUser.c_str(),
            (dbPass.empty() ? nullptr : dbPass.c_str()),
            dbName.c_str(),
            dbPort, // port
            (dbSock.empty() ? nullptr : dbSock.c_str()), // socket
            0 // flags
        );
        if (!resMysql) {
            throw_std_runtime_error("Connecting to database {}:{}/{} failed: {}", dbHost, dbPort, dbName, getError(&db));
        }
    }

    mysql_connection = true;
}

std::string MySQLDatabase::prepareDatabase()
{
    std::string dbVersion;
    try {
        dbVersion = getInternalSetting("db_version");
    } catch (const std::runtime_error& e) {
        log_debug("{}", e.what());
    }

    if (dbVersion.empty()) {
        log_info("Database doesn't seem to exist. Creating database...");
        auto sqlFilePath = fs::path(config->getOption(ConfigVal::SERVER_STORAGE_MYSQL_INIT_SQL_FILE));
        log_debug("Loading initialisation SQL from: {}", sqlFilePath.c_str());
        auto sql = GrbFile(std::move(sqlFilePath)).readTextFile();
        auto&& myHash = stringHash(sql);

        if (myHash == hashies[0]) {
            for (auto&& statement : splitString(sql, ';')) {
                trimStringInPlace(statement);
                if (statement.empty()) {
                    continue;
                }
                log_debug("executing statement: '{}'", statement);
                int ret = mysql_real_query(&db, statement.c_str(), statement.size());
                if (ret) {
                    std::string myError = getError(&db);
                    throw DatabaseException(myError, fmt::format("Mysql: error while creating db: {}", myError));
                }
            }
            _exec(fmt::format(MYSQL_SET_VERSION, DBVERSION));
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
    return dbVersion;
}

void MySQLDatabase::init()
{
    log_debug("start");
    SQLDatabase::init();

    std::unique_lock<decltype(sqlMutex)> lock(sqlMutex);

    connect();
    auto dbVersion = prepareDatabase();
    upgradeDatabase(std::stoul(dbVersion), hashies, ConfigVal::SERVER_STORAGE_MYSQL_UPGRADE_FILE, mysqlUpdateVersion, mysqlAddResourceAttr);
    initDynContainers();

    lock.unlock();

    log_debug("end");
}

std::shared_ptr<Database> MySQLDatabase::getSelf()
{
    return shared_from_this();
}

std::string MySQLDatabase::quote(const std::string& value) const
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
    auto ret = std::string(q, size + 2);
    delete[] q;
    return ret;
}

std::string MySQLDatabase::getError(MYSQL* db)
{
    auto res = fmt::format("mysql_error ({}): \"{}\"", mysql_errno(db), mysql_error(db));
    log_debug("{}", res);
    return res;
}

MySQLDatabaseWithTransactions::MySQLDatabaseWithTransactions(const std::shared_ptr<Config>& config, const std::shared_ptr<Mime>& mime, const std::shared_ptr<ConverterManager>& converterManager)
    : SqlWithTransactions(config)
    , MySQLDatabase(config, mime, converterManager)
{
}

void MySQLDatabaseWithTransactions::beginTransaction(std::string_view tName)
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

void MySQLDatabaseWithTransactions::rollback(std::string_view tName)
{
    log_debug("ROLLBACK {}", tName);
    if (use_transaction && inTransaction && mysql_rollback(&db)) {
        std::string myError = getError(&db);
        throw DatabaseException(myError, fmt::format("Mysql: error while rolling back db: {}", myError));
    }
    inTransaction = false;
}

void MySQLDatabaseWithTransactions::commit(std::string_view tName)
{
    log_debug("COMMIT {}", tName);
    SqlAutoLock lock(sqlMutex);
    if (use_transaction && inTransaction && mysql_commit(&db)) {
        std::string myError = getError(&db);
        throw DatabaseException(myError, fmt::format("Mysql: error while committing db: {}", myError));
    }
    inTransaction = false;
}

std::shared_ptr<SQLResult> MySQLDatabaseWithTransactions::select(const std::string& query)
{
    log_debug("{}", query);

    checkMysqlThreadInit();
    SqlAutoLock lock(sqlMutex);
    bool myTransaction = false;
    if (!inTransaction) { // protect calls outside transactions
        inTransaction = true;
        myTransaction = true;
    }
    auto res = mysql_real_query(&db, query.c_str(), query.size());
    if (res) {
        std::string myError = getError(&db);
        rollback("");
        throw DatabaseException(myError, fmt::format("Mysql: mysql_real_query() failed: {}; query: {}", myError, query));
    }

    MYSQL_RES* mysqlRes = mysql_store_result(&db);
    if (!mysqlRes && mysql_field_count(&db)) {
        std::string myError = getError(&db);
        rollback("");
        throw DatabaseException(myError, fmt::format("Mysql: mysql_store_result() failed: {}; query: {}", myError, query));
    }
    if (myTransaction) {
        inTransaction = false;
    }

    return std::make_shared<MysqlResult>(mysqlRes);
}

std::shared_ptr<SQLResult> MySQLDatabase::select(const std::string& query)
{
    log_debug("{}", query);

    checkMysqlThreadInit();
    SqlAutoLock lock(sqlMutex);
    auto res = mysql_real_query(&db, query.c_str(), query.size());
    if (res) {
        std::string myError = getError(&db);
        throw DatabaseException(myError, fmt::format("Mysql: mysql_real_query() failed: {}; query: {}", myError, query));
    }

    MYSQL_RES* mysqlRes = mysql_store_result(&db);
    if (!mysqlRes && mysql_field_count(&db)) {
        std::string myError = getError(&db);
        throw DatabaseException(myError, fmt::format("Mysql: mysql_store_result() failed: {}; query: {}", myError, query));
    }

    return std::make_shared<MysqlResult>(mysqlRes);
}

void MySQLDatabase::del(std::string_view tableName, const std::string& clause, const std::vector<int>& ids)
{
    auto query = clause.empty() //
        ? fmt::format("DELETE FROM {}", identifier(std::string(tableName))) //
        : fmt::format("DELETE FROM {} WHERE {}", identifier(std::string(tableName)), clause);
    log_debug("{}", query);

    checkMysqlThreadInit();
    SqlAutoLock lock(sqlMutex);
    auto res = mysql_real_query(&db, query.c_str(), query.size());
    if (res) {
        std::string myError = getError(&db);
        rollback("");
        throw DatabaseException(myError, fmt::format("Mysql: mysql_real_query() failed: {}; query: {}", myError, query));
    }
}

void MySQLDatabase::exec(std::string_view tableName, const std::string& query, int objId)
{
    log_debug("{}", query);

    checkMysqlThreadInit();
    SqlAutoLock lock(sqlMutex);
    auto res = mysql_real_query(&db, query.c_str(), query.size());
    if (res) {
        std::string myError = getError(&db);
        rollback("");
        throw DatabaseException(myError, fmt::format("Mysql: mysql_real_query() failed: {}; query: {}", myError, query));
    }
}

int MySQLDatabase::exec(const std::string& query, bool getLastInsertId)
{
    log_debug("{}", query);

    checkMysqlThreadInit();
    SqlAutoLock lock(sqlMutex);
    auto res = mysql_real_query(&db, query.c_str(), query.size());
    if (res) {
        std::string myError = getError(&db);
        rollback("");
        throw DatabaseException(myError, fmt::format("Mysql: mysql_real_query() failed: {}; query: {}", myError, query));
    }
    int insertId = -1;
    if (getLastInsertId)
        insertId = mysql_insert_id(&db);
    return insertId;
}

void MySQLDatabase::execOnly(const std::string& query)
{
    log_debug("{}", query);

    checkMysqlThreadInit();
    SqlAutoLock lock(sqlMutex);
    auto res = mysql_real_query(&db, query.c_str(), query.size());
    if (res) {
        std::string myError = getError(&db);
        log_error("{}\n{}", myError, fmt::format("Mysql: mysql_real_query() failed: {}; query: {}", myError, query));
    }
}

void MySQLDatabase::shutdownDriver()
{
}

void MySQLDatabase::storeInternalSetting(const std::string& key, const std::string& value)
{
    exec(fmt::format("INSERT INTO `{0}` (`key`, `value`) VALUES ({1}, {2}) ON DUPLICATE KEY UPDATE `value` = {2}",
        INTERNAL_SETTINGS_TABLE, quote(key), quote(value)));
}

void MySQLDatabase::_exec(const std::string& query)
{
    if (mysql_real_query(&db, query.c_str(), query.size())) {
        std::string myError = getError(&db);
        throw DatabaseException(myError, fmt::format("Mysql: error while updating db: {}; query: {}", myError, query));
    }
}

/* MysqlResult */

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
