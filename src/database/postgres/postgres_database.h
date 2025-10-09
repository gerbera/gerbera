/*GRB*
    Gerbera - https://gerbera.io/

    postgres_database.h - this file is part of Gerbera.

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

/// @file database/postgres/postgres_database.h
/// @brief Definitions of the PostgresDatabase, PostgresSQLResult and PostgresSQLRow classes.

#ifndef __POSTGRES_DATABASE_H__
#define __POSTGRES_DATABASE_H__

#include "database/sql_database.h"
#include "util/thread_runner.h"
#include "util/timer.h"

#include <mutex>
#include <pqxx/pqxx>
#include <queue>

class PostgresSQLResult;
class PostgresSQLRow;
class PGTask;

/// @brief The Database class for using Postgres
class PostgresDatabase
    : public SQLDatabase,
      public std::enable_shared_from_this<SQLDatabase> {
public:
    PostgresDatabase(
        std::shared_ptr<Config> config,
        std::shared_ptr<Mime> mime,
        std::shared_ptr<ConverterManager> converterManager);
    ~PostgresDatabase() override;

    std::string handleError(
        const std::string& query,
        const std::string& error,
        const std::unique_ptr<pqxx::connection>& db,
        int errorCode)
    {
        return "";
    }

protected:
    void _exec(const std::string& query) override;
    std::string prepareDatabase();

private:
    void init() override;
    void shutdownDriver() override;
    std::shared_ptr<Database> getSelf() override;

    std::string quote(const std::string& value) const override;

    std::shared_ptr<SQLResult> select(const std::string& query) override;
    void del(std::string_view tableName, const std::string& clause, const std::vector<int>& ids) override;
    void exec(std::string_view tableName, const std::string& query, int objId) override;
    int exec(const std::string& query, const std::string& getLastInsertId = "") override;
    void execOnly(const std::string& query) override;

    /// @brief Implement common behaviour on exceptions while calling the database
    void handleException(const std::exception& exc, const std::string& lineMessage);

    void storeInternalSetting(const std::string& key, const std::string& value) override;

    std::string startupError;

    std::unique_ptr<StdThreadRunner> threadRunner;
    void threadProc();

    void addTask(const std::shared_ptr<PGTask>& task, bool onlyIfDirty = false);

    std::unique_ptr<pqxx::connection> conn;
    std::shared_ptr<Timer> timer;

    /// @brief increased by shutdown attempt if the sqlite3 thread should terminate
    int shutdownFlag { 0 };

    /// @brief the tasks to be done by the sqlite3 thread
    std::queue<std::shared_ptr<PGTask>> taskQueue;
    bool taskQueueOpen {};

    void threadCleanup() override { }
    bool threadCleanupRequired() const override { return false; }

    bool dirty {};
    bool dbInitDone {};
    /// @brief maximum number of attempts to terminate gracefully
    int shutdownAttempts { 5 };
};

/// @brief The Database class for using Postgres with transactions
class PostgresDatabaseWithTransactions : public SqlWithTransactions, public PostgresDatabase {
public:
    PostgresDatabaseWithTransactions(
        const std::shared_ptr<Config>& config,
        const std::shared_ptr<Mime>& mime,
        const std::shared_ptr<ConverterManager>& converterManager);

    void beginTransaction(std::string_view tName) override;
    void rollback(std::string_view tName) override;
    void commit(std::string_view tName) override;
};

/// @brief Represents a result of a Postgres select
class PostgresSQLResult : public SQLResult {
public:
    explicit PostgresSQLResult(const pqxx::result& r)
        : result(r)
        , row(0)
    {
    }
    PostgresSQLResult() = default;
    ~PostgresSQLResult() override;

    PostgresSQLResult(const PostgresSQLResult&) = delete;
    PostgresSQLResult& operator=(const PostgresSQLResult&) = delete;

private:
    std::unique_ptr<SQLRow> nextRow() override;
    unsigned long long getNumRows() const override { return result.size(); }

    pqxx::result result;
    pqxx::result::size_type row;
};

/// @brief Represents a row of a result of a Postgres select
class PostgresSQLRow : public SQLRow {
public:
    explicit PostgresSQLRow(const pqxx::row& r)
        : row(r)
    {
    }

private:
    char* col_c_str(int index) const override
    {
        return (index < row.size() && !row.at(index).is_null())
            ? const_cast<char*>(row.at(index).c_str())
            : nullptr;
    }
    pqxx::row row;
};

#endif // __POSTGRES_DATABASE_H__
