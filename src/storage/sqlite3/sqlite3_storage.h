/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    sqlite3_storage.h - this file is part of MediaTomb.
    
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

/// \file sqlite3_storage.h
///\brief Definitions of the Sqlite3Storage, Sqlite3Result, Sqlite3Row and SLTask classes.

#ifdef HAVE_SQLITE3

#ifndef __SQLITE3_STORAGE_H__
#define __SQLITE3_STORAGE_H__

#include <condition_variable>
#include <mutex>
#include <queue>
#include <sqlite3.h>
#include <sstream>

#include "storage/sql_storage.h"
#include "util/timer.h"

class Sqlite3Storage;
class Sqlite3Result;

/// \brief A virtual class that represents a task to be done by the sqlite3 thread.
class SLTask {
public:
    /// \brief Instantiate a task
    SLTask();

    /// \brief run the sqlite3 task
    /// \param sl The instance of Sqlite3Storage to do the queries with.
    virtual void run(sqlite3** db, Sqlite3Storage* sl) = 0;

    /// \brief returns true if the task is not completed
    /// \return true if the task is not completed yet, false if the task is finished and the results are ready.
    bool is_running();

    /// \brief modify the creator of the task using the supplied pthread_mutex and pthread_cond, that the task is finished
    void sendSignal();

    void sendSignal(std::string error);

    void waitForTask();

    bool didContamination() { return contamination; }
    bool didDecontamination() { return decontamination; }

    std::string getError() { return error; }

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
    SLInitTask(std::shared_ptr<ConfigManager> config);
    void run(sqlite3** db, Sqlite3Storage* sl) override;

protected:
    std::shared_ptr<ConfigManager> config;
};

/// \brief A task for the sqlite3 thread to do a SQL select.
class SLSelectTask : public SLTask {
public:
    /// \brief Constructor for the sqlite3 select task
    /// \param query The SQL query string
    SLSelectTask(const char* query);
    void run(sqlite3** db, Sqlite3Storage* sl) override;
    inline std::shared_ptr<SQLResult> getResult() { return std::static_pointer_cast<SQLResult>(pres); }

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
    void run(sqlite3** db, Sqlite3Storage* sl) override;
    inline int getLastInsertId() { return lastInsertId; }

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
    SLBackupTask(std::shared_ptr<ConfigManager> config, bool restore);
    void run(sqlite3** db, Sqlite3Storage* sl) override;

protected:
    std::shared_ptr<ConfigManager> config;
    bool restore;
};

/// \brief The Storage class for using SQLite3
class Sqlite3Storage : public Timer::Subscriber, public SQLStorage, public std::enable_shared_from_this<SQLStorage> {
public:
    void timerNotify(std::shared_ptr<Timer::Parameter> param) override;
    Sqlite3Storage(std::shared_ptr<ConfigManager> config, std::shared_ptr<Timer> timer);

private:
    void init() override;
    void shutdownDriver() override;
    std::shared_ptr<Storage> getSelf() override;

    std::string quote(std::string value) override;
    inline std::string quote(const char* str) override { return quote(std::string(str)); }
    inline std::string quote(int val) override { return std::to_string(val); }
    inline std::string quote(unsigned int val) override { return std::to_string(val); }
    inline std::string quote(long val) override { return std::to_string(val); }
    inline std::string quote(unsigned long val) override { return std::to_string(val); }
    inline std::string quote(bool val) override { return std::string(val ? "1" : "0"); }
    inline std::string quote(char val) override { return quote(std::string(1, val)); }
    inline std::string quote(long long val) override { return std::to_string(val); }
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

    void threadCleanup() override {}
    bool threadCleanupRequired() override { return false; }

    bool dirty;

    friend class SLSelectTask;
    friend class SLExecTask;
    friend class SLInitTask;
    friend class Sqlite3BackupTimerSubscriber;
};

/// \brief Represents a result of a sqlite3 select
class Sqlite3Result : public SQLResult {
public:
    Sqlite3Result();
    virtual ~Sqlite3Result();

private:
    std::unique_ptr<SQLRow> nextRow() override;
    unsigned long long getNumRows() override { return nrow; }

    char** table;
    char** row;

    int cur_row;

    int nrow;
    int ncolumn;

    friend class SLSelectTask;
    friend class Sqlite3Row;
    friend class Sqlite3Storage;
};

/// \brief Represents a row of a result of a sqlite3 select
class Sqlite3Row : public SQLRow {
public:
    Sqlite3Row(char** row);

private:
    inline char* col_c_str(int index) override { return row[index]; }
    char** row;
};

#endif // __SQLITE3_STORAGE_H__

#endif // HAVE_SQLITE3
