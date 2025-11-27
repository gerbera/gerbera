/*GRB*
    Gerbera - https://gerbera.io/

    postgres_database.cc - this file is part of Gerbera.

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

/// @file database/postgres/postgres_database.cc
/// @brief Definitions of the PostgresDatabase, Sqlite3Result and Sqlite3Row classes.

#define GRB_LOG_FAC GrbLogFacility::postgres

#ifdef HAVE_PGSQL
#include "postgres_database.h" // API

#include "cds/cds_enums.h"
#include "config/config.h"
#include "config/config_val.h"
#include "database/sql_table.h"
#include "exceptions.h"
#include "pg_result.h"
#include "pg_task.h"
#include "util/thread_runner.h"
#include "util/tools.h"

#include <netinet/in.h>
#include <regex>

static constexpr auto postgresUpdateVersion = std::string_view(R"(UPDATE "mt_internal_setting" SET "value"='{}' WHERE "key"='db_version' AND "value"='{}')");
static const auto postgresAddResourceAttr = std::map<ResourceDataType, std::string_view> {
    { ResourceDataType::String, R"(ALTER TABLE "grb_cds_resource" ADD COLUMN "{}" varchar(255) default NULL)" },
    { ResourceDataType::Number, R"(ALTER TABLE "grb_cds_resource" ADD COLUMN "{}" bigint default NULL)" }
};

PostgresDatabase::PostgresDatabase(std::shared_ptr<Config> config,
    std::shared_ptr<Mime> mime,
    std::shared_ptr<ConverterManager> converterManager)
    : SQLDatabase(config, mime, converterManager)
{
    table_quote_begin = '"';
    table_quote_end = '"';
    firstDBVersion = 25; // no need to migrate from older version
    // if postgres.sql or postgres-upgrade.xml is changed hashies have to be updated
    hashies = {
        2694248237, // index 0 is used for create script postgres.sql = Version 1
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // upgrade 2-11
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // upgrade 12-21
        0, 0, 0, 0,
        2796031870 // index DBVERSION is used for drop script postgres-drop.sql = Version -1
    };
}

PostgresDatabase::~PostgresDatabase()
{
    shutdownDriver();
}

std::string PostgresDatabase::prepareDatabase()
{
    std::string dbVersion;
    try {
        dbVersion = getInternalSetting("db_version");
    } catch (const std::runtime_error& e) {
        log_debug("{}", e.what());
    }

    if (dbVersion.empty()) {
        log_info("Database doesn't seem to exist. Creating database...");
        auto itask = std::make_shared<PGScriptTask>(config, hashies.at(0), stringLimit, ConfigVal::SERVER_STORAGE_PGSQL_INIT_SQL_FILE);
        addTask(itask);
        try {
            itask->waitForTask();
            fillDatabase();
            storeInternalSetting("string_limit", fmt::to_string(stringLimit));
            storeInternalSetting("resource_attribute", "");
            dbVersion = getInternalSetting("db_version");
        } catch (const std::runtime_error& e) {
            shutdown();
            throw_std_runtime_error("Error while creating database: {}", e.what());
        }
        log_info("Database created successfully!");

        if (dbVersion.empty()) {
            shutdown();
            throw_std_runtime_error("error while creating database");
        }
        log_info("Database created successfully!");
    }
    return dbVersion;
}

void PostgresDatabase::run()
{
    taskQueueOpen = true;
    threadRunner = std::make_unique<StdThreadRunner>(
        "PGLiteThread", [](void* arg) {
            auto inst = static_cast<PostgresDatabase*>(arg);
            inst->threadProc(); }, this);

    if (!threadRunner->isAlive()) {
        throw DatabaseException("", fmt::format("Could not start postgres thread: {}", std::strerror(errno)));
    }

    // wait for postgres thread to become ready
    threadRunner->waitForReady();
    if (!startupError.empty())
        throw DatabaseException("", startupError);
}

void PostgresDatabase::init()
{
    dbInitDone = false;
    SQLDatabase::init();

    std::string dbVersion = prepareDatabase();

    try {
        upgradeDatabase(std::stoul(dbVersion), hashies, ConfigVal::SERVER_STORAGE_PGSQL_UPGRADE_FILE, postgresUpdateVersion, postgresAddResourceAttr);

        auto dbLimit = getInternalSetting("string_limit");
        if (!dbLimit.empty() && std::stoul(dbLimit) > stringLimit)
            log_warning("Your database was created with string length {} but your current config value is {}", dbLimit, stringLimit);
        else if (dbLimit.empty()) {
            log_info("Saving string limit {}", stringLimit);
            storeInternalSetting("string_limit", fmt::to_string(stringLimit));
        }
        dbInitDone = true;
    } catch (const std::runtime_error& e) {
        log_error("Prematurely shutting down.\n{}", e.what());
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

void PostgresDatabase::dropTables()
{
    auto file = config->getOption(ConfigVal::SERVER_STORAGE_PGSQL_DROP_FILE);
    log_info("Dropping tables with {}", file);
    auto dtask = std::make_shared<PGScriptTask>(config, hashies.at(DBVERSION), stringLimit, ConfigVal::SERVER_STORAGE_PGSQL_DROP_FILE);
    addTask(dtask);
    try {
        dtask->waitForTask();
    } catch (const std::runtime_error& e) {
        shutdown();
        throw_std_runtime_error("Error while dropping database: {}", e.what());
    }
    log_info("Database tables dropped successfully.");
}

static void handlePqxxNotice(pqxx::zview v)
{
    log_debug("Error {}", v.data());
}

void PostgresDatabase::threadProc()
{
    log_debug("Running thread");
    try {
        std::string dbHost = config->getOption(ConfigVal::SERVER_STORAGE_PGSQL_HOST);
        std::string dbName = config->getOption(ConfigVal::SERVER_STORAGE_PGSQL_DATABASE);
        std::string dbUser = config->getOption(ConfigVal::SERVER_STORAGE_PGSQL_USERNAME);
        auto port = in_port_t(config->getIntOption(ConfigVal::SERVER_STORAGE_PGSQL_PORT));
        std::string dbPass = config->getOption(ConfigVal::SERVER_STORAGE_PGSQL_PASSWORD);
        if (!dbPass.empty())
            dbPass = fmt::format(":{}", dbPass);
        std::string dbPort = (port > 0) ? fmt::format(":{}", port) : "";

        // postgresql://[userspec@][hostspec][/dbname], userspec=user[:password], hostspec=[host][:port]
        auto connString = fmt::format("postgresql://{}{}@{}{}/{}", dbUser, dbPass, dbHost, dbPort, dbName);
        log_debug("connecting to postgresql://{}@{}{}/{}", dbUser, dbHost, dbPort, dbName);
        conn = std::make_unique<pqxx::connection>(connString);

        log_info("Connected to PostgreSQL {}/{} on server version {} {}", dbHost, dbName, conn->server_version(), conn->get_client_encoding());
        conn->set_client_encoding("utf8");
        conn->set_notice_handler(handlePqxxNotice);
        StdThreadRunner::waitFor("PostgresDatabase", [this] { return threadRunner != nullptr; });
        auto lock = threadRunner->uniqueLockS("threadProc");
        // tell init() that we are ready
        threadRunner->setReady();

        auto throwOnError = [&](const std::shared_ptr<PGTask>& task) {
            return task->getThrowOnError();
        };

        while (!shutdownFlag) {
            while (!taskQueue.empty()) {
                auto task = std::move(taskQueue.front());
                taskQueue.pop();

                lock.unlock();
                try {
                    task->run(conn, *this, throwOnError(task));
                    if (task->didContamination())
                        dirty = true;
                    else if (task->didDecontamination())
                        dirty = false;
                    task->sendSignal();
                } catch (const pqxx::argument_error& e) {
                    log_debug(e.what());
                    task->sendSignal(e.what());
                } catch (const std::runtime_error& e) {
                    task->sendSignal(e.what());
                } catch (const std::logic_error& e) {
                    task->sendSignal(e.what());
                }
                lock.lock();
            }

            /* if nothing to do, sleep until awakened */
            threadRunner->wait(lock);
        }
        log_debug("Exiting");

        taskQueueOpen = false;
        while (!taskQueue.empty()) {
            auto task = std::move(taskQueue.front());
            taskQueue.pop();
            task->sendSignal("Sorry, postgres thread is shutting down");
        }

        if (conn) {
            log_debug("closing database");
            conn.reset(); // calls conn->close()
            conn = nullptr;
        }
    } catch (const std::runtime_error& e) {
        log_error("Aborting thread {}", e.what());
    } catch (const std::exception& e) {
        log_error("Aborting thread {}", e.what());
    }
}

