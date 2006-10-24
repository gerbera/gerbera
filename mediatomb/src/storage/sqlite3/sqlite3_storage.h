/*  sqlite3_storage.h - this file is part of MediaTomb.

    Copyright (C) 2005 Gena Batyan <bgeradz@deadlock.dhs.org>,
                       Sergey Bostandzhyan <jin@deadlock.dhs.org>

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
*/

///\file sqlite3_storage.h
///\brief Definitions of the Sqlite3Storage, Sqlite3Result, Sqlite3Row and SLTask classes.

#ifdef HAVE_SQLITE3

#ifndef __SQLITE3_STORAGE_H__
#define __SQLITE3_STORAGE_H__

#include <sqlite3.h>

#include "storage/sql_storage.h"
#include "sync.h"

class Sqlite3Storage;
class Sqlite3Result;

/// \brief A virtual class that represents a task to be done by the sqlite3 thread.
class SLTask : public zmm::Object
{
public:
    /// \brief Instantiate a task
    SLTask();
    
    /// \brief run the sqlite3 task
    /// \param sl The instance of Sqlite3Storage to do the queries with.
    virtual void run(sqlite3 *db, Sqlite3Storage *sl) = 0;
    
    /// \brief returns true if the task is not completed
    /// \return true if the task is not completed yet, false if the task is finished and the results are ready.
    bool is_running();
    
    /// \brief modify the creator of the task using the supplied pthread_mutex and pthread_cond, that the task is finished
    void sendSignal();
    
    void sendSignal(zmm::String error);
    
    void waitForTask();
    
    zmm::String getError() { return error; }
    
protected:
    /// \brief true as long as the task is not finished
    ///
    /// The value is set by the constructor to true and then to false be sendSignal()
    bool running;
    zmm::Ref<Cond> cond;
    zmm::Ref<Mutex> mutex;
    zmm::String error;
};

/// \brief A task for the sqlite3 thread to do the initial checks.
class SLInitTask : public SLTask
{
public:
    /// \brief Constructor for the sqlite3 init task
    SLInitTask() : SLTask() {}
    virtual void run(sqlite3 *db, Sqlite3Storage *sl);
};


/// \brief A task for the sqlite3 thread to do a SQL select.
class SLSelectTask : public SLTask
{
public:
    /// \brief Constructor for the sqlite3 select task
    /// \param query The SQL query string
    SLSelectTask(zmm::String query);
    virtual void run(sqlite3 *db, Sqlite3Storage *sl);
    inline zmm::Ref<SQLResult> getResult() { return RefCast(pres, SQLResult); };
protected:
    /// \brief The SQL query string
    zmm::String query;
    /// \brief The Sqlite3Result
    zmm::Ref<Sqlite3Result> pres;
};

/// \brief A task for the sqlite3 thread to do a SQL exec.
class SLExecTask : public SLTask
{
public:
    /// \brief Constructor for the sqlite3 exec task
    /// \param query The SQL query string
    SLExecTask(zmm::String query, bool getLastInsertId);
    virtual void run(sqlite3 *db, Sqlite3Storage *sl);
    inline int getLastInsertId() { return lastInsertId; }
protected:
    /// \brief The SQL query string
    zmm::String query;
    int lastInsertId;
    bool getLastInsertIdFlag;
};

/// \brief Represents a row of a result of a sqlite3 select
class Sqlite3Row : public SQLRow
{
public:
    Sqlite3Row(char **row, zmm::Ref<SQLResult> sqlResult);
    virtual zmm::String col(int index);
protected:
    char **row;
    zmm::Ref<Sqlite3Result> res;
    friend class Sqlite3Result;
};

/// \brief Represents a result of a sqlite3 select
class Sqlite3Result : public SQLResult
{
public:
    Sqlite3Result();
    virtual ~Sqlite3Result();
    virtual zmm::Ref<SQLRow> nextRow();
    virtual unsigned long long getNumRows() { return nrow; }
protected:
    char **table;
    char **row;

    int cur_row;

    int nrow;
    int ncolumn;

    friend class Sqlite3Storage;
    friend class SLSelectTask;
};



/// \brief The Storage class for using SQLite3
class Sqlite3Storage : public SQLStorage
{
public:
    Sqlite3Storage();
    virtual ~Sqlite3Storage() {}
    
    virtual void init();
    virtual zmm::String quote(zmm::String str);
    virtual zmm::Ref<SQLResult> select(zmm::String query);
    virtual int exec(zmm::String query, bool getLastInsertId = false);
    virtual void shutdown();
    virtual void storeInternalSetting(zmm::String key, zmm::String value);
protected:
    zmm::String startupError;
    
    void reportError(zmm::String query, sqlite3 *db);
    
    static void *staticThreadProc(void *arg);
    void threadProc();
    
    void addTask(zmm::Ref<SLTask> task);
    
    pthread_t sqliteThread;
    zmm::Ref<Cond> cond;
    zmm::Ref<Mutex> mutex;
    
    /// \brief is set to true by shutdown() if the sqlite3 thread should terminate
    bool shutdownFlag;
    
    /// \brief the tasks to be done by the sqlite3 thread
    zmm::Ref<zmm::ObjectQueue<SLTask> > taskQueue;
    bool taskQueueOpen;
    
    inline void lock() { mutex->lock(); }
    inline void unlock() { mutex->unlock(); }
    inline void signal() { cond->signal(); }
    
    friend class SLSelectTask;
    friend class SLExecTask;
    friend class SLInitTask;
};


#endif // __SQLITE3_STORAGE_H__

#endif // HAVE_SQLITE3
