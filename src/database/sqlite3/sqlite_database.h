/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    sqlite3_database.h - this file is part of MediaTomb.
    
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

/// \file sqlite3_database.h
///\brief Definitions of the Sqlite3Database, Sqlite3Result, Sqlite3Row and SLTask classes.

#ifndef __SQLITE3_STORAGE_H__
#define __SQLITE3_STORAGE_H__

#include <condition_variable>
#include <mutex>
#include <queue>
#include <sqlite3.h>
#include <sstream>
#include <unistd.h>

#include "database/sql_database.h"
#include "util/timer.h"

class Sqlite3Database;
class Sqlite3Result;

/// \brief A virtual class that represents a task to be done by the sqlite3 thread.
class SLTask {
public:
    /// \brief Instantiate a task
    SLTask();

    /// \brief run the sqlite3 task
    /// \param sl The instance of Sqlite3Database to do the queries with.
    virtual void run(sqlite3** db, Sqlite3Database* sl) = 0;

    /// \brief returns true if the task is not completed
    /// \return true if the task is not completed yet, false if the task is finished and the results are ready.
    bool is_running() const;

    /// \brief modify the creator of the task using the supplied pthread_mutex and pthread_cond, that the task is finished
    void sendSignal();

    void sendSignal(std::string error);

    void waitForTask();

    bool didContamination() const { return contamination; }
    bool didDecontamination() const { return decontamination; }

    std::string getError() const { return error; }

    virtual ~SLTask() = default;

protected:
    /// \brief true as long as the task is not finished
    ///
    /// The value is set by the constructor to true and then to false be sendSignal()
    bool running;

    /// \brief true if this task has changed the db (in comparison to the backup)
    bool contamination;

    /// \brief true if this task has backuped the db
    bool decontamination;

    std::condition_variable cond;
    std::mutex mutex;

    std::string error;
};

/// \brief A task for the sqlite3 thread to inititally create the database.
class SLInitTask : public SLTask {
public:
    /// \brief Constructor for the sqlite3 init task
    explicit SLInitTask(std::shared_ptr<Config> config);
    void run(sqlite3** db, Sqlite3Database* sl) override;

protected:
    std::shared_ptr<Config> config;
};

/// \brief A task for the sqlite3 thread to do a SQL select.
class SLSelectTask : public SLTask {
public:
    /// \brief Constructor for the sqlite3 select task
    /// \param query The SQL query string
    explicit SLSelectTask(const char* query);
    void run(sqlite3** db, Sqlite3Database* sl) override;
    [[nodiscard]] std::shared_ptr<SQLResult> getResult() const { return std::static_pointer_cast<SQLResult>(pres); }

protected:
    /// \brief The SQL query string
    const char* query;
    /// \brief The Sqlite3Result
    std::shared_ptr<Sqlite3Result> pres;
};

/// \brief A task for the sqlite3 thread to do a SQL exec.
class SLExecTask : public SLTask {
public:
    /// \brief Constructor for the sqlite3 exec task
    /// \param query The SQL query string
    SLExecTask(const char* query, bool getLastInsertId);
    void run(sqlite3** db, Sqlite3Database* sl) override;
    int getLastInsertId() const { return lastInsertId; }

protected:
    /// \brief The SQL query string
    const char* query;

    int lastInsertId;
    bool getLastInsertIdFlag;
};

/// \brief A task for the sqlite3 thread to do a SQL exec.
class SLBackupTask : public SLTask {
public:
    /// \brief Constructor for the sqlite3 backup task
    SLBackupTask(std::shared_ptr<Config> config, bool restore);
    void run(sqlite3** db, Sqlite3Database* sl) override;

protected:
    std::shared_ptr<Config> config;
    bool restore;
};

/// \brief The Database class for using SQLite3
class Sqlite3Database : public Timer::Subscriber, public SQLDatabase, public std::enable_shared_from_this<SQLDatabase> {
public:
    void timerNotify(std::shared_ptr<Timer::Parameter> param) override;
    Sqlite3Database(std::shared_ptr<Config> config, std::shared_ptr<Timer> timer);

private:
    void init() override;
    void shutdownDriver() override;
    std::shared_ptr<Database> getSelf() override;

    std::string quote(std::string value) const override;
    std::string quote(const char* str) const override { return quote(std::string(str)); }
    std::string quote(int val) const override { return fmt::to_string(val); }
    std::string quote(unsigned int val) const override { return fmt::to_string(val); }
    std::string quote(long val) const override { return fmt::to_string(val); }
    std::string quote(unsigned long val) const override { return fmt::to_string(val); }
    std::string quote(bool val) const override { return std::string(val ? "1" : "0"); }
    std::string quote(char val) const override { return quote(std::string(1, val)); }
    std::string quote(long long val) const override { return fmt::to_string(val); }
    std::shared_ptr<SQLResult> select(const char* query, int length) override;
    int exec(const char* query, int length, bool getLastInsertId = false) override;
    void storeInternalSetting(const std::string& key, const std::string& value) override;

    void _exec(const char* query);

    std::string startupError;

    static std::string getError(const std::string& query, const std::string& error, sqlite3* db);

    static void* staticThreadProc(void* arg);
    void threadProc();

    void addTask(const std::shared_ptr<SLTask>& task, bool onlyIfDirty = false);

    pthread_t sqliteThread;
    std::condition_variable cond;
    std::mutex sqliteMutex;
    using AutoLock = std::lock_guard<decltype(sqliteMutex)>;
    using AutoLockU = std::unique_lock<decltype(sqliteMutex)>;

    std::shared_ptr<Timer> timer;

    /// \brief is set to true by shutdown() if the sqlite3 thread should terminate
    bool shutdownFlag;

    /// \brief the tasks to be done by the sqlite3 thread
    std::queue<std::shared_ptr<SLTask>> taskQueue;
    bool taskQueueOpen;

    void threadCleanup() override { }
    bool threadCleanupRequired() const override { return false; }

    bool dirty;
    bool dbInitDone;

    friend class SLSelectTask;
    friend class SLExecTask;
    friend class SLInitTask;
    friend class Sqlite3BackupTimerSubscriber;
};

/// \brief Represents a result of a sqlite3 select
class Sqlite3Result : public SQLResult {
public:
    Sqlite3Result();
    ~Sqlite3Result() override;

private:
    std::unique_ptr<SQLRow> nextRow() override;
    [[nodiscard]] unsigned long long getNumRows() const override { return nrow; }

    char** table;
    char** row;

    int cur_row;

    int nrow;
    int ncolumn;

    friend class SLSelectTask;
    friend class Sqlite3Row;
    friend class Sqlite3Database;
};

/// \brief Represents a row of a result of a sqlite3 select
class Sqlite3Row : public SQLRow {
public:
    explicit Sqlite3Row(char** row);

private:
    char* col_c_str(int index) const override { return row[index]; }
    char** row;
};

#endif // __SQLITE3_STORAGE_H__