void PostgresDatabase::handleException(const std::exception& exc, const std::string& lineMessage)
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

void PostgresDatabase::addTask(const std::shared_ptr<PGTask>& task, bool onlyIfDirty)
{
    if (!taskQueueOpen) {
        throw_std_runtime_error("postgres task queue is already closed");
    }

    auto lock = threadRunner->lockGuard(fmt::format("addTask {}", task->taskType()));

    if (!taskQueueOpen) {
        throw_std_runtime_error("postgres task queue is already closed");
    }
    if (!onlyIfDirty || dirty) {
        taskQueue.push(task);
        threadRunner->notify();
    }
}

void PostgresDatabase::del(std::string_view tableName, const std::string& clause, const std::vector<int>& ids)
{
    try {
        auto query = clause.empty() //
            ? fmt::format("DELETE FROM {}", identifier(std::string(tableName))) //
            : fmt::format("DELETE FROM {} WHERE {}", identifier(std::string(tableName)), clause);
        log_debug("Adding delete to Queue: {}", query);
        auto etask = std::make_shared<PGExecTask>(query, "");
        addTask(etask);
        etask->waitForTask();
    } catch (const std::exception& e) {
        handleException(e, LINE_MESSAGE);
    }
}

void PostgresDatabase::execOnTable(std::string_view tableName, const std::string& query, int objId)
{
    try {
        log_debug("Adding query to Queue: {}", query);
        auto etask = std::make_shared<PGExecTask>(query);
        addTask(etask);
        etask->waitForTask();
    } catch (const std::exception& e) {
        handleException(e, LINE_MESSAGE);
    }
}

