/*GRB*

    Gerbera - https://gerbera.io/

    test_database.cc - this file is part of Gerbera.

    Copyright (C) 2020-2026 Gerbera Contributors

    Gerbera is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.

    Gerbera is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Gerbera.  If not, see <http://www.gnu.org/licenses/>.

    $Id$
*/

/// \file test_database.cc
#include "cds/cds_container.h"
#include "cds/cds_item.h"
#include "cds/cds_objects.h"
#include "cds/cds_resource.h"
#include "config/result/autoscan.h"
#include "config/result/box_layout.h"
#include "database/database.h"
#include "database/sql_database.h"
#include "database/sqlite3/sqlite_database.h"
#include "exceptions.h"
#include "sqlite_config_fake.h"
#include "upnp/xml_builder.h"
#include "util/fmt_compat.h"
#include "util/tools.h"

#ifdef HAVE_MYSQL
#include "database/mysql/mysql_database.h"
#include "mysql_config_fake.h"
#endif

#ifdef HAVE_PGSQL
#include "database/postgres/postgres_database.h"
#include "postgres_config_fake.h"
#include "util/url_utils.h"
#endif

#include <cstdint>
#include <cstdlib>
#include <gtest/gtest.h>
#include <memory>
#include <pugixml.hpp>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

class TestDatabaseFactory : public Database {
public:
    using Database::createInstance;
};

class DatabaseTestBase : public ::testing::Test {

protected:
    void testUpgrade(ConfigVal option);
    std::shared_ptr<Database> subject;
    std::shared_ptr<Config> config;
};

void DatabaseTestBase::testUpgrade(ConfigVal option)
{
    const fs::path& upgradeFile = config->getOption(option);
    pugi::xml_document xmlDoc;
    pugi::xml_parse_result result = xmlDoc.load_file(upgradeFile.c_str());
    if (result.status != pugi::xml_parse_status::status_ok) {
        throw ConfigParseException(result.description());
    }
    auto root = xmlDoc.document_element();
    EXPECT_TRUE(root.name() == std::string_view("upgrade"));

    size_t version = subject->getFirstVersion();
    for (auto&& versionElement : root.select_nodes("/upgrade/version")) {
        auto versionNode = versionElement.node();
        auto&& myHash = stringHash(UpnpXMLBuilder::printXml(versionNode));
        EXPECT_EQ(myHash, std::dynamic_pointer_cast<SQLDatabase>(subject)->getHash(version));
        version++;
    }
}

class Sqlite3DatabaseTest : public DatabaseTestBase {

public:
    Sqlite3DatabaseTest() = default;
    ~Sqlite3DatabaseTest() override = default;

    void SetUp() override
    {
        config = std::make_shared<SqliteConfigFake>();
        subject = std::make_shared<Sqlite3Database>(config, nullptr, nullptr, nullptr);
    }

    void TearDown() override
    {
        subject = nullptr;
    }
};

TEST_F(Sqlite3DatabaseTest, CheckInitScript)
{
    auto sqlFilePath = config->getOption(ConfigVal::SERVER_STORAGE_SQLITE_INIT_SQL_FILE);
    auto sql = GrbFile(sqlFilePath).readTextFile();
    auto&& myHash = stringHash(sql);

    EXPECT_EQ(myHash, std::dynamic_pointer_cast<SQLDatabase>(subject)->getHash(0));
}

TEST_F(Sqlite3DatabaseTest, CheckDropScript)
{
    auto sqlFilePath = config->getOption(ConfigVal::SERVER_STORAGE_SQLITE_DROP_FILE);
    auto sql = GrbFile(sqlFilePath).readTextFile();
    auto&& myHash = stringHash(sql);

    EXPECT_EQ(myHash, std::dynamic_pointer_cast<SQLDatabase>(subject)->getHash(-1));
}

TEST_F(Sqlite3DatabaseTest, CheckUpgradeCommands)
{
    testUpgrade(ConfigVal::SERVER_STORAGE_SQLITE_UPGRADE_FILE);
}

class PlaylistSqliteConfig : public SqliteConfigFake {
public:
    explicit PlaylistSqliteConfig(fs::path path, bool transactions = false)
        : path(std::move(path))
        , transactions(transactions)
    {
    }

