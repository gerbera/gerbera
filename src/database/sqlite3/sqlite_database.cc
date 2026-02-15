/*MT*

    MediaTomb - http://www.mediatomb.cc/

    sqlite_database.cc - this file is part of MediaTomb.

    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>

    Copyright (C) 2006-2010 Gena Batyan <bgeradz@mediatomb.cc>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>,
                            Leonhard Wimmer <leo@mediatomb.cc>

    Copyright (C) 2016-2026 Gerbera Contributors

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

/// @file database/sqlite3/sqlite_database.cc
#define GRB_LOG_FAC GrbLogFacility::sqlite3

#include "sqlite_database.h" // API

#include "cds/cds_enums.h"
#include "config/config.h"
#include "config/config_val.h"
#include "exceptions.h"
#include "sl_result.h"
#include "sl_task.h"

#include <sqlite3.h>

static constexpr auto sqlite3UpdateVersion = std::string_view(R"(UPDATE "mt_internal_setting" SET "value"='{}' WHERE "key"='db_version' AND "value"='{}')");
static const auto sqlite3AddResourceAttr = std::map<ResourceDataType, std::string_view> {
    { ResourceDataType::String, R"(ALTER TABLE "grb_cds_resource" ADD COLUMN "{}" varchar(255) default NULL)" },
    { ResourceDataType::Number, R"(ALTER TABLE "grb_cds_resource" ADD COLUMN "{}" bigint default NULL)" }
};

#define DELETE_CACHE_MAX_TIME 60 // drop cache if last delete was more than 60 secs ago
#define DELETE_CACHE_RED_SIZE 0.2 // reduce cache to 80% of max entries

Sqlite3Database::Sqlite3Database(const std::shared_ptr<Config>& config, const std::shared_ptr<Mime>& mime, const std::shared_ptr<ConverterManager>& converterManager, std::shared_ptr<Timer> timer)
    : SQLDatabase(config, mime, converterManager)
    , timer(std::move(timer))
    , shutdownAttempts(this->config->getIntOption(ConfigVal::SERVER_STORAGE_SQLITE_SHUTDOWN_ATTEMPTS))
{
    dbFilePath = config->getOption(ConfigVal::SERVER_STORAGE_SQLITE_DATABASE_FILE);
    table_quote_begin = '"';
    table_quote_end = '"';

    // if sqlite3.sql or sqlite3-upgrade.xml is changed hashies have to be updated
    hashies = {
        1911103696, // index 0 is used for create script sqlite3.sql = Version 1
        778996897, 3362507034, 853149842, 2776802417, 3497064885, 974692115, 119767663, 3167732653, 2427825904, 3305506356, // upgrade 2-11
        3908767237, 509765404, 2512852146, 1273710965, 319062951, 2316641127, 1028160353, 881071639, 1989518047, 782849313, // upgrade 12-21
        3135921396, 3108208, 2156790525, 2004941040, 3165480624, 2252596209,
        459854332 // index DBVERSION is used for drop script sqlite3-drop.sql = Version -1
    };
}

void Sqlite3Database::prepare()
{
    _exec("PRAGMA locking_mode = EXCLUSIVE");
    _exec("PRAGMA foreign_keys = ON");
    _exec(fmt::format("PRAGMA journal_mode = {}", config->getOption(ConfigVal::SERVER_STORAGE_SQLITE_JOURNALMODE)));
    exec(fmt::format("PRAGMA synchronous = {}", config->getIntOption(ConfigVal::SERVER_STORAGE_SQLITE_SYNCHRONOUS)));
}

std::string Sqlite3Database::prepareDatabase(const fs::path& dbFilePath, GrbFile& dbFile)
{
    std::string dbVersion;
    fs::path dbFilePathbackup = fmt::format(SQLITE3_BACKUP_FORMAT, dbFilePath.c_str());
    auto dbBackupFile = GrbFile(dbFilePathbackup);
    try {
        dbVersion = getInternalSetting("db_version");
    } catch (const std::runtime_error&) {
        log_warning("SQLite3 database seems to be corrupt or doesn't exist yet.");
        // database seems to be corrupt or nonexistent
        if (config->getBoolOption(ConfigVal::SERVER_STORAGE_SQLITE_RESTORE) && sqliteStatus == SQLITE_OK) {
            // try to restore database

            // checking for backup file
            if (dbBackupFile.isReadable()) {
                try {
                    // trying to copy backup file
                    auto btask = std::make_shared<SLBackupTask>(config, true);
                    this->addTask(btask);
                    btask->waitForTask();
                    prepare();
                    dbVersion = getInternalSetting("db_version");
                } catch (const std::runtime_error&) {
                }
            }
        } else {
            // fail because restore option is false
            shutdown();
            throw_std_runtime_error("SQLite3 database seems to be corrupt and the 'on-error' option is set to 'fail'");
        }
    }

    if (dbVersion.empty() && !dbBackupFile.isReadable()) {
        log_info("No SQLite3 backup is available or backup is corrupt. Automatically creating new database file...");
        auto itask = std::make_shared<SLScriptTask>(config, hashies.at(0), stringLimit, ConfigVal::SERVER_STORAGE_SQLITE_INIT_SQL_FILE);
        addTask(itask);
        try {
            itask->waitForTask();
            fillDatabase();
            storeInternalSetting("string_limit", fmt::to_string(stringLimit));
            storeInternalSetting("resource_attribute", "");
            prepare();
            dbFile.setPermissions();
            dbVersion = getInternalSetting("db_version");
        } catch (const std::runtime_error& e) {
            shutdown();
            throw_std_runtime_error("Error while creating database: {}", e.what());
        }
        log_info("Database tables {} created successfully.", dbFilePath.c_str());
    }

    if (dbVersion.empty()) {
        shutdown();
        throw_std_runtime_error("SQLite3 database seems to be corrupt and restoring from backup failed");
    }

    return dbVersion;
}

void Sqlite3Database::dropTables()
{
    auto file = config->getOption(ConfigVal::SERVER_STORAGE_SQLITE_DROP_FILE);
    log_info("Dropping tables with {}", file);
    auto dtask = std::make_shared<SLScriptTask>(config, hashies.at(DBVERSION), stringLimit, ConfigVal::SERVER_STORAGE_SQLITE_DROP_FILE);
    addTask(dtask);
    try {
        dtask->waitForTask();
    } catch (const std::runtime_error& e) {
        shutdown();
        throw_std_runtime_error("Error while dropping database: {}", e.what());
    }

    // back up file to make sure the tables are dropped from backup as well
    if (config->getBoolOption(ConfigVal::SERVER_STORAGE_SQLITE_RESTORE) && config->getBoolOption(ConfigVal::SERVER_STORAGE_SQLITE_BACKUP_ENABLED)) {
        fs::path dbFilePathbackup = fmt::format(SQLITE3_BACKUP_FORMAT, dbFilePath.c_str());
        auto dbBackupFile = GrbFile(dbFilePathbackup);

        // checking for backup file
        if (dbBackupFile.isReadable()) {
            try {
                auto btask = std::make_shared<SLBackupTask>(config, false);
                this->addTask(btask);
                btask->waitForTask();
            } catch (const std::runtime_error& e) {
                shutdown();
                throw_std_runtime_error("Error while backing up database: {}", e.what());
            }
        }
    }

    log_info("Database tables {} dropped successfully.", dbFilePath.c_str());
}

void Sqlite3Database::run()
{
    auto dbFile = GrbFile(dbFilePath);
    log_debug("SQLite3 path: {}", dbFilePath.c_str());

    // check for db-file
    if (!dbFile.isWritable())
        throw DatabaseException("", fmt::format("Error while accessing sqlite database file ({}): {}", dbFilePath.c_str(), std::strerror(errno)));

    taskQueueOpen = true;
    threadRunner = std::make_unique<StdThreadRunner>(
        "SQLiteThread", [](void* arg) {
            auto inst = static_cast<Sqlite3Database*>(arg);
            inst->threadProc(); }, this);

    if (!threadRunner->isAlive()) {
        throw DatabaseException("", fmt::format("Could not start sqlite thread: {}", std::strerror(errno)));
    }

    // wait for sqlite3 thread to become ready
    threadRunner->waitForReady();
    if (!startupError.empty())
        throw DatabaseException("", startupError);

    // try to detect already active database client and terminate before doing any harm
    try {
        prepare();
    } catch (const std::runtime_error&) {
        shutdown();
        throw_std_runtime_error("Sqlite3Database.run: could not open '{}' exclusively", dbFilePath.c_str());
    }
}

void Sqlite3Database::init()
{
    dbInitDone = false;
    SQLDatabase::init();

    auto dbFile = GrbFile(dbFilePath);
    std::string dbVersion = prepareDatabase(dbFilePath, dbFile);

    try {
        upgradeDatabase(std::stoul(dbVersion), hashies, ConfigVal::SERVER_STORAGE_SQLITE_UPGRADE_FILE, sqlite3UpdateVersion, sqlite3AddResourceAttr);
        if (config->getBoolOption(ConfigVal::SERVER_STORAGE_SQLITE_BACKUP_ENABLED) && timer) {
            // do a backup now
            auto btask = std::make_shared<SLBackupTask>(config, false);
            this->addTask(btask);
            btask->waitForTask();

            // add timer for backups
            auto backupInterval = std::chrono::seconds(config->getLongOption(ConfigVal::SERVER_STORAGE_SQLITE_BACKUP_INTERVAL));
            timer->addTimerSubscriber(this, backupInterval, nullptr);
            hasBackupTimer = true;
        }

        auto dbLimit = getInternalSetting("string_limit");
        if (!dbLimit.empty() && std::stoul(dbLimit) > stringLimit)
            log_warning("Your database was created with string length {} but your current config value is {}", dbLimit, stringLimit);
        else if (dbLimit.empty()) {
            log_info("Saving string limit {}", stringLimit);
            storeInternalSetting("string_limit", fmt::to_string(stringLimit));
        }
        dbInitDone = true;
    } catch (const std::runtime_error& e) {
        log_error("Prematurely shutting down.");
        shutdown();
        throw_std_runtime_error(e.what());
    }

    try {
        initDynContainers();
    } catch (const std::runtime_error& e) {
        log_error("Prematurely shutting down because of initDynContainers.");
        shutdown();
        throw_std_runtime_error(e.what());
    }
}

std::shared_ptr<Database> Sqlite3Database::getSelf()
{
    return shared_from_this();
}

void Sqlite3Database::_exec(const std::string& query)
{
    execOnly(query);
}

std::string Sqlite3Database::quote(const std::string& value) const
{
    // https://www.sqlite.org/printf.html#percentq:
    // The string is printed with all single quote (') characters doubled so that the string can safely appear inside an SQL string literal.
    // The %Q substitution type also puts single-quotes on both ends of the substituted string.
    char* q = sqlite3_mprintf("%Q", value.c_str());
    auto ret = std::string(q);
    sqlite3_free(q);
    return ret;
}

std::string Sqlite3Database::handleError(const std::string& query, const std::string& error, sqlite3* db, int errorCode)
{
    sqliteStatus = (errorCode == SQLITE_BUSY || errorCode == SQLITE_LOCKED) ? SQLITE_LOCKED : SQLITE_OK;
    return fmt::format("SQLite3 ({}: {}): {}\n       Query: {}\n       Error: {}", sqlite3_errcode(db), sqlite3_extended_errcode(db), sqlite3_errmsg(db), query.empty() ? "unknown" : query, error.empty() ? "unknown" : error);
}

void Sqlite3Database::handleException(const std::exception& exc, const std::string& lineMessage)
{
    if (!dbInitDone)
        throw_std_runtime_error(exc.what());
    ++shutdownFlag;
    if (shutdownFlag <= 1) {
        log_error("Prematurely shutting down.\n{}\n{}", lineMessage, exc.what());
        rollback("");
        shutdown();
    }
    if (shutdownFlag >= shutdownAttempts) {
        log_error("Exceeding shutdown limit {}.\n{}\n{}", shutdownAttempts, lineMessage, exc.what());
        std::exit(1);
    }
    log_error("Already shutting down.\n{}\n{}", lineMessage, exc.what());
}

std::shared_ptr<SQLResult> Sqlite3Database::select(const std::string& query)
{
    try {
        log_debug("Adding select to Queue: {}", query);
        auto stask = std::make_shared<SLSelectTask>(query);
        addTask(stask);
        stask->waitForTask();
        return stask->getResult();
    } catch (const std::runtime_error& e) {
        handleException(e, LINE_MESSAGE);
        return {};
    }
}

void Sqlite3Database::del(std::string_view tableName, const std::string& clause, const std::vector<int>& ids)
{
    auto query = clause.empty() //
        ? fmt::format("DELETE FROM {}", identifier(std::string(tableName))) //
        : fmt::format("DELETE FROM {} WHERE {}", identifier(std::string(tableName)), clause);
    try {
        log_debug("Adding delete to Queue: {}", query);
        {
            DelAutoLock del_lock(del_mutex);
            maxDeleteCount = maxDeleteCount > ids.size() * 10 ? maxDeleteCount : ids.size() * 10;
            for (auto&& id : ids) {
                deletedEntries.push_back(fmt::format("{}_{}", tableName, id));
            }
            lastDelete = currentTime();
        }
        auto etask = std::make_shared<SLExecTask>(query, "", false);
        addTask(etask);
        etask->waitForTask();
    } catch (const std::runtime_error& e) {
        handleException(e, LINE_MESSAGE);
    }
}

void Sqlite3Database::execOnTable(std::string_view tableName, const std::string& query, int objId)
{
    try {
        log_debug("Adding query to Queue: {}", query);
        auto eKey = fmt::format("{}_{}", tableName, objId);
        auto etask = std::make_shared<SLExecTask>(query, eKey);
        addTask(etask);
        etask->waitForTask();
    } catch (const std::runtime_error& e) {
        handleException(e, LINE_MESSAGE);
    }
}

int Sqlite3Database::exec(const std::string& query, const std::string& getLastInsertId)
{
    try {
        log_debug("Adding query to Queue: {}", query);
        auto etask = std::make_shared<SLExecTask>(query, getLastInsertId, false);
        addTask(etask);
        etask->waitForTask();
        return getLastInsertId.empty() ? -1 : etask->getLastInsertId();
    } catch (const std::runtime_error& e) {
        handleException(e, LINE_MESSAGE);
        return -1;
    }
}

void Sqlite3Database::execOnly(const std::string& query)
{
    try {
        log_debug("Adding query to Queue: {}", query);
        auto etask = std::make_shared<SLExecTask>(query, "", false);
        addTask(etask);
        etask->waitForTask();
    } catch (const std::runtime_error& e) {
        log_error("Failed to execute {}\n{}", query, e.what());
    }
}

void Sqlite3Database::threadProc()
{
    log_debug("Running thread");
    try {
        sqlite3* db;

        int res = sqlite3_open(dbFilePath.c_str(), &db);
        if (res != SQLITE_OK) {
            startupError = fmt::format("Sqlite3Database.threadProc: could not open '{}'", dbFilePath.c_str());
            return;
        }

        StdThreadRunner::waitFor("Sqlite3Database", [this] { return threadRunner != nullptr; });
        auto lock = threadRunner->uniqueLockS("threadProc");
        // tell init() that we are ready
        threadRunner->setReady();

        auto throwOnError = [&](const std::shared_ptr<SLTask>& task) {
            for (auto&& ent : deletedEntries) {
                if (!task->checkKey(ent))
                    return false;
            }
            return task->getThrowOnError();
        };

        while (!shutdownFlag) {
            while (!taskQueue.empty()) {
                auto task = std::move(taskQueue.front());
                taskQueue.pop();

                lock.unlock();
                try {
                    task->run(db, *this, throwOnError(task));
                    if (task->didContamination())
                        dirty = true;
                    else if (task->didDecontamination())
                        dirty = false;
                    task->sendSignal();
                } catch (const std::runtime_error& e) {
                    task->sendSignal(e.what());
                } catch (const std::logic_error& e) {
                    task->sendSignal(e.what());
                }
                lock.lock();
            }

            /* if nothing to do, sleep until awakened */
            auto now = currentTime();
            if (now.count() - lastDelete.count() > DELETE_CACHE_MAX_TIME) { // drop cache if last delete was more than 60 secs ago
                DelAutoLock del_lock(del_mutex);
                maxDeleteCount = DELETE_CACHE_MAX_SIZE;
                deletedEntries.clear();
            } else if (deletedEntries.size() > maxDeleteCount) { // dynamically increase if large DELETESs happen
                DelAutoLock del_lock(del_mutex);
                deletedEntries.erase(deletedEntries.begin(), deletedEntries.begin() + maxDeleteCount * DELETE_CACHE_RED_SIZE);
            }
            threadRunner->wait(lock);
        }
        log_debug("Exiting");

        taskQueueOpen = false;
        while (!taskQueue.empty()) {
            auto task = std::move(taskQueue.front());
            taskQueue.pop();
            task->sendSignal("Sorry, SQLite3 thread is shutting down");
        }

        if (db) {
            log_debug("closing database");
            if (sqlite3_close(db) == SQLITE_OK) {
                log_debug("Closed database");
            } else {
                log_error("Closing database failed");
            }
            db = nullptr;
        }
    } catch (const std::runtime_error& e) {
        log_error("Aborting thread {}", e.what());
    }
}

