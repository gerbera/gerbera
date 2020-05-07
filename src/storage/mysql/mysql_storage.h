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
#include <mutex>
#include <mysql.h>
#include <string>
#include <vector>

class MysqlStorage : public SQLStorage, public std::enable_shared_from_this<SQLStorage> {
public:
    explicit MysqlStorage(std::shared_ptr<Config> config);
    ~MysqlStorage() override;

private:
    void init() override;
    void shutdownDriver() override;
    std::shared_ptr<Storage> getSelf() override;

    std::string quote(std::string value) const override;
    inline std::string quote(const char* str) const override { return quote(std::string(str)); }
    inline std::string quote(int val) const override { return std::to_string(val); }
    inline std::string quote(unsigned int val) const override { return std::to_string(val); }
    inline std::string quote(long val) const override { return std::to_string(val); }
    inline std::string quote(unsigned long val) const override { return std::to_string(val); }
    inline std::string quote(bool val) const override { return std::to_string(val ? '1' : '0'); }
    inline std::string quote(char val) const override { return quote(std::to_string(val)); }
    inline std::string quote(long long val) const override { return std::to_string(val); }
    std::shared_ptr<SQLResult> select(const char* query, int length) override;
    int exec(const char* query, int length, bool getLastInsertId = false) override;
    void storeInternalSetting(const std::string& key, const std::string& value) override;

    void _exec(const char* query, int length = -1);

    MYSQL db;

    bool mysql_connection;

    static std::string getError(MYSQL* db);

    std::recursive_mutex mysqlMutex;
    using AutoLock = std::lock_guard<decltype(mysqlMutex)>;

    void threadCleanup() override;
    bool threadCleanupRequired() const override { return true; }

    pthread_key_t mysql_init_key;
    bool mysql_init_key_initialized;

    void checkMysqlThreadInit() const;
};

class MysqlResult : public SQLResult {
public:
    explicit MysqlResult(MYSQL_RES* mysql_res);
    ~MysqlResult() override;

private:
    int nullRead;
    std::unique_ptr<SQLRow> nextRow() override;
    unsigned long long getNumRows() const override { return mysql_num_rows(mysql_res); }
    MYSQL_RES* mysql_res;

    friend class MysqlRow;
    friend class MysqlStorage;
};

class MysqlRow : public SQLRow {
public:
    explicit MysqlRow(MYSQL_ROW mysql_row);

private:
    inline char* col_c_str(int index) const override { return mysql_row[index]; }

    MYSQL_ROW mysql_row;

    friend std::unique_ptr<SQLRow> MysqlResult::nextRow();
};

#endif // __MYSQL_STORAGE_H__

#endif // HAVE_MYSQL
