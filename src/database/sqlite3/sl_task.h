/*GRB*
    Gerbera - https://gerbera.io/

    sl_task.h - this file is part of Gerbera.

    Copyright (C) 2022 Gerbera Contributors

    Gerbera is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.

    Gerbera is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Gerbera.  If not, see <http://www.gnu.org/licenses/>.
*/

/// \file sl_task.h
///\brief Definitions of the SLTask classes.

#ifndef __SQLITE3_TASK_H__
#define __SQLITE3_TASK_H__

#include <condition_variable>
#include <mutex>

#include <sqlite3.h>

#include "util/grb_fs.h"

class Config;
class Sqlite3Database;
class Sqlite3Result;

#define SQLITE3_BACKUP_FORMAT "{}.backup"
#define SQLITE3_SET_VERSION "INSERT INTO \"mt_internal_setting\" VALUES('db_version', '{}')"

/// \brief A virtual class that represents a task to be done by the sqlite3 thread.
class SLTask {
public:
    virtual ~SLTask() = default;

    /// \brief run the sqlite3 task
    /// \param sl The instance of Sqlite3Database to do the queries with.
    virtual void run(sqlite3*& db, Sqlite3Database* sl, bool throwOnError = true) = 0;

    /// \brief returns true if the task is not completed
    /// \return true if the task is not completed yet, false if the task is finished and the results are ready.
    bool is_running() const;

    /// \brief notify the creator of the task using the supplied pthread_mutex and pthread_cond, that the task is finished
    void sendSignal();

    void sendSignal(std::string error);

    void waitForTask();

    bool didContamination() const { return contamination; }
    bool didDecontamination() const { return decontamination; }

    std::string getError() const { return error; }

    virtual std::string_view taskType() const = 0;

    virtual bool checkKey(const std::string& key) const { return true; }

protected:
    /// \brief true as long as the task is not finished
    ///
    /// The value is set by the constructor to true and then to false be sendSignal()
    bool running { true };

    /// \brief true if this task has changed the db (in comparison to the backup)
    bool contamination {};

    /// \brief true if this task has backuped the db
    bool decontamination {};

    std::condition_variable cond;
    mutable std::mutex mutex;

    std::string error;
};

/// \brief A task for the sqlite3 thread to inititally create the database.
class SLInitTask : public SLTask {
public:
    /// \brief Constructor for the sqlite3 init task
    explicit SLInitTask(std::shared_ptr<Config> config, unsigned int hashie);
    void run(sqlite3*& db, Sqlite3Database* sl, bool throwOnError = true) override;

    std::string_view taskType() const override { return "InitTask"; }

protected:
    std::shared_ptr<Config> config;
    unsigned int hashie;
};

/// \brief A task for the sqlite3 thread to do a SQL select.
class SLSelectTask : public SLTask {
public:
    /// \brief Constructor for the sqlite3 select task
    /// \param query The SQL query string
    explicit SLSelectTask(const std::string& query);
    void run(sqlite3*& db, Sqlite3Database* sl, bool throwOnError = true) override;
    [[nodiscard]] std::shared_ptr<Sqlite3Result> getResult() const { return pres; }

    std::string_view taskType() const override { return "SelectTask"; }

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
    SLExecTask(const std::string& query, bool getLastInsertId);
    SLExecTask(const std::string& query, const std::string& eKey);
    void run(sqlite3*& db, Sqlite3Database* sl, bool throwOnError = true) override;
    int getLastInsertId() const { return lastInsertId; }
    bool checkKey(const std::string& key) const override { return key != eKey; }

    std::string_view taskType() const override { return "ExecTask"; }

protected:
    /// \brief The SQL query string
    const char* query;

    int lastInsertId {};
    bool getLastInsertIdFlag;
    std::string eKey;
};

/// \brief A task for the sqlite3 thread to do a SQL exec.
class SLBackupTask : public SLTask {
public:
    /// \brief Constructor for the sqlite3 backup task
    SLBackupTask(std::shared_ptr<Config> config, bool restore);
    void run(sqlite3*& db, Sqlite3Database* sl, bool throwOnError = true) override;

    std::string_view taskType() const override { return "BackupTask"; }

protected:
    std::shared_ptr<Config> config;
    bool restore;
    GrbFile dbFile;
    GrbFile dbBackupFile;
};

#endif // __SQLITE3_TASK_H__
