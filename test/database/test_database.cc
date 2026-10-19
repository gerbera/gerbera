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
#include "database/sqlite3/sqlite_database.h"
#include "exceptions.h"
#include "sqlite_config_fake.h"
#include "upnp/xml_builder.h"
#include "util/tools.h"

#ifdef HAVE_MYSQL
#include "database/mysql/mysql_database.h"
#include "mysql_config_fake.h"
#endif

#ifdef HAVE_PGSQL
#include "database/postgres/postgres_database.h"
#include "postgres_config_fake.h"
#endif

#include <cstdlib>
#include <gtest/gtest.h>
#include <pugixml.hpp>

class TestSqliteDatabase : public Sqlite3Database {
    friend class Sqlite3DatabaseTest;
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
    explicit PlaylistSqliteConfig(fs::path path)
        : path(std::move(path))
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

    std::shared_ptr<BoxLayoutList> getBoxLayoutListOption(ConfigVal) const override
    {
        return std::make_shared<BoxLayoutList>();
    }

private:
    fs::path path;
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
        auto config = std::make_shared<PlaylistSqliteConfig>(directory / "gerbera.db");
        database = std::make_shared<Sqlite3Database>(config, nullptr, nullptr, nullptr);
        database->run();
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