int PostgresDatabase::exec(const std::string& query, const std::string& getLastInsertId)
{
    try {
        log_debug("Adding query to Queue: {}", query);
        auto etask = std::make_shared<PGExecTask>(query, getLastInsertId);
        addTask(etask);
        etask->waitForTask();
        return getLastInsertId.empty() ? -1 : etask->getLastInsertId();
    } catch (const std::exception& e) {
        handleException(e, LINE_MESSAGE);
        return -1;
    }
}

void PostgresDatabase::execOnly(const std::string& query)
{
    try {
        log_debug("Adding query to Queue: {}", query);
        auto etask = std::make_shared<PGExecTask>(query, "", false);
        addTask(etask);
        etask->waitForTask();
    } catch (const std::exception& e) {
        log_error("Failed to execute {}\n{}", query, e.what());
    }
}

std::shared_ptr<SQLResult> PostgresDatabase::select(const std::string& query)
{
    try {
        log_debug("Adding select to Queue: {}", query);
        auto stask = std::make_shared<PGSelectTask>(query);
        addTask(stask);
        stask->waitForTask();
        return stask->getResult();
    } catch (const std::exception& e) {
        handleException(e, LINE_MESSAGE);
        return {};
    }
}

void PostgresDatabase::_exec(const std::string& query)
{
    execOnly(query);
}

void PostgresDatabase::shutdownDriver()
{
    if (conn && conn->is_open())
        conn->close();
}

std::string PostgresDatabase::quote(const std::string& value) const
{
    if (!conn)
        throw DatabaseException("", "No PostgreSQL connection for quoting");
    try {
        return conn->quote(value);
    } catch (const pqxx::argument_error& e) {
        log_error("Argument Error.\n{}\n{}: {}", LINE_MESSAGE, value, e.what());
        std::regex char_re("[^_ [:alnum:]");
        auto nval = std::regex_replace(value, char_re, "");
        return conn->quote(nval);
    }
}

std::string PostgresDatabase::getUnreferencedQuery(const std::string& table)
{
    auto colId = browseColumnMapper->mapQuoted(BrowseColumn::Id, true);
    auto colRefId = browseColumnMapper->mapQuoted(BrowseColumn::RefId, true);
    return fmt::format("SELECT {} FROM {} \"cds1\" where {} IS NOT NULL AND NOT EXISTS (SELECT * FROM {} \"cds2\" WHERE \"cds1\".{} = \"cds2\".{})",
        colId, table, colRefId, table, colRefId, colId);
}

std::shared_ptr<Database> PostgresDatabase::getSelf()
{
    return shared_from_this();
}

void PostgresDatabase::storeInternalSetting(const std::string& key, const std::string& value)
{
    std::string sql = fmt::format("INSERT INTO \"mt_internal_setting\" (\"key\", \"value\") VALUES ({0}, {1}) ON CONFLICT (\"key\") DO UPDATE SET \"value\" = EXCLUDED.\"value\"", quote(key), quote(value));
    execOnly(sql);
}

/* PostgresDatabaseWithTransactions */

PostgresDatabaseWithTransactions::PostgresDatabaseWithTransactions(
    const std::shared_ptr<Config>& config,
    const std::shared_ptr<Mime>& mime,
    const std::shared_ptr<ConverterManager>& converterManager)
    : SqlWithTransactions(config)
    , PostgresDatabase(config, mime, converterManager)
{
}

void PostgresDatabaseWithTransactions::beginTransaction(std::string_view tName)
{
    if (use_transaction) {
        log_debug("BEGIN TRANSACTION {} {}", tName, inTransaction);
        SqlAutoLock lock(sqlMutex);
        log_debug("BEGIN TRANSACTION LOCK {} {}", tName, inTransaction);
        StdThreadRunner::waitFor(
            fmt::format("PostgresDatabaseWithTransactions.begin {}", tName), [this] { return !inTransaction; }, 100);
        inTransaction = true;
        _exec("BEGIN TRANSACTION");
    }
}

void PostgresDatabaseWithTransactions::rollback(std::string_view tName)
{
    if (use_transaction && inTransaction) {
        log_debug("ROLLBACK {} {}", tName, inTransaction);
        _exec("ROLLBACK");
        inTransaction = false;
    }
}

void PostgresDatabaseWithTransactions::commit(std::string_view tName)
{
    if (use_transaction && inTransaction) {
        log_debug("COMMIT {} {}", tName, inTransaction);
        _exec("COMMIT");
        inTransaction = false;
    }
}

#endif
