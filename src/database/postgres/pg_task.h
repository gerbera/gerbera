/*GRB*
    Gerbera - https://gerbera.io/

    pg_task.h - this file is part of Gerbera.

    Copyright (C) 2025 Gerbera Contributors

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

/// @file database/postgres/pg_task.h
/// @brief Definitions of the PGTask classes.

#ifndef __POSTGRES_TASK_H__
#define __POSTGRES_TASK_H__

#include "util/grb_fs.h"

#include <condition_variable>
#include <mutex>
#include <pqxx/pqxx>

class Config;
class PostgresDatabase;
class PostgresSQLResult;

#define POSTGRES_SET_VERSION "INSERT INTO \"mt_internal_setting\" VALUES('db_version', '{}')"

/// @brief A virtual class that represents a task to be done by the postgres thread.
class PGTask {
public:
    PGTask(bool throwOnError = true)
        : throwOnError(throwOnError)
    {
    }

    virtual ~PGTask();

    /// @brief run the postgres task
    /// @param db database api reference
    /// @param sl The instance of PostgresDatabase to do the queries with.
    /// @param throwOnError throw exception when error occurs
    virtual void run(const std::unique_ptr<pqxx::connection>& conn, PostgresDatabase& pg, bool throwOnError = true) = 0;

    /// @brief returns true if the task is not completed
    /// @return true if the task is not completed yet, false if the task is finished and the results are ready.
    bool is_running() const;

    /// @brief notify the creator of the task using the supplied pthread_mutex and pthread_cond, that the task is finished
    void sendSignal();

    void sendSignal(std::string error);

    void waitForTask();

    bool getThrowOnError() const { return throwOnError; }
    bool didContamination() const { return contamination; }
    bool didDecontamination() const { return decontamination; }

    std::string getError() const { return error; }

    virtual std::string_view taskType() const = 0;

    virtual bool checkKey(const std::string& key) const { return true; }

protected:
    /// @brief true as long as the task is not finished
    /// The value is set by the constructor to true and then to false be sendSignal()
    bool running { true };
    bool throwOnError { false };

    /// @brief true if this task has changed the db (in comparison to the backup)
    bool contamination {};

    /// @brief true if this task has backuped the db
    bool decontamination {};

    std::condition_variable cond;
    mutable std::mutex mutex;

    std::string error;
};

/// @brief A task for the postgres thread to inititally create the database.
class PGInitTask : public PGTask {
public:
    /// @brief Constructor for the postgres init task
    explicit PGInitTask(
        std::shared_ptr<Config> config,
        unsigned int hashie,
        unsigned int stringLimit);

    void run(const std::unique_ptr<pqxx::connection>& conn, PostgresDatabase& pg, bool throwOnError = true) override;

    std::string_view taskType() const override { return "PGInitTask"; }

protected:
    std::shared_ptr<Config> config;
    unsigned int hashie;
    unsigned int stringLimit;
};

/// @brief A task for the postgres thread to do a SQL select.
class PGSelectTask : public PGTask {
public:
    /// @brief Constructor for the postgres select task
    /// @param query The SQL query string
    explicit PGSelectTask(const std::string& query);

    void run(const std::unique_ptr<pqxx::connection>& conn, PostgresDatabase& pg, bool throwOnError = true) override;
    [[nodiscard]] std::shared_ptr<PostgresSQLResult> getResult() const { return pres; }

    std::string_view taskType() const override { return "PGSelectTask"; }

protected:
    /// @brief The SQL query string
    std::string query;
    /// @brief The PostgresSQLResult
    std::shared_ptr<PostgresSQLResult> pres;
};

/// @brief A task for the postgres thread to do a SQL exec.
class PGExecTask : public PGTask {
public:
    /// @brief Constructor for the postgres exec task
    /// @param query The SQL query string
    /// @param getLastInsertId return the last created object
    /// @param warnOnly no not throw exceptions
    PGExecTask(const std::string& query, const std::string& getLastInsertId, bool warnOnly = false);
    PGExecTask(const std::string& query);

    void run(const std::unique_ptr<pqxx::connection>& conn, PostgresDatabase& pg, bool throwOnError = true) override;
    int getLastInsertId() const { return lastInsertId; }

    std::string_view taskType() const override { return "PGExecTask"; }

protected:
    /// @brief The SQL query string
    std::string query;

    int lastInsertId {};
    std::string lastInsertColumn {};
};

#endif // __POSTGRES_TASK_H__
