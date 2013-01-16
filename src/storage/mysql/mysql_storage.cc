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

#ifdef HAVE_CONFIG_H
    #include "autoconfig.h"
#endif

#ifdef HAVE_MYSQL

//#define MYSQL_SET_NAMES "/*!40101 SET NAMES utf8 */"

//#define MYSQL_SELECT_DEBUG
//#define MYSQL_EXEC_DEBUG

#include "mysql_storage.h"
#include "config_manager.h"

#ifdef AUTO_CREATE_DATABASE
    #include "mysql_create_sql.h"
    #include <zlib.h>
#endif

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


using namespace zmm;
using namespace mxml;

MysqlStorage::MysqlStorage() : SQLStorage()
{
    mysql_init_key_initialized = false;
    mysql_connection = false;
    mysqlMutex = Ref<Mutex> (new Mutex(true));
    table_quote_begin = '`';
    table_quote_end = '`';
    insertBuffer = nil;
}
MysqlStorage::~MysqlStorage()
{
    AUTOLOCK(mysqlMutex);    // just to ensure, that we don't close while another thread 
                    // is executing a query
    
    if(mysql_connection)
    {
        mysql_close(&db);
        mysql_connection = false;
    }
    log_debug("calling mysql_server_end...\n");
    mysql_server_end();
    log_debug("...ok\n");
}

void MysqlStorage::checkMysqlThreadInit()
{
    if (! mysql_connection)
        throw _Exception(_("mysql connection is not open or already closed"));
    //log_debug("checkMysqlThreadInit; thread_id=%d\n", pthread_self());
    if (pthread_getspecific(mysql_init_key) == NULL)
    {
        log_debug("running mysql_thread_init(); thread_id=%d\n", pthread_self());
        if (mysql_thread_init()) throw _Exception(_("error while calling mysql_thread_init()"));
        if (pthread_setspecific(mysql_init_key, (void *) 1)) throw _Exception(_("error while calling pthread_setspecific()"));
    }
}

void MysqlStorage::threadCleanup()
{
    log_debug("thread cleanup; thread_id=%d\n", pthread_self());
    if (pthread_getspecific(mysql_init_key) != NULL)
    {
        mysql_thread_end();
    }
}

