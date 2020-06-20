/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    mysql_storage.cc - this file is part of MediaTomb.
    
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

/// \file mysql_storage.cc

#ifdef HAVE_MYSQL
#include "mysql_storage.h"

#include <cstdlib>
#include <utility>

#include <zlib.h>

#include "config/config_manager.h"
#include "mysql_create_sql.h"

//#define MYSQL_SET_NAMES "/*!40101 SET NAMES utf8 */"
//#define MYSQL_SELECT_DEBUG
//#define MYSQL_EXEC_DEBUG

// updates 1->2
#define MYSQL_UPDATE_1_2_1 "ALTER TABLE `mt_cds_object` CHANGE `location` `location` BLOB NULL DEFAULT NULL"
#define MYSQL_UPDATE_1_2_2 "ALTER TABLE `mt_cds_object` CHANGE `metadata` `metadata` BLOB NULL DEFAULT NULL"
#define MYSQL_UPDATE_1_2_3 "ALTER TABLE `mt_cds_object` CHANGE `auxdata` `auxdata` BLOB NULL DEFAULT NULL"
#define MYSQL_UPDATE_1_2_4 "ALTER TABLE `mt_cds_object` CHANGE `resources` `resources` BLOB NULL DEFAULT NULL"
#define MYSQL_UPDATE_1_2_5 "ALTER TABLE `mt_autoscan` CHANGE `location` `location` BLOB NULL DEFAULT NULL"
#define MYSQL_UPDATE_1_2_6 "UPDATE `mt_internal_setting` SET `value`='2' WHERE `key`='db_version'"

// updates 2->3
#define MYSQL_UPDATE_2_3_1 "ALTER TABLE `mt_autoscan` CHANGE `scan_mode` `scan_mode` ENUM( 'timed', 'inotify' ) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL"
#define MYSQL_UPDATE_2_3_2 "ALTER TABLE `mt_autoscan` DROP INDEX `mt_autoscan_obj_id`, ADD UNIQUE `mt_autoscan_obj_id` ( `obj_id` )"
#define MYSQL_UPDATE_2_3_3 "ALTER TABLE `mt_autoscan` ADD `path_ids` BLOB AFTER `location`"
#define MYSQL_UPDATE_2_3_4 "UPDATE `mt_internal_setting` SET `value`='3' WHERE `key`='db_version' AND `value`='2'"

// updates 3->4
#define MYSQL_UPDATE_3_4_1 "ALTER TABLE `mt_cds_object` ADD `service_id` varchar(255) default NULL"
#define MYSQL_UPDATE_3_4_2 "ALTER TABLE `mt_cds_object` ADD KEY `cds_object_service_id` (`service_id`)"
#define MYSQL_UPDATE_3_4_3 "UPDATE `mt_internal_setting` SET `value`='4' WHERE `key`='db_version' AND `value`='3'"

#define MYSQL_UPDATE_4_5_1 "CREATE TABLE `mt_metadata` ( \
  `id` int(11) NOT NULL auto_increment, \
  `item_id` int(11) NOT NULL, \
  `property_name` varchar(255) NOT NULL, \
  `property_value` text NOT NULL, \
  PRIMARY KEY `id` (`id`), \
  KEY `metadata_item_id` (`item_id`), \
  CONSTRAINT `mt_metadata_idfk1` FOREIGN KEY (`item_id`) REFERENCES `mt_cds_object` (`id`) ON DELETE CASCADE ON UPDATE CASCADE \
) ENGINE=MyISAM CHARSET=utf8"
#define MYSQL_UPDATE_4_5_2 "UPDATE `mt_internal_setting` SET `value`='5' WHERE `key`='db_version' AND `value`='4'"

MysqlStorage::MysqlStorage(std::shared_ptr<Config> config)
    : SQLStorage(std::move(config))
{
    mysql_init_key_initialized = false;
    mysql_connection = false;
    table_quote_begin = '`';
    table_quote_end = '`';
}
MysqlStorage::~MysqlStorage()
{
    AutoLock lock(mysqlMutex); // just to ensure, that we don't close while another thread
    // is executing a query

    if (mysql_connection) {
        mysql_close(&db);
        mysql_connection = false;
    }
    log_debug("calling mysql_server_end...");
    mysql_server_end();
    log_debug("...ok");
}

void MysqlStorage::checkMysqlThreadInit() const
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

void MysqlStorage::threadCleanup()
{
    if (pthread_getspecific(mysql_init_key) != nullptr) {
        mysql_thread_end();
    }
}

