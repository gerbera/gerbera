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

#include <cstdlib>

#include <netinet/in.h>

#include "config/config_manager.h"
#include "util/thread_runner.h"
#include "util/tools.h"

//#define MYSQL_SET_NAMES "/*!40101 SET NAMES utf8 */"
//#define MYSQL_SELECT_DEBUG
//#define MYSQL_EXEC_DEBUG

// updates 1->2
#define MYSQL_UPDATE_1_2_1 "ALTER TABLE `mt_cds_object` CHANGE `location` `location` BLOB NULL DEFAULT NULL"
#define MYSQL_UPDATE_1_2_2 "ALTER TABLE `mt_cds_object` CHANGE `metadata` `metadata` BLOB NULL DEFAULT NULL"
#define MYSQL_UPDATE_1_2_3 "ALTER TABLE `mt_cds_object` CHANGE `auxdata` `auxdata` BLOB NULL DEFAULT NULL"
#define MYSQL_UPDATE_1_2_4 "ALTER TABLE `mt_cds_object` CHANGE `resources` `resources` BLOB NULL DEFAULT NULL"
#define MYSQL_UPDATE_1_2_5 "ALTER TABLE `mt_autoscan` CHANGE `location` `location` BLOB NULL DEFAULT NULL"

// updates 2->3
#define MYSQL_UPDATE_2_3_1 "ALTER TABLE `mt_autoscan` CHANGE `scan_mode` `scan_mode` ENUM( 'timed', 'inotify' ) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL"
#define MYSQL_UPDATE_2_3_2 "ALTER TABLE `mt_autoscan` DROP INDEX `mt_autoscan_obj_id`, ADD UNIQUE `mt_autoscan_obj_id` ( `obj_id` )"
#define MYSQL_UPDATE_2_3_3 "ALTER TABLE `mt_autoscan` ADD `path_ids` BLOB AFTER `location`"

// updates 3->4
#define MYSQL_UPDATE_3_4_1 "ALTER TABLE `mt_cds_object` ADD `service_id` varchar(255) default NULL"
#define MYSQL_UPDATE_3_4_2 "ALTER TABLE `mt_cds_object` ADD KEY `cds_object_service_id` (`service_id`)"

// updates 4->5
#define MYSQL_UPDATE_4_5_1 "CREATE TABLE `mt_metadata` ( \
  `id` int(11) NOT NULL auto_increment, \
  `item_id` int(11) NOT NULL, \
  `property_name` varchar(255) NOT NULL, \
  `property_value` text NOT NULL, \
  PRIMARY KEY `id` (`id`), \
  KEY `metadata_item_id` (`item_id`), \
  CONSTRAINT `mt_metadata_idfk1` FOREIGN KEY (`item_id`) REFERENCES `mt_cds_object` (`id`) ON DELETE CASCADE ON UPDATE CASCADE \
) ENGINE=MyISAM CHARSET=utf8"

// updates 5->6: add config value table
#define MYSQL_UPDATE_5_6_1 "CREATE TABLE `grb_config_value` ( \
  `item` varchar(255) primary key, \
  `key` varchar(255) NOT NULL, \
  `item_value` varchar(255) NOT NULL, \
  `status` varchar(20) NOT NULL) \
  ENGINE=MyISAM CHARSET=utf8"
#define MYSQL_UPDATE_5_6_2 "CREATE INDEX grb_config_value_item ON grb_config_value(item)"

// updates 6->7
#define MYSQL_UPDATE_6_7_1 "DROP TABLE mt_cds_active_item;"

// updates 7->8: part_number
#define MYSQL_UPDATE_7_8_1 "ALTER TABLE `mt_cds_object` ADD `part_number` int(11) default NULL AFTER `flags`"
#define MYSQL_UPDATE_7_8_2 "ALTER TABLE `mt_cds_object` DROP KEY `cds_object_track_number`"
#define MYSQL_UPDATE_7_8_3 "ALTER TABLE `mt_cds_object` ADD KEY `cds_object_track_number` (`part_number`,`track_number`)"

// updates 8->9: bookmark_pos
#define MYSQL_UPDATE_8_9_1 "ALTER TABLE `mt_cds_object` ADD `bookmark_pos` int(11) unsigned NOT NULL default '0' AFTER `service_id`"

