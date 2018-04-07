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
#include <sqlite3.h>
#include <sstream>

#include "storage/sql_storage.h"
#include "timer.h"

class Sqlite3Storage;
class Sqlite3Result;

/// \brief A virtual class that represents a task to be done by the sqlite3 thread.
class SLTask : public zmm::Object {
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

    void sendSignal(zmm::String error);

    void waitForTask();

    bool didContamination() { return contamination; }
    bool didDecontamination() { return decontamination; }

    zmm::String getError() { return error; }

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

    zmm::String error;
};

/// \brief A task for the sqlite3 thread to inititally create the database.
class SLInitTask : public SLTask {
public:
    /// \brief Constructor for the sqlite3 init task
    SLInitTask()
        : SLTask()
    {
    }
    virtual void run(sqlite3** db, Sqlite3Storage* sl);
};

/// \brief A task for the sqlite3 thread to do a SQL select.
class SLSelectTask : public SLTask {
public:
    /// \brief Constructor for the sqlite3 select task
    /// \param query The SQL query string
    SLSelectTask(const char* query);
    virtual void run(sqlite3** db, Sqlite3Storage* sl);
    inline zmm::Ref<SQLResult> getResult() { return RefCast(pres, SQLResult); };

protected:
    /// \brief The SQL query string
    const char* query;
    /// \brief The Sqlite3Result
    zmm::Ref<Sqlite3Result> pres;
};

/// \brief A task for the sqlite3 thread to do a SQL exec.
class SLExecTask : public SLTask {
public:
    /// \brief Constructor for the sqlite3 exec task
    /// \param query The SQL query string
    SLExecTask(const char* query, bool getLastInsertId);
    virtual void run(sqlite3** db, Sqlite3Storage* sl);
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
    SLBackupTask(bool restore) { this->restore = restore; };
    virtual void run(sqlite3** db, Sqlite3Storage* sl);

protected:
    bool restore;
};

/// \brief The Storage class for using SQLite3
class Sqlite3Storage : public Timer::Subscriber, private SQLStorage {
public:
    virtual void timerNotify(zmm::Ref<Timer::Parameter> sqlite3storage) override;

private:
    Sqlite3Storage();
    friend zmm::Ref<Storage> Storage::createInstance();
    virtual void init() override;
    virtual void shutdownDriver() override;

    virtual zmm::String quote(zmm::String str) override;
    virtual inline zmm::String quote(int val) override { return zmm::String::from(val); }
    virtual inline zmm::String quote(unsigned int val) override { return zmm::String::from(val); }
    virtual inline zmm::String quote(long val) override { return zmm::String::from(val); }
    virtual inline zmm::String quote(unsigned long val) override { return zmm::String::from(val); }
    virtual inline zmm::String quote(bool val) override { return zmm::String(val ? '1' : '0'); }
    virtual inline zmm::String quote(char val) override { return quote(zmm::String(val)); }
    virtual inline zmm::String quote(long long val) override { return zmm::String::from(val); }
    virtual zmm::Ref<SQLResult> select(const char* query, int length) override;
    virtual int exec(const char* query, int length, bool getLastInsertId = false) override;
    virtual void storeInternalSetting(zmm::String key, zmm::String value) override;

    void _exec(const char* query);

    zmm::String startupError;

    zmm::String getError(zmm::String query, zmm::String error, sqlite3* db);

    static void* staticThreadProc(void* arg);
    void threadProc();

    void addTask(zmm::Ref<SLTask> task, bool onlyIfDirty = false);

    pthread_t sqliteThread;
    std::condition_variable cond;
    std::mutex sqliteMutex;
    using AutoLock = std::lock_guard<decltype(sqliteMutex)>;
    using AutoLockU = std::unique_lock<decltype(sqliteMutex)>;

    /// \brief is set to true by shutdown() if the sqlite3 thread should terminate
    bool shutdownFlag;

    /// \brief the tasks to be done by the sqlite3 thread
    zmm::Ref<zmm::ObjectQueue<SLTask>> taskQueue;
    bool taskQueueOpen;

    virtual void threadCleanup() override {}
    virtual bool threadCleanupRequired() override { return false; }

    std::ostringstream insertBuffer;
    void _addToInsertBuffer(const std::string &query) override;
    virtual void _flushInsertBuffer() override;

    bool dirty;

    friend class SLSelectTask;
    friend class SLExecTask;
    friend class SLInitTask;
    friend class Sqlite3BackupTimerSubscriber;
};

/// \brief Represents a result of a sqlite3 select
class Sqlite3Result : public SQLResult {
private:
    Sqlite3Result();
    virtual ~Sqlite3Result();
    virtual zmm::Ref<SQLRow> nextRow() override;
    virtual unsigned long long getNumRows() override { return nrow; }

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
private:
    Sqlite3Row(char** row, zmm::Ref<SQLResult> sqlResult);
    inline virtual char* col_c_str(int index) { return row[index]; }
    char** row;
    zmm::Ref<Sqlite3Result> res;

    friend class Sqlite3Result;
};

#endif // __SQLITE3_STORAGE_H__

#endif // HAVE_SQLITE3