void MysqlStorage::init()
{
    log_debug("start");
    SQLStorage::init();

    std::unique_lock<decltype(mysqlMutex)> lock(mysqlMutex);
    int ret;

    if (!mysql_thread_safe()) {
        throw_std_runtime_error("mysql library is not thread safe");
    }

    /// \todo write destructor function
    ret = pthread_key_create(&mysql_init_key, nullptr);
    if (ret) {
        throw_std_runtime_error("could not create pthread_key");
    }
    mysql_server_init(0, nullptr, nullptr);
    pthread_setspecific(mysql_init_key, reinterpret_cast<void*>(1));

    std::string dbHost = config->getOption(CFG_SERVER_STORAGE_MYSQL_HOST);
    std::string dbName = config->getOption(CFG_SERVER_STORAGE_MYSQL_DATABASE);
    std::string dbUser = config->getOption(CFG_SERVER_STORAGE_MYSQL_USERNAME);
    int dbPort = config->getIntOption(CFG_SERVER_STORAGE_MYSQL_PORT);
    std::string dbPass = config->getOption(CFG_SERVER_STORAGE_MYSQL_PASSWORD);
    std::string dbSock = config->getOption(CFG_SERVER_STORAGE_MYSQL_SOCKET);

    MYSQL* res_mysql;

    res_mysql = mysql_init(&db);
    if (!res_mysql) {
        throw_std_runtime_error("mysql_init failed");
    }

    mysql_init_key_initialized = true;

    mysql_options(&db, MYSQL_SET_CHARSET_NAME, "utf8");

    bool my_bool_var = true;
    mysql_options(&db, MYSQL_OPT_RECONNECT, &my_bool_var);

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
        throw_std_runtime_error("The connection to the MySQL database has failed: " + getError(&db));
    }

    /*
    int res = mysql_real_query(&db, MYSQL_SET_NAMES, strlen(MYSQL_SET_NAMES));
    if(res)
    {
        std::string myError = getError(&db);
        throw StorageException("", "MySQL query 'SET NAMES' failed!");
    }
    */

    mysql_connection = true;

    std::string dbVersion;
    try {
        dbVersion = getInternalSetting("db_version");
    } catch (const std::runtime_error& e) {
        log_debug("{}", e.what());
    }

    if (dbVersion.empty()) {
        log_info("database doesn't seem to exist. automatically creating database...");
        unsigned char buf[MS_CREATE_SQL_INFLATED_SIZE + 1]; // + 1 for '\0' at the end of the string
        unsigned long uncompressed_size = MS_CREATE_SQL_INFLATED_SIZE;
        int ret = uncompress(buf, &uncompressed_size, mysql_create_sql, MS_CREATE_SQL_DEFLATED_SIZE);
        if (ret != Z_OK || uncompressed_size != MS_CREATE_SQL_INFLATED_SIZE)
            throw_std_runtime_error("Error while uncompressing mysql create sql. returned: " + std::to_string(ret));
        buf[MS_CREATE_SQL_INFLATED_SIZE] = '\0';

        auto sql_start = reinterpret_cast<char*>(buf);
        auto sql_end = std::strchr(sql_start, ';');
        if (sql_end == nullptr) {
            throw_std_runtime_error("';' not found in mysql create sql");
        }
        do {
            ret = mysql_real_query(&db, sql_start, sql_end - sql_start);
            if (ret) {
                std::string myError = getError(&db);
                throw StorageException(myError, "Mysql: error while creating db: " + myError);
            }
            sql_start = sql_end + 1; // skip ';'
            if (*sql_start == '\n') // skip newline
                sql_start++;

            sql_end = std::strchr(sql_start, ';');
        } while (sql_end != nullptr);
        dbVersion = getInternalSetting("db_version");
        if (dbVersion.empty()) {
            shutdown();
            throw_std_runtime_error("error while creating database");
        }
        log_info("database created successfully.");
    }
    log_debug("db_version: {}", dbVersion.c_str());

    /* --- database upgrades --- */
    if (dbVersion == "1") {
        log_info("Doing an automatic database upgrade from database version 1 to version 2...");
        _exec(MYSQL_UPDATE_1_2_1);
        _exec(MYSQL_UPDATE_1_2_2);
        _exec(MYSQL_UPDATE_1_2_3);
        _exec(MYSQL_UPDATE_1_2_4);
        _exec(MYSQL_UPDATE_1_2_5);
        _exec(MYSQL_UPDATE_1_2_6);
        log_info("database upgrade successful.");
        dbVersion = "2";
    }

    if (dbVersion == "2") {
        log_info("Doing an automatic database upgrade from database version 2 to version 3...");
        _exec(MYSQL_UPDATE_2_3_1);
        _exec(MYSQL_UPDATE_2_3_2);
        _exec(MYSQL_UPDATE_2_3_3);
        _exec(MYSQL_UPDATE_2_3_4);
        log_info("database upgrade successful.");
        dbVersion = "3";
    }

    if (dbVersion == "3") {
        log_info("Doing an automatic database upgrade from database version 3 to version 4...");
        _exec(MYSQL_UPDATE_3_4_1);
        _exec(MYSQL_UPDATE_3_4_2);
        _exec(MYSQL_UPDATE_3_4_3);
        log_info("database upgrade successful.");
        dbVersion = "4";
    }

    if (dbVersion == "4") {
        log_info("Doing an automatic database upgrade from database version 4 to version 5...");
        _exec(MYSQL_UPDATE_4_5_1);
        _exec(MYSQL_UPDATE_4_5_2);
        log_info("database upgrade successful.");
        dbVersion = "5";
    }

    /* --- --- ---*/

    if (dbVersion != "5")
        throw_std_runtime_error("The database seems to be from a newer version (database version " + dbVersion + ")");

    lock.unlock();

    log_debug("end");

    dbReady();
}

