/*GRB*
    Gerbera - https://gerbera.io/

    pg_task.cc - this file is part of Gerbera.

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

/// @file database/postgres/pg_task.cc
#define GRB_LOG_FAC GrbLogFacility::postgres

#ifdef HAVE_PGSQL
#include "pg_task.h" // API

#include "config/config.h"
#include "config/config_val.h"
#include "exceptions.h"
#include "postgres_database.h"
#include "util/tools.h"

PGTask::~PGTask() = default;

bool PGTask::is_running() const
{
    return running;
}

void PGTask::sendSignal()
{
    if (is_running()) { // we check before we lock first, because there is no need to lock then
        std::scoped_lock<decltype(mutex)> lock(mutex);
        running = false;
        cond.notify_one();
    }
}

void PGTask::sendSignal(std::string error)
{
    this->error = std::move(error);
    sendSignal();
}

void PGTask::waitForTask()
{
    if (is_running()) { // we check before we lock first, because there is no need to lock then
        std::unique_lock<decltype(mutex)> lock(mutex);
        if (is_running()) { // we check it a second time after locking to ensure we didn't miss the pthread_cond_signal
            cond.wait(lock); // waiting for the task to complete
        }
    }

    if (!getError().empty()) {
        log_debug("{}", getError());
        throw_std_runtime_error(getError());
    }
}

/* PGInitTask */
PGInitTask::PGInitTask(
    std::shared_ptr<Config> config,
    unsigned int hashie,
    unsigned int stringLimit)
    : config(std::move(config))
    , hashie(hashie)
    , stringLimit(stringLimit)
{
}

void PGInitTask::run(
    const std::unique_ptr<pqxx::connection>& conn,
    PostgresDatabase& pg,
    bool throwOnError)
{
    log_debug("Running: init");

    auto sqlFilePath = fs::path(config->getOption(ConfigVal::SERVER_STORAGE_PGSQL_INIT_SQL_FILE));
    log_debug("Loading initialisation SQL from: {}", sqlFilePath.c_str());
    auto sql = GrbFile(std::move(sqlFilePath)).readTextFile();
    auto&& myHash = stringHash(sql);
    replaceAllString(sql, STRING_LIMIT, fmt::to_string(stringLimit));

    if (myHash == hashie) {
        sql += fmt::format(";\n" POSTGRES_SET_VERSION ";", DBVERSION);

        for (auto&& statement : splitString(sql, ';')) {
            trimStringInPlace(statement);
            if (statement.empty()) {
                continue;
            }
            replaceAllString(statement, STRING_LIMIT, fmt::to_string(stringLimit));
            log_debug("executing statement: '{}'", statement);
            pqxx::work txn(*conn);
            pqxx::result res = txn.exec(statement);
            txn.commit();
        }
        contamination = true;
    } else {
        log_warning("Wrong hash for create script {}: {} != {}", DBVERSION, myHash, hashie);
    }
}

/* PGSelectTask */

PGSelectTask::PGSelectTask(const std::string& query)
    : query(query)
{
}

void PGSelectTask::run(
    const std::unique_ptr<pqxx::connection>& conn,
    PostgresDatabase& pg,
    bool throwOnError)
{
    log_debug("Running: {}", query);

    pqxx::work txn(*conn);
    pqxx::result res = txn.exec(query);
    txn.commit();
    pres = std::make_shared<PostgresSQLResult>(res);
}

/* PGExecTask */

PGExecTask::PGExecTask(
    const std::string& query,
    const std::string& getLastInsertId,
    bool warnOnly)
    : PGTask(!warnOnly)
    , query(query.c_str())
    , lastInsertColumn(getLastInsertId)
{
    if (!lastInsertColumn.empty())
        this->query = fmt::format("{} RETURNING {}", this->query, this->lastInsertColumn);
}

PGExecTask::PGExecTask(const std::string& q)
    : query(q)
{
}

void PGExecTask::run(
    const std::unique_ptr<pqxx::connection>& conn,
    PostgresDatabase& pg,
    bool throwOnError)
{
    log_debug("Running: {}", query);
    pqxx::work txn(*conn);
    pqxx::result res = txn.exec(query);
    txn.commit();
    if (!lastInsertColumn.empty() && !res.empty() && res.at(0).size() > 0) {
        lastInsertId = res.at(0).at(0).as<int>();
        log_debug("lastInsertId: {} -> {}", query, lastInsertId);
    }
    contamination = true;
}
#endif