    std::string getOption(ConfigVal option) const override
    {
        if (option == ConfigVal::SERVER_STORAGE_SQLITE_DATABASE_FILE)
            return path.string();
        if (option == ConfigVal::SERVER_STORAGE_SQLITE_JOURNALMODE)
            return "DELETE";
        return SqliteConfigFake::getOption(option);
    }

    std::uint32_t getUIntOption(ConfigVal option) const override
    {
        return option == ConfigVal::SERVER_STORAGE_STRING_LIMIT ? 65535 : 0;
    }

    bool getBoolOption(ConfigVal option) const override
    {
        return option == ConfigVal::SERVER_STORAGE_USE_TRANSACTIONS ? transactions : SqliteConfigFake::getBoolOption(option);
    }

    std::shared_ptr<BoxLayoutList> getBoxLayoutListOption(ConfigVal) const override
    {
        return std::make_shared<BoxLayoutList>();
    }

private:
    fs::path path;
    bool transactions;
};

class PlaylistDatabaseTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        auto name = (fs::temp_directory_path() / "gerbera-playlist-test-XXXXXX").string();
        std::vector<char> buffer(name.begin(), name.end());
        buffer.push_back('\0');
        auto created = mkdtemp(buffer.data());
        ASSERT_NE(created, nullptr);
        directory = created;
        database = createDatabase();
        database->init();
    }

    void TearDown() override
    {
        if (database)
            database->shutdown();
        database.reset();
        if (!directory.empty())
            fs::remove_all(directory);
    }

    virtual std::shared_ptr<Database> createDatabase()
    {
        auto config = std::make_shared<PlaylistSqliteConfig>(directory / "gerbera.db");
        return TestDatabaseFactory::createInstance(config, nullptr, nullptr, nullptr);
    }

    std::shared_ptr<CdsItem> addFile(int number, bool video = false)
    {
        auto item = std::make_shared<CdsItem>(CdsEntryType::File);
        item->setTitle("Track " + std::to_string(number));
        item->setClass(video ? UPNP_CLASS_VIDEO_ITEM : UPNP_CLASS_MUSIC_TRACK);
        item->setMimeType(video ? "video/mp4" : "audio/flac");
        item->setParentID(CDS_ID_FS_ROOT);
        item->setLocation(directory / (std::to_string(number) + (video ? ".mp4" : ".flac")), CdsEntryType::File);
        auto resource = std::make_shared<CdsResource>(ContentHandler::DEFAULT, ResourcePurpose::Content);
        resource->addAttribute(ResourceAttribute::PROTOCOLINFO, "http-get:*:" + item->getMimeType() + ":*");
        item->addResource(resource);
        database->addObject(item, nullptr);
        return item;
    }

    int addContainer(const std::string& title)
    {
        auto container = std::make_shared<CdsContainer>(title);
        container->setParentID(CDS_ID_ROOT);
        container->setEntryType(CdsEntryType::VirtualContainer);
        database->addObject(container, nullptr);
        return container->getID();
    }

    int addReference(const std::shared_ptr<CdsItem>& file, int parent, int position, bool playlist = true)
    {
        auto item = std::make_shared<CdsItem>(CdsEntryType::VirtualItem);
        file->copyTo(item);
        item->setID(INVALID_OBJECT_ID);
        item->setEntryType(CdsEntryType::VirtualItem);
        item->setVirtual(true);
        item->setRefID(file->getID());
        item->setParentID(parent);
        item->setTrackNumber(position);
        item->setFlag(ObjectFlag::UseResourceReference);
        if (playlist)
            item->addMetaData(MetadataFields::M_CONTENT_CLASS, UPNP_CLASS_PLAYLIST_ITEM);
        database->addObject(item, nullptr);
        return item->getID();
    }

    fs::path directory;
    std::shared_ptr<Database> database;
};