/* Sqlite3BackupTimerSubscriber */

void Sqlite3Database::timerNotify(const std::shared_ptr<Timer::Parameter>& param)
{
    auto btask = std::make_shared<SLBackupTask>(config, false);
    addTask(btask, true);
}

void Sqlite3Database::addTask(const std::shared_ptr<SLTask>& task, bool onlyIfDirty)
{
    if (!taskQueueOpen) {
        throw_std_runtime_error("SQLite3 task queue is already closed");
    }

    auto lock = threadRunner->lockGuard(fmt::format("addTask {}", task->taskType()));

    if (!taskQueueOpen) {
        throw_std_runtime_error("SQLite3 task queue is already closed");
    }
    if (!onlyIfDirty || dirty) {
        taskQueue.push(task);
        threadRunner->notify();
    }
}

void Sqlite3Database::shutdownDriver()
{
    log_debug("start");
    auto lock = threadRunner->uniqueLockS("shutdown");
    if (!shutdownFlag) {
        shutdownFlag = true;
        if (hasBackupTimer && timer) {
            timer->removeTimerSubscriber(this, nullptr);
        }
        log_debug("signalling...");
        threadRunner->notify();
        lock.unlock();
        log_debug("waiting for thread");
        threadRunner->join();
    }
    log_debug("end");
}