void MysqlStorage::init()
{
    log_debug("start\n");
    SQLStorage::init();
    
    AUTOLOCK(mysqlMutex);
    int ret;
    
    if (! mysql_thread_safe())
    {
        throw _Exception(_("mysql library is not thread safe!"));
    }
    
    /// \todo write destructor function
    ret = pthread_key_create(&mysql_init_key, NULL);
    if (ret)
    {
        throw _Exception(_("could not create pthread_key"));
    }
    mysql_server_init(0, NULL, NULL);
    pthread_setspecific(mysql_init_key, (void *) 1);
    
    Ref<ConfigManager> config = ConfigManager::getInstance();
    
    String dbHost = config->getOption(CFG_SERVER_STORAGE_MYSQL_HOST);
    String dbName = config->getOption(CFG_SERVER_STORAGE_MYSQL_DATABASE);
    String dbUser = config->getOption(CFG_SERVER_STORAGE_MYSQL_USERNAME);
    int dbPort = config->getIntOption(CFG_SERVER_STORAGE_MYSQL_PORT);
    String dbPass = config->getOption(CFG_SERVER_STORAGE_MYSQL_PASSWORD);
    String dbSock = config->getOption(CFG_SERVER_STORAGE_MYSQL_SOCKET);
    
    MYSQL *res_mysql;
    
    res_mysql = mysql_init(&db);
    if(! res_mysql)
    {
        throw _Exception(_("mysql_init failed"));
    }
    
    mysql_init_key_initialized = true;
    
    mysql_options(&db, MYSQL_SET_CHARSET_NAME, "utf8");
    
    #ifdef HAVE_MYSQL_OPT_RECONNECT
        my_bool my_bool_var = true;
        mysql_options(&db, MYSQL_OPT_RECONNECT, &my_bool_var);
    #endif
    
    res_mysql = mysql_real_connect(&db,
        dbHost.c_str(),
        dbUser.c_str(),
        (dbPass == nil ? NULL : dbPass.c_str()),
        dbName.c_str(),
        dbPort, // port
        (dbSock == nil ? NULL : dbSock.c_str()), // socket
        0 // flags
    );
    if(! res_mysql)
    {
        throw _Exception(_("The connection to the MySQL database has failed: ") + getError(&db));
    }
    
    /*
    int res = mysql_real_query(&db, MYSQL_SET_NAMES, strlen(MYSQL_SET_NAMES));
    if(res)
    {
        String myError = getError(&db);
        throw _StorageException(nil, _("MySQL query 'SET NAMES' failed!"));
    }
    */
    
    
    mysql_connection = true;
    
    String dbVersion = nil;
    try
    {
        dbVersion = getInternalSetting(_("db_version"));
    }
    catch (Exception)
    {
    }
    
    if (dbVersion == nil)
    {
#ifdef AUTO_CREATE_DATABASE
        log_info("database doesn't seem to exist. automatically creating database...\n");
        unsigned char buf[MS_CREATE_SQL_INFLATED_SIZE + 1]; // + 1 for '\0' at the end of the string
        unsigned long uncompressed_size = MS_CREATE_SQL_INFLATED_SIZE;
        int ret = uncompress(buf, &uncompressed_size, mysql_create_sql, MS_CREATE_SQL_DEFLATED_SIZE);
        if (ret != Z_OK || uncompressed_size != MS_CREATE_SQL_INFLATED_SIZE)
            throw _Exception(_("Error while uncompressing mysql create sql. returned: ") + ret);
        buf[MS_CREATE_SQL_INFLATED_SIZE] = '\0';
        
        char *sql_start = (char *)buf;
        char *sql_end = strchr(sql_start, ';');
        if (sql_end == NULL)
        {
            throw _Exception(_("';' not found in mysql create sql"));
        }
        do
        {
            ret = mysql_real_query(&db, sql_start, sql_end - sql_start);
            if (ret)
            {
                String myError = getError(&db);
                throw _StorageException(myError, _("Mysql: error while creating db: ") + myError);
            }
            sql_start = sql_end + 1; // skip ';'
            if (*sql_start == '\n')  // skip newline
                sql_start++;
            
            sql_end = strchr(sql_start, ';');
        }
        while(sql_end != NULL);
        dbVersion = getInternalSetting(_("db_version"));
        if (dbVersion == nil)
        {
            shutdown();
            throw _Exception(_("error while creating database"));
        }
        log_info("database created successfully.\n");
#else
        shutdown();
        throw _Exception(_("database doesn't seem to exist yet and autocreation wasn't compiled in"));
#endif
        
    }
    log_debug("db_version: %s\n", dbVersion.c_str());
    
    /* --- database upgrades --- */
    if (dbVersion == "1")
    {
        log_info("Doing an automatic database upgrade from database version 1 to version 2...\n");
        _exec(MYSQL_UPDATE_1_2_1);
        _exec(MYSQL_UPDATE_1_2_2);
        _exec(MYSQL_UPDATE_1_2_3);
        _exec(MYSQL_UPDATE_1_2_4);
        _exec(MYSQL_UPDATE_1_2_5);
        _exec(MYSQL_UPDATE_1_2_6);
        log_info("database upgrade successful.\n");
        dbVersion = _("2");
    }
    
    if (dbVersion == "2")
    {
        log_info("Doing an automatic database upgrade from database version 2 to version 3...\n");
        _exec(MYSQL_UPDATE_2_3_1);
        _exec(MYSQL_UPDATE_2_3_2);
        _exec(MYSQL_UPDATE_2_3_3);
        _exec(MYSQL_UPDATE_2_3_4);
        log_info("database upgrade successful.\n");
        dbVersion = _("3");
    }
    
    if (dbVersion == "3")
    {
        log_info("Doing an automatic database upgrade from database version 3 to version 4...\n");
        _exec(MYSQL_UPDATE_3_4_1);
        _exec(MYSQL_UPDATE_3_4_2);
        _exec(MYSQL_UPDATE_3_4_3);
        log_info("database upgrade successful.\n");
        dbVersion = _("4");
    }
    
    /* --- --- ---*/
    
    if (! string_ok(dbVersion) || dbVersion != "4")
        throw _Exception(_("The database seems to be from a newer version (database version ") + dbVersion + ")!");
    
    AUTOUNLOCK();
    
    log_debug("end\n");
    
    dbReady();
}

String MysqlStorage::quote(String value)
{
    /* note: mysql_real_escape_string returns a maximum of (length * 2 + 1)
     * chars; we need +1 for the first ' - the second ' will be written over
     * the \0; then the string won't be null-terminated, but that doesn't matter,
     * because we give the correct length to String()
     */
    char *q = (char *)MALLOC(value.length() * 2 + 2);
    *q = '\'';
    long size = mysql_real_escape_string(&db, q + 1, value.c_str(), value.length());
    q[size + 1] = '\'';
    String ret(q, size + 2);
    FREE(q);
    return ret;
}

