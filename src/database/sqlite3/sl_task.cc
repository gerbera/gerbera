/*GRB*
    Gerbera - https://gerbera.io/

    sl_task.cc - this file is part of Gerbera.

    Copyright (C) 2022-2025 Gerbera Contributors

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

/// @file database/sqlite3/sl_task.cc
#define GRB_LOG_FAC GrbLogFacility::sqlite3

#include "sl_task.h" // API

#include "config/config.h"
#include "config/config_val.h"
#include "exceptions.h"
#include "sqlite_database.h"
#include "util/tools.h"

SLTask::~SLTask() = default;

bool SLTask::is_running() const
{
    return running;
}

void SLTask::sendSignal()
{
    if (is_running()) { // we check before we lock first, because there is no need to lock then
        std::scoped_lock<decltype(mutex)> lock(mutex);
        running = false;
        cond.notify_one();
    }
}

void SLTask::sendSignal(std::string error)
{
    this->error = std::move(error);
    sendSignal();
}

void SLTask::waitForTask()
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

/* SLInitTask */
SLInitTask::SLInitTask(
    std::shared_ptr<Config> config,
    unsigned int hashie,
    unsigned int stringLimit)
    : config(std::move(config))
    , hashie(hashie)
    , stringLimit(stringLimit)
{
}

void SLInitTask::run(sqlite3*& db, Sqlite3Database& sl, bool throwOnError)
{
    log_debug("Running: init");
    std::string dbFilePath = config->getOption(ConfigVal::SERVER_STORAGE_SQLITE_DATABASE_FILE);

    sqlite3_close(db);

    int res = sqlite3_open(dbFilePath.c_str(), &db);
    if (res != SQLITE_OK)
        throw DatabaseException("", "SQLite: Failed to create new database");

    auto sqlFilePath = fs::path(config->getOption(ConfigVal::SERVER_STORAGE_SQLITE_INIT_SQL_FILE));
    log_debug("Loading initialisation SQL from: {}", sqlFilePath.c_str());
    auto sql = GrbFile(std::move(sqlFilePath)).readTextFile();
    auto&& myHash = stringHash(sql);
    replaceAllString(sql, STRING_LIMIT, fmt::to_string(stringLimit));

    if (myHash == hashie) {
        sql += fmt::format("\n" SQLITE3_SET_VERSION ";", DBVERSION);

        char* err = nullptr;
        int ret = sqlite3_exec(
            db,
            sql.c_str(),
            nullptr,
            nullptr,
            &err);
        std::string error;
        if (err) {
            error = err;
            sqlite3_free(err);
        }
        if (ret != SQLITE_OK) {
            throw DatabaseException("", sl.handleError(sql, error, db, ret));
        }
        contamination = true;
    } else {
        log_warning("Wrong hash for create script {}: {} != {}", DBVERSION, myHash, hashie);
    }
}

/* SLSelectTask */
SLSelectTask::SLSelectTask(const std::string& query)
    : query(query.c_str())
{
}

void SLSelectTask::run(sqlite3*& db, Sqlite3Database& sl, bool throwOnError)
{
    log_debug("Running: {}", query);
    pres = std::make_shared<Sqlite3Result>();

    char* err = nullptr;
    int ret = sqlite3_get_table(
        db,
        query,
        &pres->table,
        &pres->nrow,
        &pres->ncolumn,
        &err);
    std::string error;
    if (err) {
        log_debug(err);
        error = err;
        sqlite3_free(err);
    }
    if (ret != SQLITE_OK) {
        throw DatabaseException("", sl.handleError(query, error, db, ret));
    }

    pres->row = pres->table;
    pres->cur_row = 0;
}

/* SLExecTask */

SLExecTask::SLExecTask(const std::string& query, const std::string& getLastInsertId, bool warnOnly)
    : SLTask(!warnOnly)
    , query(query.c_str())
    , lastInsertColumn(getLastInsertId)
{
}

SLExecTask::SLExecTask(const std::string& query, std::string eKey)
    : query(query.c_str())
    , eKey(std::move(eKey))
{
}

void SLExecTask::run(sqlite3*& db, Sqlite3Database& sl, bool throwOnError)
{
    log_debug("Running: {}", query);
    char* err;
    int ret = sqlite3_exec(
        db,
        query,
        nullptr,
        nullptr,
        &err);
    std::string error;
    if (err) {
        error = err;
        sqlite3_free(err);
    }
    if (ret != SQLITE_OK) {
        if (throwOnError)
            throw DatabaseException("", sl.handleError(query, error, db, ret));
        log_debug("Error in sqlite3_exec: {}", sl.handleError(query, error, db, ret));
    }
    if (!lastInsertColumn.empty())
        lastInsertId = sqlite3_last_insert_rowid(db);
    contamination = true;
}

/* SLBackupTask */
SLBackupTask::SLBackupTask(std::shared_ptr<Config> config, bool restore)
    : config(std::move(config))
    , restore(restore)
    , dbFile(this->config->getOption(ConfigVal::SERVER_STORAGE_SQLITE_DATABASE_FILE))
    , dbBackupFile(fmt::format(SQLITE3_BACKUP_FORMAT, this->config->getOption(ConfigVal::SERVER_STORAGE_SQLITE_DATABASE_FILE)))
{
}

void SLBackupTask::run(sqlite3*& db, Sqlite3Database& sl, bool throwOnError)
{
    log_debug("Running: backup");

    if (!restore) {
        try {
            fs::copy(
                dbFile.getPath(),
                dbBackupFile.getPath(),
                fs::copy_options::overwrite_existing);
            log_debug("sqlite3 backup successful");
            dbBackupFile.setPermissions();
            decontamination = true;
        } catch (const std::runtime_error& e) {
            log_error("error while making sqlite3 backup: {}", e.what());
        }
    } else {
        log_info("trying to restore sqlite3 database from backup...");
        sqlite3_close(db);
        try {
            fs::copy(
                dbBackupFile.getPath(),
                dbFile.getPath(),
                fs::copy_options::overwrite_existing);
            dbFile.setPermissions();
        } catch (const std::runtime_error& e) {
            throw DatabaseException(fmt::format("Error while restoring sqlite3 backup: {}", e.what()), fmt::format("Error while restoring sqlite3 backup: {}", e.what()));
        }
        int res = sqlite3_open(dbFile.getPath().c_str(), &db);
        if (res != SQLITE_OK) {
            throw DatabaseException("", "error while restoring sqlite3 backup: could not reopen sqlite3 database after restore");
        }
        log_info("sqlite3 database successfully restored from backup.");
    }
}