std::shared_ptr<Storage> MysqlStorage::getSelf()
{
    return shared_from_this();
}

std::string MysqlStorage::quote(std::string value) const
{
    /* note: mysql_real_escape_string returns a maximum of (length * 2 + 1)
     * chars; we need +1 for the first ' - the second ' will be written over
     * the \0; then the string won't be null-terminated, but that doesn't matter,
     * because we give the correct length to std::string()
     */
    auto q = static_cast<char*>(malloc(value.length() * 2 + 2));
    *q = '\'';
    long size = mysql_real_escape_string(const_cast<MYSQL*>(&db), q + 1, value.c_str(), value.length());
    q[size + 1] = '\'';
    std::string ret(q, size + 2);
    free(q);
    return ret;
}

std::string MysqlStorage::getError(MYSQL* db)
{
    std::ostringstream err_buf;
    err_buf << "mysql_error (" << mysql_errno(db);
    err_buf << "): \"" << mysql_error(db) << "\"";
    log_debug("{}", err_buf.str().c_str());
    return err_buf.str();
}

std::shared_ptr<SQLResult> MysqlStorage::select(const char* query, int length)
{
#ifdef MYSQL_SELECT_DEBUG
    log_debug("{}", query);
    print_backtrace();
#endif

    int res;

    checkMysqlThreadInit();
    AutoLock lock(mysqlMutex);
    res = mysql_real_query(&db, query, length);
    if (res) {
        std::string myError = getError(&db);
        throw StorageException(myError, "Mysql: mysql_real_query() failed: " + myError + "; query: " + query);
    }

    MYSQL_RES* mysql_res;
    mysql_res = mysql_store_result(&db);
    if (!mysql_res) {
        std::string myError = getError(&db);
        throw StorageException(myError, "Mysql: mysql_store_result() failed: " + myError + "; query: " + query);
    }

    return std::static_pointer_cast<SQLResult>(std::make_shared<MysqlResult>(mysql_res));
}

int MysqlStorage::exec(const char* query, int length, bool getLastInsertId)
{
#ifdef MYSQL_EXEC_DEBUG
    log_debug("{}", query);
    print_backtrace();
#endif

    int res;

    checkMysqlThreadInit();
    AutoLock lock(mysqlMutex);
    res = mysql_real_query(&db, query, length);
    if (res) {
        std::string myError = getError(&db);
        throw StorageException(myError, "Mysql: mysql_real_query() failed: " + myError + "; query: " + query);
    }
    int insert_id = -1;
    if (getLastInsertId)
        insert_id = mysql_insert_id(&db);
    return insert_id;
}

void MysqlStorage::shutdownDriver()
{
}

void MysqlStorage::storeInternalSetting(const std::string& key, const std::string& value)
{
    std::string quotedValue = quote(value);
    std::ostringstream q;
    q << "INSERT INTO " << QTB << INTERNAL_SETTINGS_TABLE << QTE << " (`key`, `value`) "
                                                                    "VALUES ("
      << quote(key) << ", " << quotedValue << ") "
                                              "ON DUPLICATE KEY UPDATE `value` = "
      << quotedValue;
    SQLStorage::exec(q);
}

void MysqlStorage::_exec(const char* query, int length)
{
    if (mysql_real_query(&db, query, (length > 0 ? length : strlen(query)))) {
        std::string myError = getError(&db);
        throw StorageException(myError, "Mysql: error while updating db: " + myError);
    }
}

/* MysqlResult */

MysqlResult::MysqlResult(MYSQL_RES* mysql_res)
{
    this->mysql_res = mysql_res;
    nullRead = false;
}

MysqlResult::~MysqlResult()
{
    if (mysql_res) {
        if (!nullRead) {
            MYSQL_ROW mysql_row;
            while ((mysql_row = mysql_fetch_row(mysql_res)) != nullptr)
                ; // read out data
        }
        mysql_free_result(mysql_res);
        mysql_res = nullptr;
    }
}

std::unique_ptr<SQLRow> MysqlResult::nextRow()
{
    MYSQL_ROW mysql_row;
    mysql_row = mysql_fetch_row(mysql_res);
    if (mysql_row) {
        auto p = std::make_unique<MysqlRow>(mysql_row);
        return p;
    }
    nullRead = true;
    mysql_free_result(mysql_res);
    mysql_res = nullptr;
    return nullptr;
}

/* MysqlRow */

MysqlRow::MysqlRow(MYSQL_ROW mysql_row)
{
    this->mysql_row = mysql_row;
}

#endif // HAVE_MYSQL