String MysqlStorage::getError(MYSQL *db)
{
    Ref<StringBuffer> err_buf(new StringBuffer());
    *err_buf << "mysql_error (" << String::from(mysql_errno(db));
    *err_buf << "): \"" << mysql_error(db) << "\"";
    log_debug("%s\n", err_buf->c_str());
    return err_buf->toString();
}

Ref<SQLResult> MysqlStorage::select(const char *query, int length)
{
#ifdef MYSQL_SELECT_DEBUG
    log_debug("%s\n", query);
    print_backtrace();
#endif
    
    int res;
    
    checkMysqlThreadInit();
    AUTOLOCK(mysqlMutex);
    res = mysql_real_query(&db, query, length);
    if (res)
    {
        String myError = getError(&db);
        throw _StorageException(myError, _("Mysql: mysql_real_query() failed: ") + myError + "; query: " + query);
    }
    
    MYSQL_RES *mysql_res;
    mysql_res = mysql_store_result(&db);
    if(! mysql_res)
    {
        String myError = getError(&db);
        throw _StorageException(myError, _("Mysql: mysql_store_result() failed: ") + myError + "; query: " + query);
    }
    return Ref<SQLResult> (new MysqlResult(mysql_res));
}

int MysqlStorage::exec(const char *query, int length, bool getLastInsertId)
{
#ifdef MYSQL_EXEC_DEBUG
    log_debug("%s\n", query);
    print_backtrace();
#endif

    int res;
    
    checkMysqlThreadInit();
    AUTOLOCK(mysqlMutex);
    res = mysql_real_query(&db, query, length);
    if(res)
    {
        String myError = getError(&db);
        throw _StorageException(myError, _("Mysql: mysql_real_query() failed: ") + myError + "; query: " + query);
    }
    int insert_id=-1;
    if (getLastInsertId) insert_id = mysql_insert_id(&db);
    return insert_id;
    
}

void MysqlStorage::shutdownDriver()
{
}

void MysqlStorage::storeInternalSetting(String key, String value)
{
    String quotedValue = quote(value);
    Ref<StringBuffer> q(new StringBuffer());
    *q << "INSERT INTO " << QTB << INTERNAL_SETTINGS_TABLE << QTE << " (`key`, `value`) "
    "VALUES (" << quote(key) << ", "<< quotedValue << ") "
    "ON DUPLICATE KEY UPDATE `value` = " << quotedValue;
    SQLStorage::exec(q);
}

void MysqlStorage::_exec(const char *query, int length)
{
    if (mysql_real_query(&db, query, (length > 0 ? length : strlen(query))))
    {
        String myError = getError(&db);
        throw _StorageException(myError, _("Mysql: error while updating db: ") + myError);
    }
}

void MysqlStorage::_addToInsertBuffer(Ref<StringBuffer> query)
{
    if (insertBuffer == nil)
    {
        insertBuffer = Ref<Array<StringBase> >(new Array<StringBase>());
        insertBuffer->append(_("BEGIN"));
    }
    Ref<StringBase> sb (new StringBase(query->c_str()));
    insertBuffer->append(sb);
}

void MysqlStorage::_flushInsertBuffer()
{
    if (insertBuffer == nil)
        return;
    insertBuffer->append(_("COMMIT"));
    
    checkMysqlThreadInit();
    AUTOLOCK(mysqlMutex);
    for (int i=0; i < insertBuffer->size(); i++)
    {
        _exec(insertBuffer->get(i)->data, insertBuffer->get(i)->len);
    }
    AUTOUNLOCK();
    insertBuffer->clear();
    insertBuffer->append(_("BEGIN"));
}


/* MysqlResult */

MysqlResult::MysqlResult(MYSQL_RES *mysql_res) : SQLResult()
{
    this->mysql_res = mysql_res;
    nullRead = false;
}

MysqlResult::~MysqlResult()
{
    if(mysql_res)
    {
        if (! nullRead)
        {
            MYSQL_ROW mysql_row;
            while((mysql_row = mysql_fetch_row(mysql_res)) != NULL); // read out data
        }
        mysql_free_result(mysql_res);
        mysql_res = NULL;
    }
}

Ref<SQLRow> MysqlResult::nextRow()
{   
    MYSQL_ROW mysql_row;
    mysql_row = mysql_fetch_row(mysql_res);
    if(mysql_row)
    {
        return Ref<SQLRow>(new MysqlRow(mysql_row, Ref<SQLResult>(this)));
    }
    nullRead = true;
    mysql_free_result(mysql_res);
    mysql_res = NULL;
    return nil;
}

/* MysqlRow */

MysqlRow::MysqlRow(MYSQL_ROW mysql_row, Ref<SQLResult> sqlResult) : SQLRow(sqlResult)
{
    this->mysql_row = mysql_row;
}

#endif // HAVE_MYSQL