TEST_F(PlaylistDatabaseTest, RepeatedFilesKeepTheirPositionsAndReferences)
{
    auto first = addFile(1);
    auto repeated = addFile(15, true);
    auto third = addFile(4);
    auto fourth = addFile(19, true);
    int playlist = addContainer("Mixed playlist");
    std::vector<std::shared_ptr<CdsItem>> files { first, repeated, third, fourth, repeated };
    std::vector<int> ids;
    for (std::size_t index = 0; index < files.size(); ++index)
        ids.push_back(addReference(files[index], playlist, index + 1));

    EXPECT_NE(ids[1], ids[4]);
    EXPECT_EQ(database->getChildCounts({ playlist }).at(playlist), 5);
    for (std::size_t index = 0; index < files.size(); ++index) {
        auto item = std::dynamic_pointer_cast<CdsItem>(database->loadObject(ids[index]));
        ASSERT_NE(item, nullptr);
        EXPECT_EQ(item->getRefID(), files[index]->getID());
        EXPECT_EQ(item->getTrackNumber(), index + 1);
        EXPECT_EQ(item->getTitle(), files[index]->getTitle());
        // Reimporting an existing occurrence must not create another entry.
        EXPECT_EQ(addReference(files[index], playlist, index + 1), ids[index]);
    }
    EXPECT_EQ(database->getChildCounts({ playlist }).at(playlist), 5);
    int otherPlaylist = addContainer("Other playlist");
    EXPECT_NE(addReference(repeated, otherPlaylist, 2), ids[1]);
}

TEST_F(PlaylistDatabaseTest, OrdinaryVirtualReferencesRemainDeduplicated)
{
    auto file = addFile(1);
    int parent = addContainer("Library view");
    int first = addReference(file, parent, 1, false);
    EXPECT_EQ(addReference(file, parent, 5, false), first);
    EXPECT_EQ(database->getChildCounts({ parent }).at(parent), 1);
}

TEST_F(PlaylistDatabaseTest, ReimportUpdatesMetadataWithoutLosingRepeatedPositions)
{
    auto file = addFile(1);
    file->addMetaData(MetadataFields::M_ALBUM, "Before");
    database->updateObject(file, nullptr);
    int parent = addContainer("Playlist");
    int first = addReference(file, parent, 1);
    int repeated = addReference(file, parent, 3);
    file->removeMetaData(MetadataFields::M_ALBUM);
    file->addMetaData(MetadataFields::M_ALBUM, "After");
    database->updateObject(file, nullptr);
    EXPECT_EQ(addReference(file, parent, 1), first);
    EXPECT_EQ(addReference(file, parent, 3), repeated);
    for (int id : { first, repeated }) {
        auto item = std::dynamic_pointer_cast<CdsItem>(database->loadObject(id));
        ASSERT_NE(item, nullptr);
        EXPECT_EQ(item->getRefID(), file->getID());
        EXPECT_EQ(item->getMetaData(MetadataFields::M_ALBUM), "After");
        EXPECT_EQ(item->getTrackNumber(), id == first ? 1 : 3);
    }
    EXPECT_EQ(database->getChildCounts({ parent }).at(parent), 2);
}

TEST_F(PlaylistDatabaseTest, UnpositionedPlaylistReferencesRemainDeduplicated)
{
    auto file = addFile(1);
    int parent = addContainer("Unpositioned playlist");
    int first = addReference(file, parent, 0);
    EXPECT_EQ(addReference(file, parent, 0), first);
    EXPECT_EQ(database->getChildCounts({ parent }).at(parent), 1);
}

#ifdef HAVE_PGSQL
class PlaylistPostgresConfig : public PlaylistSqliteConfig {
public:
    PlaylistPostgresConfig(const fs::path& path, bool transactions)
        : PlaylistSqliteConfig(path, transactions)
        , socket(URLUtils::urlEscape(std::getenv("GERBERA_TEST_PG_SOCKET")))
        , database(std::getenv("GERBERA_TEST_PG_DATABASE"))
        , user(std::getenv("GERBERA_TEST_PG_USER"))
        , port(stoiString(std::getenv("GERBERA_TEST_PG_PORT"), 0))
    {
    }

    std::string getOption(ConfigVal option) const override
    {
        switch (option) {
        case ConfigVal::SERVER_STORAGE_DRIVER:
            return DB_DRIVER_POSTGRES;
        case ConfigVal::SERVER_STORAGE_PGSQL_HOST:
            return socket;
        case ConfigVal::SERVER_STORAGE_PGSQL_DATABASE:
            return database;
        case ConfigVal::SERVER_STORAGE_PGSQL_USERNAME:
            return user;
        case ConfigVal::SERVER_STORAGE_PGSQL_INIT_SQL_FILE:
            return PG_INIT_FILE;
        case ConfigVal::SERVER_STORAGE_PGSQL_DROP_FILE:
            return PG_DROP_FILE;
        case ConfigVal::SERVER_STORAGE_PGSQL_UPGRADE_FILE:
            return PG_UPGR_FILE;
        default:
            return PlaylistSqliteConfig::getOption(option);
        }
    }

