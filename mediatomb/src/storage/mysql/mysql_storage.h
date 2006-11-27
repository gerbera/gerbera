/*MT*
    
    MediaTomb - http://www.mediatomb.org/
    
    mysql_storage.h - this file is part of MediaTomb.
    
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

/// \file mysql_storage.h

#ifdef HAVE_MYSQL

#ifndef __MYSQL_STORAGE_H__
#define __MYSQL_STORAGE_H__

#include "common.h"
#include "storage/sql_storage.h"
#include "sync.h"
#include <mysql.h>


class MysqlRow : public SQLRow
{
public:
    MysqlRow(MYSQL_ROW mysql_row, zmm::Ref<SQLResult> sqlResult);
    virtual zmm::String col(int index);
protected:
    MYSQL_ROW mysql_row;
};

class MysqlResult : public SQLResult
{
    int nullRead;
public:
    MysqlResult(MYSQL_RES *mysql_res);
    virtual ~MysqlResult();
    virtual zmm::Ref<SQLRow> nextRow();
    virtual unsigned long long getNumRows() { return mysql_num_rows(mysql_res); }
protected:
    MYSQL_RES *mysql_res;
};

class MysqlStorage : public SQLStorage
{
public:
    MysqlStorage();
    virtual ~MysqlStorage();

    virtual void init();
    virtual zmm::String quote(zmm::String str);
    virtual inline zmm::String quote(int val) { return zmm::String::from(val); }
    virtual inline zmm::String quote(unsigned int val) { return zmm::String::from(val); }
    virtual inline zmm::String quote(long val) { return zmm::String::from(val); }
    virtual inline zmm::String quote(unsigned long val) { return zmm::String::from(val); }
    virtual zmm::Ref<SQLResult> select(zmm::String query);
    virtual int exec(zmm::String query, bool getLastInsertId = false);
    virtual void shutdown();
    virtual void storeInternalSetting(zmm::String key, zmm::String value);
    
protected:
    
    MYSQL db;
    
    bool mysql_connection;
    
    zmm::String getError(MYSQL *db);
    
    zmm::Ref<Mutex> mutex;
    
    pthread_key_t mysql_init_key;
    bool mysql_init_key_initialized;
    
    void checkMysqlThreadInit();
};


#endif // __MYSQL_STORAGE_H__

#endif // HAVE_MYSQL
