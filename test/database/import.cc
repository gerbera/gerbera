/*GRB*

Gerbera - https://gerbera.io/

    import.cc.c - this file is part of Gerbera.

    Copyright (C) 2020 Gerbera Contributors

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

/// \file import.cc.c
#include "../mock/config_mock.h"
#include "cds_objects.h"
#include "database/mysql/mysql_database.h"
#include "mysql_config_fake.h"
#include <gtest/gtest.h>

using namespace ::testing;

class DatabaseTest : public ::testing::Test {

public:
    DatabaseTest() {};
    virtual ~DatabaseTest() {};

    virtual void SetUp()
    {
        config = std::make_shared<MySQLConfigFake>();
        subject = Database::createInstance(config, nullptr , "mysql");
    }

    virtual void TearDown()
    {
        //delete subject;
    };

    std::shared_ptr<Database> subject;
    std::shared_ptr<Config> config;
};


TEST_F(DatabaseTest, AddObject)
{
    auto cds = std::make_shared<CdsItem>();
    cds->setTrackNumber(854);
    cds->setMetadata(M_ARTIST, "Your Mother");
    cds->setMetadata(M_ALBUM, "Wombats");
    cds->setLocation(fs::path("/mnt/basilisk/Music/Adele/21/01 - Adele - Rolling In The Deep.flac"));
    int number = 1;

    spdlog::set_level(spdlog::level::debug);
    auto start_time = std::chrono::high_resolution_clock::now();
    //for(int i = 0; i<1000;i++) {
        subject->addObject(cds, &number);
    //}
    auto end_time = std::chrono::high_resolution_clock::now();

    log_info("took {}ms to add 1 item", std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count() );
}