    std::int32_t getIntOption(ConfigVal option) const override
    {
        return option == ConfigVal::SERVER_STORAGE_PGSQL_PORT ? port : PlaylistSqliteConfig::getIntOption(option);
    }

private:
    std::string socket;
    std::string database;
    std::string user;
    int port;
};
#endif

enum class ForeignKeyBackend {
    Sqlite,
    Postgres
};

struct ForeignKeyRecoveryMode {
    ForeignKeyBackend backend;
    bool transactions;
};

class ForeignKeyRecoveryTest : public PlaylistDatabaseTest,
                               public ::testing::WithParamInterface<ForeignKeyRecoveryMode> {
protected:
    void SetUp() override
    {
#ifdef HAVE_PGSQL
        if (GetParam().backend == ForeignKeyBackend::Postgres) {
            const auto* socket = std::getenv("GERBERA_TEST_PG_SOCKET");
            const auto* database = std::getenv("GERBERA_TEST_PG_DATABASE");
            const auto* user = std::getenv("GERBERA_TEST_PG_USER");
            const auto* port = std::getenv("GERBERA_TEST_PG_PORT");
            if (!socket || !database || !user || !port) {
                GTEST_SKIP() << "PostgreSQL FK recovery requires isolated GERBERA_TEST_PG_SOCKET/DATABASE/USER/PORT";
            }
            ASSERT_TRUE(fs::path(socket).is_absolute());
            ASSERT_TRUE(fs::is_directory(socket));
            ASSERT_EQ(std::string(database), "gerbera_fk_test");
            ASSERT_EQ(std::string(user), "gerbera_fk_test");
            ASSERT_GT(stoiString(port, 0), 0);
            ASSERT_LE(stoiString(port, 0), 65535);
        }
#endif
        PlaylistDatabaseTest::SetUp();
    }

    std::shared_ptr<Database> createDatabase() override
    {
        std::shared_ptr<Config> config;
#ifdef HAVE_PGSQL
        if (GetParam().backend == ForeignKeyBackend::Postgres) {
            config = std::make_shared<PlaylistPostgresConfig>(directory / "gerbera.db", GetParam().transactions);
        } else
#endif
        {
            config = std::make_shared<PlaylistSqliteConfig>(directory / "gerbera.db", GetParam().transactions);
        }
        auto result = TestDatabaseFactory::createInstance(config, nullptr, nullptr, nullptr);
        if (GetParam().backend == ForeignKeyBackend::Postgres) {
            auto sql = std::dynamic_pointer_cast<SQLDatabase>(result);
            schema = "gerbera_fk_" + generateRandomId();
            sql->exec(fmt::format("CREATE SCHEMA {}", sql->identifier(schema)));
            sql->exec(fmt::format("SET search_path TO {}", sql->identifier(schema)));
        }
        return result;
    }

    void TearDown() override
    {
        if (database && !schema.empty()) {
            auto sql = std::dynamic_pointer_cast<SQLDatabase>(database);
            EXPECT_NO_THROW(sql->exec(fmt::format("DROP SCHEMA {} CASCADE", sql->identifier(schema))));
        }
        PlaylistDatabaseTest::TearDown();
    }

    static std::string referenceInsert(int ref, int parent)
    {
        return fmt::format("INSERT INTO \"mt_cds_object\" (\"ref_id\", \"parent_id\", \"object_type\", \"upnp_class\", \"dc_title\", \"entry_type\") VALUES ({}, {}, {}, 'object.container.playlistContainer', 'FK regression', {})",
            ref, parent, OBJECT_TYPE_CONTAINER, static_cast<int>(CdsEntryType::PlaylistContainer));
    }

    std::string schema;
};

TEST_P(ForeignKeyRecoveryTest, RemovedParentInsertThrowsAndDatabaseRemainsUsable)
{
    auto file = addFile(1);
    int parent = addContainer("Removed parent");
    ASSERT_GT(addFile(99)->getID(), parent);
    database->removeObject(parent, "", false);
    ASSERT_THROW(database->loadObject(parent), ObjectNotFoundException);
    auto sql = std::dynamic_pointer_cast<SQLDatabase>(database);

    EXPECT_THROW(sql->exec(referenceInsert(file->getID(), parent), "\"id\""), std::runtime_error);
    EXPECT_THROW(database->loadObject(parent), ObjectNotFoundException);

    auto next = addFile(2);
    int validParent = addContainer("After failed parent insert");
    int position = addReference(next, validParent, 1);
    auto saved = database->loadObject(position);
    EXPECT_EQ(saved->getRefID(), next->getID());
    EXPECT_EQ(database->getChildCounts({ validParent }).at(validParent), 1);
}