// updates 9->10: last_modified
#define MYSQL_UPDATE_9_10_1 "ALTER TABLE `mt_cds_object` ADD `last_modified` bigint(20) unsigned default NULL AFTER `bookmark_pos`"

#define MYSQL_UPDATE_VERSION "UPDATE `mt_internal_setting` SET `value`='{}' WHERE `key`='db_version' AND `value`='{}'"

static const auto dbUpdates = std::array<std::vector<const char*>, 9> { {
    { MYSQL_UPDATE_1_2_1, MYSQL_UPDATE_1_2_2, MYSQL_UPDATE_1_2_3, MYSQL_UPDATE_1_2_4, MYSQL_UPDATE_1_2_5 },
    { MYSQL_UPDATE_2_3_1, MYSQL_UPDATE_2_3_2, MYSQL_UPDATE_2_3_3 },
    { MYSQL_UPDATE_3_4_1, MYSQL_UPDATE_3_4_2 },
    { MYSQL_UPDATE_4_5_1 },
    { MYSQL_UPDATE_5_6_1, MYSQL_UPDATE_5_6_2 },
    { MYSQL_UPDATE_6_7_1 },
    { MYSQL_UPDATE_7_8_1, MYSQL_UPDATE_7_8_2, MYSQL_UPDATE_7_8_3 },
    { MYSQL_UPDATE_8_9_1 },
    { MYSQL_UPDATE_9_10_1 },
} };

MySQLDatabase::MySQLDatabase(std::shared_ptr<Config> config)
    : SQLDatabase(std::move(config))
    , mysql_connection(false)
    , mysql_init_key_initialized(false)
{
    table_quote_begin = '`';
    table_quote_end = '`';
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
    if (pthread_getspecific(mysql_init_key) == nullptr) {
        if (mysql_thread_init())
            throw_std_runtime_error("error while calling mysql_thread_init()");
        if (pthread_setspecific(mysql_init_key, reinterpret_cast<void*>(1)))
            throw_std_runtime_error("error while calling pthread_setspecific()");
    }
}

void MySQLDatabase::threadCleanup()
{
    if (pthread_getspecific(mysql_init_key) != nullptr) {
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

        dbVersion = getInternalSetting("db_version");
        if (dbVersion.empty()) {
            shutdown();
            throw_std_runtime_error("error while creating database");
        }
        log_info("Database created successfully!");
    }
    log_debug("db_version: {}", dbVersion.c_str());

    /* --- database upgrades --- */
    int version = 1;
    for (auto&& upgrade : dbUpdates) {
        if (dbVersion == fmt::to_string(version)) {
            log_info("Running an automatic database upgrade from database version {} to version {}...", version, version + 1);
            for (auto&& upgradeCmd : upgrade) {
                _exec(upgradeCmd);
            }
            _exec(fmt::format(MYSQL_UPDATE_VERSION, version + 1, version).c_str());
            dbVersion = fmt::to_string(version + 1);
            log_info("Database upgrade to version {} successful.", dbVersion.c_str());
        }
        version++;
    }

    if (dbVersion != fmt::to_string(version))
        throw_std_runtime_error("The database seems to be from a newer version (database version {})", dbVersion);

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

std::shared_ptr<SQLResult> MySQLDatabase::select(const char* query, int length)
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

    return std::static_pointer_cast<SQLResult>(std::make_shared<MysqlResult>(mysql_res));
}

int MySQLDatabase::exec(const char* query, int length, bool getLastInsertId)
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

MysqlResult::MysqlResult(MYSQL_RES* mysql_res)
    : nullRead(false)
    , mysql_res(mysql_res)
{
}

MysqlResult::~MysqlResult()
{
    if (mysql_res) {
        if (!nullRead) {
            while ((mysql_fetch_row(mysql_res)) != nullptr)
                ; // read out data
        }
        mysql_free_result(mysql_res);
        mysql_res = nullptr;
    }
}

std::unique_ptr<SQLRow> MysqlResult::nextRow()
{
    if (auto mysql_row = mysql_fetch_row(mysql_res); mysql_row != nullptr) {
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