void Sqlite3Database::storeInternalSetting(const std::string& key, const std::string& value)
{
    auto command = fmt::format("INSERT OR REPLACE INTO {} ({}, {}) VALUES ({}, {})",
        identifier(INTERNAL_SETTINGS_TABLE), identifier("key"), identifier("value"), quote(key), quote(value));
    execOnly(command);
}

/* Sqlite3DatabaseWithTransactions */

Sqlite3DatabaseWithTransactions::Sqlite3DatabaseWithTransactions(
    const std::shared_ptr<Config>& config,
    const std::shared_ptr<Mime>& mime,
    const std::shared_ptr<ConverterManager>& converterManager,
    const std::shared_ptr<Timer>& timer)
    : SqlWithTransactions(config)
    , Sqlite3Database(config, mime, converterManager, timer)
{
}

void Sqlite3DatabaseWithTransactions::beginTransaction(std::string_view tName)
{
    if (use_transaction) {
        log_debug("BEGIN TRANSACTION {} {}", tName, inTransaction);
        SqlAutoLock lock(sqlMutex);
        log_debug("BEGIN TRANSACTION LOCK {} {}", tName, inTransaction);
        StdThreadRunner::waitFor(
            fmt::format("SqliteDatabase.begin {}", tName), [this] { return !inTransaction; }, 100);
        inTransaction = true;
        _exec("BEGIN TRANSACTION");
    }
}

void Sqlite3DatabaseWithTransactions::rollback(std::string_view tName)
{
    if (use_transaction && inTransaction) {
        log_debug("ROLLBACK {} {}", tName, inTransaction);
        _exec("ROLLBACK");
        inTransaction = false;
    }
}

void Sqlite3DatabaseWithTransactions::commit(std::string_view tName)
{
    if (use_transaction && inTransaction) {
        log_debug("COMMIT {} {}", tName, inTransaction);
        _exec("COMMIT");
        inTransaction = false;
    }
}