TEST_P(ForeignKeyRecoveryTest, RemovedReferenceInsertThrowsAndDatabaseRemainsUsable)
{
    auto removed = addFile(1);
    int parent = addContainer("Playlist");
    int removedID = removed->getID();
    ASSERT_GT(addFile(99)->getID(), removedID);
    database->removeObject(removedID, "", false);
    ASSERT_THROW(database->loadObject(removedID), ObjectNotFoundException);
    auto sql = std::dynamic_pointer_cast<SQLDatabase>(database);

    EXPECT_THROW(sql->exec(referenceInsert(removedID, parent), "\"id\""), std::runtime_error);
    EXPECT_THROW(database->loadObject(removedID), ObjectNotFoundException);
    EXPECT_EQ(database->getChildCounts({ parent }).count(parent), 0);

    auto next = addFile(2);
    int position = addReference(next, parent, 1);
    EXPECT_EQ(database->loadObject(position)->getRefID(), next->getID());
    EXPECT_EQ(database->getChildCounts({ parent }).at(parent), 1);
}

TEST_P(ForeignKeyRecoveryTest, RemovedReferenceUpdateLeavesOriginalAndDatabaseUsable)
{
    auto file = addFile(1);
    int parent = addContainer("Playlist");
    int position = addReference(file, parent, 1);
    auto removed = addFile(7);
    int removedID = removed->getID();
    ASSERT_GT(addFile(99)->getID(), removedID);
    database->removeObject(removedID, "", false);
    ASSERT_THROW(database->loadObject(removedID), ObjectNotFoundException);
    auto sql = std::dynamic_pointer_cast<SQLDatabase>(database);
    auto query = fmt::format("UPDATE \"mt_cds_object\" SET \"ref_id\" = {} WHERE \"id\" = {}", removedID, position);

    EXPECT_THROW(sql->execOnTable("mt_cds_object", query, position), std::runtime_error);
    EXPECT_EQ(database->loadObject(position)->getRefID(), file->getID());

    auto next = addFile(2);
    addReference(next, parent, 2);
    EXPECT_EQ(database->getChildCounts({ parent }).at(parent), 2);
}

TEST_P(ForeignKeyRecoveryTest, FailedStatementRollsBackAndAcceptsFollowingTransaction)
{
    auto removed = addFile(1);
    int removedID = removed->getID();
    ASSERT_GT(addFile(99)->getID(), removedID);
    database->removeObject(removedID, "", false);
    ASSERT_THROW(database->loadObject(removedID), ObjectNotFoundException);
    int parent = addContainer("Playlist");
    auto sql = std::dynamic_pointer_cast<SQLDatabase>(database);
    sql->beginTransaction("FK regression");
    const auto query = std::string("INSERT INTO \"mt_internal_setting\" (\"key\", \"value\") VALUES ('fk_rollback_marker', 'written'); ") + referenceInsert(removedID, parent);

    EXPECT_THROW(sql->exec(query, "\"id\""), std::runtime_error);
    // PostgreSQL work is atomic per statement task even without configured transactions.
    EXPECT_EQ(database->getInternalSetting("fk_rollback_marker"), GetParam().transactions || GetParam().backend == ForeignKeyBackend::Postgres ? "" : "written");

    auto next = addFile(2);
    int position = addReference(next, parent, 1);
    EXPECT_EQ(database->loadObject(position)->getRefID(), next->getID());
    EXPECT_EQ(database->getChildCounts({ parent }).at(parent), 1);
}

INSTANTIATE_TEST_SUITE_P(BackendsAndTransactions, ForeignKeyRecoveryTest,
    ::testing::Values(ForeignKeyRecoveryMode { ForeignKeyBackend::Sqlite, false }, ForeignKeyRecoveryMode { ForeignKeyBackend::Sqlite, true }
#ifdef HAVE_PGSQL
        ,
        ForeignKeyRecoveryMode { ForeignKeyBackend::Postgres, false }, ForeignKeyRecoveryMode { ForeignKeyBackend::Postgres, true }
#endif
        ),
    [](const ::testing::TestParamInfo<ForeignKeyRecoveryMode>& info) {
        return std::string(info.param.backend == ForeignKeyBackend::Postgres ? "Postgres" : "Sqlite") + (info.param.transactions ? "Transactions" : "NoTransactions");
    });

