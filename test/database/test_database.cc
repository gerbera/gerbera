/*GRB*

Gerbera - https://gerbera.io/

    test_database.cc - this file is part of Gerbera.

    Copyright (C) 2020-2021 Gerbera Contributors

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

/// \file test_database.cc
#include <gtest/gtest.h>
#include <pugixml.hpp>

#include "cds_objects.h"
#include "database/sqlite3/sqlite_database.h"
#include "sqlite_config_fake.h"
#include "upnp_xml.h"

#if HAVE_MYSQL
#include "database/mysql/mysql_database.h"
#include "mysql_config_fake.h"
#endif

class TestSqliteDatabase : public Sqlite3Database {
    friend class Sqlite3DatabaseTest;
};

class DatabaseTestBase : public ::testing::Test {

protected:
    void testUpgrade(config_option_t option);
    std::shared_ptr<Database> subject;
    std::shared_ptr<Config> config;
};

void DatabaseTestBase::testUpgrade(config_option_t option)
{
    const fs::path& upgradeFile = config->getOption(option);
    pugi::xml_document xmlDoc;
    pugi::xml_parse_result result = xmlDoc.load_file(upgradeFile.c_str());
    if (result.status != pugi::xml_parse_status::status_ok) {
        throw ConfigParseException(result.description());
    }
    auto root = xmlDoc.document_element();
    EXPECT_TRUE(root.name() == std::string_view("upgrade"));

    size_t version = 1;
    for (auto&& versionElement : root.select_nodes("/upgrade/version")) {
        const pugi::xml_node& versionNode = versionElement.node();
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
        subject = std::make_shared<Sqlite3Database>(config, nullptr, nullptr);
    }

    void TearDown() override
    {
        subject = nullptr;
    }
};

TEST_F(Sqlite3DatabaseTest, CheckInitScript)
{
    auto sqlFilePath = config->getOption(CFG_SERVER_STORAGE_SQLITE_INIT_SQL_FILE);
    auto sql = readTextFile(sqlFilePath);
    auto&& myHash = stringHash(sql);

    EXPECT_EQ(myHash, std::dynamic_pointer_cast<SQLDatabase>(subject)->getHash(0));
}

TEST_F(Sqlite3DatabaseTest, CheckUpgradeCommands)
{
    testUpgrade(CFG_SERVER_STORAGE_SQLITE_UPGRADE_FILE);
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
        subject = std::make_shared<MySQLDatabase>(config, nullptr);
    }

    void TearDown() override
    {
    }
};

TEST_F(MysqlDatabaseTest, CheckUpgradeCommands)
{
    testUpgrade(CFG_SERVER_STORAGE_MYSQL_UPGRADE_FILE);
}

TEST_F(MysqlDatabaseTest, CheckInitScript)
{
    auto sqlFilePath = config->getOption(CFG_SERVER_STORAGE_MYSQL_INIT_SQL_FILE);
    auto sql = readTextFile(sqlFilePath);
    auto&& myHash = stringHash(sql);

    EXPECT_EQ(myHash, std::dynamic_pointer_cast<SQLDatabase>(subject)->getHash(0));
}
#endif
