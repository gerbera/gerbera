/*MT*

    MediaTomb - http://www.mediatomb.cc/

    sqlite_database.h - this file is part of MediaTomb.

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

/// \file sqlite_database.h
/// \brief Definitions of the Sqlite3Database, Sqlite3Result and Sqlite3Row classes.

#ifndef __SQLITE3_STORAGE_H__
#define __SQLITE3_STORAGE_H__

#include "database/sql_database.h"
#include "util/thread_runner.h"
#include "util/timer.h"

#include <mutex>
#include <queue>
#include <sqlite3.h>

class Sqlite3Database;
class Sqlite3Result;
class SLTask;

#define DELETE_CACHE_MAX_SIZE 500 // remove entries, if cache has more than 500 (default)

/// \brief The Database class for using SQLite3
class Sqlite3Database : public Timer::Subscriber, public SQLDatabase, public std::enable_shared_from_this<SQLDatabase> {
public:
    Sqlite3Database(const std::shared_ptr<Config>& config, const std::shared_ptr<Mime>& mime, const std::shared_ptr<ConverterManager>& converterManager, std::shared_ptr<Timer> timer);

    std::string handleError(const std::string& query, const std::string& error, sqlite3* db, int errorCode);
    void timerNotify(const std::shared_ptr<Timer::Parameter>& param) override;

protected:
    void _exec(const std::string& query) override;
    std::string prepareDatabase(const fs::path& dbFilePath, GrbFile& dbFile);

private:
    void prepare();
    void init() override;
    void shutdownDriver() override;
    std::shared_ptr<Database> getSelf() override;

    std::string quote(const std::string& value) const override;

    std::shared_ptr<SQLResult> select(const std::string& query) override;
    void del(std::string_view tableName, const std::string& clause, const std::vector<int>& ids) override;
    void exec(std::string_view tableName, const std::string& query, int objId) override;
    int exec(const std::string& query, bool getLastInsertId = false) override;
    void execOnly(const std::string& query) override;

    /// \brief Implement common behaviour on exceptions while calling the database
    void handleException(const std::runtime_error& exc, const std::string& lineMessage);

    void storeInternalSetting(const std::string& key, const std::string& value) override;

    std::string startupError;

    std::unique_ptr<StdThreadRunner> threadRunner;
    void threadProc();

    void addTask(const std::shared_ptr<SLTask>& task, bool onlyIfDirty = false);

    std::shared_ptr<Timer> timer;

    /// \brief increased by shutdown attempt if the sqlite3 thread should terminate
    int shutdownFlag { 0 };

    /// \brief the tasks to be done by the sqlite3 thread
    std::queue<std::shared_ptr<SLTask>> taskQueue;
    bool taskQueueOpen {};

    mutable std::mutex del_mutex;
    using DelAutoLock = std::scoped_lock<std::mutex>;
    mutable std::vector<std::string> deletedEntries {};
    size_t maxDeleteCount { DELETE_CACHE_MAX_SIZE };
    std::chrono::seconds lastDelete;

    void threadCleanup() override { }
    bool threadCleanupRequired() const override { return false; }

    bool dirty {};
    bool dbInitDone {};
    bool hasBackupTimer {};
    int sqliteStatus {};
    /// \brief maximum number of attempts to terminate gracefully
    int shutdownAttempts { 5 };
};

/// \brief The Database class for using SQLite3 with transactions
class Sqlite3DatabaseWithTransactions : public SqlWithTransactions, public Sqlite3Database {
public:
    Sqlite3DatabaseWithTransactions(const std::shared_ptr<Config>& config, const std::shared_ptr<Mime>& mime, const std::shared_ptr<ConverterManager>& converterManager, const std::shared_ptr<Timer>& timer);

    void beginTransaction(std::string_view tName) override;
    void rollback(std::string_view tName) override;
    void commit(std::string_view tName) override;
};

/// \brief Represents a result of a sqlite3 select
class Sqlite3Result : public SQLResult {
public:
    Sqlite3Result() = default;
    ~Sqlite3Result() override;

    Sqlite3Result(const Sqlite3Result&) = delete;
    Sqlite3Result& operator=(const Sqlite3Result&) = delete;

private:
    std::unique_ptr<SQLRow> nextRow() override;
    [[nodiscard]] unsigned long long getNumRows() const override { return nrow; }

    char** table { nullptr };
    char** row;

    int cur_row;

    int nrow;
    int ncolumn;

    friend class SLSelectTask;
    friend class Sqlite3Row;
};

/// \brief Represents a row of a result of a sqlite3 select
class Sqlite3Row : public SQLRow {
public:
    explicit Sqlite3Row(char** row);

private:
    char* col_c_str(int index) const override;
    char** row;
};

#endif // __SQLITE3_STORAGE_H__