// test is blocking on CONAN
#if 0
class DatabaseTest : public DatabaseTestBase {

public:
    DatabaseTest() {};
    virtual ~DatabaseTest() {};

    virtual void SetUp()
    {
        config = std::make_shared<SqliteConfigFake>();
        subject = std::make_shared<Sqlite3Database>(config, nullptr, nullptr);
    }

    virtual void TearDown()
    {
    }
};

TEST_F(DatabaseTest, AddObject)
{
    subject->init();
    auto cds = std::make_shared<CdsItem>();
    cds->setTrackNumber(854);
    cds->setMetadata(M_ARTIST, "Your Mother");
    cds->setMetadata(M_ALBUM, "Wombats");
    cds->setLocation(fs::path("/mnt/basilisk/Music/Adele/21/01 - Adele - Rolling In The Deep.flac"));
    int number = 1;

    subject->addObject(cds, &number);
    subject->shutdown();
}
#endif

#ifdef HAVE_MYSQL

class MysqlDatabaseTest : public DatabaseTestBase {

public:
    MysqlDatabaseTest() = default;
    ~MysqlDatabaseTest() override = default;

    void SetUp() override
    {
        config = std::make_shared<MySQLConfigFake>();
        subject = std::make_shared<MySQLDatabase>(config, nullptr, nullptr);
    }

    void TearDown() override
    {
    }
};

TEST_F(MysqlDatabaseTest, CheckUpgradeCommands)
{
    testUpgrade(ConfigVal::SERVER_STORAGE_MYSQL_UPGRADE_FILE);
}

TEST_F(MysqlDatabaseTest, CheckInitScript)
{
    auto sqlFilePath = config->getOption(ConfigVal::SERVER_STORAGE_MYSQL_INIT_SQL_FILE);
    auto sql = GrbFile(sqlFilePath).readTextFile();
    auto&& myHash = stringHash(sql);

    EXPECT_EQ(myHash, std::dynamic_pointer_cast<SQLDatabase>(subject)->getHash(0));
}

TEST_F(MysqlDatabaseTest, CheckDropScript)
{
    auto sqlFilePath = config->getOption(ConfigVal::SERVER_STORAGE_MYSQL_DROP_FILE);
    auto sql = GrbFile(sqlFilePath).readTextFile();
    auto&& myHash = stringHash(sql);

    EXPECT_EQ(myHash, std::dynamic_pointer_cast<SQLDatabase>(subject)->getHash(-1));
}
#endif

#ifdef HAVE_PGSQL

class PostgresDatabaseTest : public DatabaseTestBase {

public:
    PostgresDatabaseTest() = default;
    ~PostgresDatabaseTest() override = default;

    void SetUp() override
    {
        config = std::make_shared<PostgresConfigFake>();
        subject = std::make_shared<PostgresDatabase>(config, nullptr, nullptr);
    }

    void TearDown() override
    {
    }
};

TEST_F(PostgresDatabaseTest, CheckUpgradeCommands)
{
    testUpgrade(ConfigVal::SERVER_STORAGE_PGSQL_UPGRADE_FILE);
}

TEST_F(PostgresDatabaseTest, CheckInitScript)
{
    auto sqlFilePath = config->getOption(ConfigVal::SERVER_STORAGE_PGSQL_INIT_SQL_FILE);
    auto sql = GrbFile(sqlFilePath).readTextFile();
    auto&& myHash = stringHash(sql);

    EXPECT_EQ(myHash, std::dynamic_pointer_cast<SQLDatabase>(subject)->getHash(0));
}

TEST_F(PostgresDatabaseTest, CheckDropScript)
{
    auto sqlFilePath = config->getOption(ConfigVal::SERVER_STORAGE_PGSQL_DROP_FILE);
    auto sql = GrbFile(sqlFilePath).readTextFile();
    auto&& myHash = stringHash(sql);

    EXPECT_EQ(myHash, std::dynamic_pointer_cast<SQLDatabase>(subject)->getHash(-1));
}
#endif
