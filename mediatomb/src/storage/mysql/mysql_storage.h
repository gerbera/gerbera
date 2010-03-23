/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    mysql_storage.h - this file is part of MediaTomb.
    
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

/// \file mysql_storage.h

#ifdef HAVE_MYSQL

#ifndef __MYSQL_STORAGE_H__
#define __MYSQL_STORAGE_H__

#include "common.h"
#include "storage/sql_storage.h"
#include "sync.h"
#include <mysql.h>

class MysqlStorage : private SQLStorage
{
private:
    MysqlStorage();
    friend zmm::Ref<Storage> Storage::createInstance();
    virtual ~MysqlStorage();
    virtual void init();
    virtual void shutdownDriver();
    
    virtual zmm::String quote(zmm::String str);
    virtual inline zmm::String quote(int val) { return zmm::String::from(val); }
    virtual inline zmm::String quote(unsigned int val) { return zmm::String::from(val); }
    virtual inline zmm::String quote(long val) { return zmm::String::from(val); }
    virtual inline zmm::String quote(unsigned long val) { return zmm::String::from(val); }
    virtual inline zmm::String quote(bool val) { return zmm::String(val ? '1' : '0'); }
    virtual inline zmm::String quote(char val) { return quote(zmm::String(val)); }
    virtual zmm::Ref<SQLResult> select(const char *query, int length);
    virtual int exec(const char *query, int length, bool getLastInsertId = false);
    virtual void storeInternalSetting(zmm::String key, zmm::String value);
    
    void _exec(const char *query, int lenth = -1);
    
    MYSQL db;
    
    bool mysql_connection;
    
    zmm::String getError(MYSQL *db);
    
    zmm::Ref<Mutex> mysqlMutex;
    
    virtual void threadCleanup();
    virtual bool threadCleanupRequired() { return true; }
    
    pthread_key_t mysql_init_key;
    bool mysql_init_key_initialized;
    
    void checkMysqlThreadInit();
    
    zmm::Ref<zmm::Array<zmm::StringBase> > insertBuffer;
    virtual void _addToInsertBuffer(zmm::Ref<zmm::StringBuffer> query);
    virtual void _flushInsertBuffer();
};

class MysqlResult : private SQLResult
{
private:
    int nullRead;
    MysqlResult(MYSQL_RES *mysql_res);
    virtual ~MysqlResult();
    virtual zmm::Ref<SQLRow> nextRow();
    virtual unsigned long long getNumRows() { return mysql_num_rows(mysql_res); }
    MYSQL_RES *mysql_res;
    
    friend class MysqlRow;
    friend class MysqlStorage;
};

class MysqlRow : private SQLRow
{
private:
    MysqlRow(MYSQL_ROW mysql_row, zmm::Ref<SQLResult> sqlResult);
    inline virtual char* col_c_str(int index) { return mysql_row[index]; }
    
    MYSQL_ROW mysql_row;
    
    friend zmm::Ref<SQLRow> MysqlResult::nextRow();
};

#endif // __MYSQL_STORAGE_H__

#endif // HAVE_MYSQL
