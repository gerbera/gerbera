/*MT*
    
    MediaTomb - http://www.mediatomb.org/
    
    mysql_storage.cc - this file is part of MediaTomb.
    
    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.org>,
                       Sergey Bostandzhyan <jin@mediatomb.org>
    Copyright (C) 2006 Gena Batyan <bgeradz@mediatomb.org>,
                       Sergey Bostandzhyan <jin@mediatomb.org>,
                       Leonhard Wimmer <leo@mediatomb.org>
    
    MediaTomb is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
    
    MediaTomb is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    along with MediaTomb; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
    
    $Id$
*/

/// \file mysql_storage.cc

#ifdef HAVE_CONFIG_H
    #include "autoconfig.h"
#endif

#ifdef HAVE_MYSQL

//#define MYSQL_SELECT_DEBUG
//#define MYSQL_EXEC_DEBUG

#include "mysql_storage.h"
#include "config_manager.h"
#include "destroyer.h"

#ifdef AUTO_CREATE_DATABASE
    #include "mysql_create_sql.h"
    #include <zlib.h>
#endif

using namespace zmm;
using namespace mxml;

MysqlStorage::MysqlStorage() : SQLStorage()
{
    mysql_init_key_initialized = false;
    /// dbRemovesDeps gets set by init() to the correct value
    dbRemovesDeps = true;
    mysql_connection = false;
    mutex = Ref<Mutex> (new Mutex(true));
    table_quote_begin = '`';
    table_quote_end = '`';
}
MysqlStorage::~MysqlStorage()
{
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

void MysqlStorage::init()
{
    log_debug("start\n");
    AUTOLOCK(mutex);
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
    my_init();
    pthread_setspecific(mysql_init_key, (void *) 1);
    
    Ref<ConfigManager> config = ConfigManager::getInstance();
    
    String dbHost = config->getOption(_("/server/storage/host"));
    String dbName = config->getOption(_("/server/storage/database"));
    String dbUser = config->getOption(_("/server/storage/username"));
    String dbPort = config->getOption(_("/server/storage/port"));
    String dbPass;
    if (config->getElement(_("/server/storage/password")) == nil)
        dbPass = nil;
    else
        dbPass = config->getOption(_("/server/storage/password"), _(""));
    
    
    String dbSock;
    if (config->getElement(_("/server/storage/socket")) == nil)
        dbSock = nil;
    else
        dbSock = config->getOption(_("/server/storage/socket"), _(""));
    
    MYSQL *res_mysql;
    
    res_mysql = mysql_init(&db);
    if(! res_mysql)
    {
        throw _Exception(_("mysql_init failed"));
    }
    
    mysql_init_key_initialized = true;
    
    res_mysql = mysql_real_connect(&db,
        dbHost.c_str(),
        dbUser.c_str(),
        (dbPass == nil ? NULL : dbPass.c_str()),
        dbName.c_str(),
        dbPort.toInt(), // port
        (dbSock == nil ? NULL : dbSock.c_str()), // socket
        0 // flags
    );
    if(! res_mysql)
    {
        throw _Exception(_("The connection to the MySQL database has failed: ") + getError(&db));
    }
    
    #ifdef HAVE_MYSQL_OPT_RECONNECT
        my_bool my_bool_var = true;
        mysql_options(&db, MYSQL_OPT_RECONNECT, &my_bool_var);
    #endif
    
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
            throw _StorageException(_("Error while uncompressing mysql create sql. returned: ") + ret);
        buf[MS_CREATE_SQL_INFLATED_SIZE] = '\0';
        
        char *sql_start = (char *)buf;
        char *sql_end = index(sql_start, ';');
        if (sql_end == NULL)
        {
            throw _StorageException(_("';' not found in mysql create sql"));
        }
        do
        {
            ret = mysql_real_query(&db, sql_start, sql_end - sql_start);
            if (ret)
            {
                throw _StorageException(_("Mysql: error while creating db: ") + getError(&db));
            }
            sql_start = sql_end + 1; // skip ';'
            if (*sql_start == '\n')  // skip newline
                sql_start++;
            
            sql_end = index(sql_start, ';');
        }
        while(sql_end != NULL);
        dbVersion = getInternalSetting(_("db_version"));
        if (dbVersion == nil)
        {
            AUTOUNLOCK();
            shutdown();
            throw _Exception(_("error while creating database"));
        }
        log_info("database created successfully.\n");
#else
        AUTOUNLOCK();
        shutdown();
        throw _Exception(_("database doesn't seem to exist yet and autocreation wasn't compiled in"));
#endif
        
    }
    
    AUTOUNLOCK();
    
    SQLStorage::init();
    log_debug("end\n");
}

String MysqlStorage::quote(String value)
{
    // \todo replace with "_real_"
    char *q = (char *)MALLOC(value.length() * 2 + 2);
    *q = '\'';
    int size = mysql_escape_string(q + 1, value.c_str(), value.length());
    q[size + 1] = '\'';
    String ret(q, size + 2);
    FREE(q);
    return ret;
}

String MysqlStorage::getError(MYSQL *db)
{
    Ref<StringBuffer> err_buf(new StringBuffer());
    *err_buf << "mysql_error (" << String::from(mysql_errno(db));
    *err_buf << "): \"" << String(mysql_error(db)) << "\"";
    return err_buf->toString();
}

Ref<SQLResult> MysqlStorage::select(String query)
{
#ifdef MYSQL_SELECT_DEBUG
    Exception *e = new Exception(query);
    e->printStackTrace();
    delete e;
#endif
    
    int res;
    
    checkMysqlThreadInit();
    AUTOLOCK(mutex);
    res = mysql_real_query(&db, query.c_str(), query.length());
    if (res)
    {
        throw _StorageException(_("Mysql: mysql_real_query() failed: ") + getError(&db));
    }
    
    MYSQL_RES *mysql_res;
    mysql_res = mysql_store_result(&db);
    if(! mysql_res)
    {
        throw _StorageException(_("Mysql: mysql_store_result() failed: ") + getError(&db));
    }
    return Ref<SQLResult> (new MysqlResult(mysql_res));
    
}

int MysqlStorage::exec(String query, bool getLastInsertId)
{
#ifdef MYSQL_EXEC_DEBUG
    Exception *e = new Exception(query);
    e->printStackTrace();
    delete e;
#endif

    int res;
    
    checkMysqlThreadInit();
    AUTOLOCK(mutex);
    res = mysql_real_query(&db, query.c_str(), query.length());
    if(res)
    {
        throw _StorageException(_("Mysql: mysql_real_query() failed: ") + getError(&db));
    }
    int insert_id=-1;
    if (getLastInsertId) insert_id = mysql_insert_id(&db);
    return insert_id;
    
}

void MysqlStorage::shutdown()
{
    AUTOLOCK(mutex);    // just to ensure, that we don't close while another thread 
                    // is executing a query
    
    if(mysql_connection)
    {
        mysql_close(&db);
        mysql_connection = false;
    }
    mysql_server_end();
}

void MysqlStorage::storeInternalSetting(String key, String value)
{
    String quotedValue = quote(value);
    Ref<StringBuffer> q(new StringBuffer());
    *q << "INSERT INTO " << QTB << INTERNAL_SETTINGS_TABLE << QTE << " (`key`, `value`) "
    "VALUES (" << quote(key) << ", "<< quotedValue << ") "
    "ON DUPLICATE KEY UPDATE `value` = " << quotedValue;
    this->exec(q->toString());
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

String MysqlRow::col(int index)
{
    return String(mysql_row[index]);
}


#endif // HAVE_MYSQL

