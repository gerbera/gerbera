#include "../mock/config_mock.h"
#include <content/content_manager.h>
#include <database/sqlite3/sqlite_database.h>
#include <gtest/gtest.h>

using namespace ::testing;

class ContentManagerTest : public ::testing::Test {

public:
    ContentManagerTest() = default;
    ~ContentManagerTest() = default;

    std::shared_ptr<ContentManager> subject;
    std::string tmp;
    std::string db_loc;

    virtual void SetUp()
    {
        spdlog::set_pattern("%Y-%m-%d %X.%e %6l: [%s:%#] %!(): %v");
        spdlog::set_level(spdlog::level::debug);

        tmp = fmt::format("{}grb-test", testing::TempDir());
        db_loc = testing::TempDir() + "grb_test_sqlite.db";
        log_info("tmp dir is: {}, db loc: {}", tmp, db_loc);


        auto config = std::make_shared<ConfigMock>();
        fs::path initSql = CMAKE_CURRENT_SOURCE_DIR;
        initSql /= "../../src/database/sqlite3/sqlite3.sql";
        auto database = std::make_shared<Sqlite3Database>(nullptr, db_loc, initSql, MT_SQLITE_SYNC_OFF, false, false, 0);
        database->init();

        auto context = std::make_shared<Context>(config, nullptr, nullptr, database, nullptr, nullptr);
        subject = std::make_shared<ContentManager>(context, nullptr, nullptr);

        subject->run();

        auto data = splitString(readTextFile(fs::path(fmt::format("{}/data/pokemon.txt", CMAKE_CURRENT_SOURCE_DIR))), '\n');

        int count = 0;
        for (const auto& d: data) {
            fs::path base;
            base /= tmp;
            base /= d;
            for (const auto& e: data) {
                fs::path next = base;
                next += "-";
                next += e;
                count++;
                //log_debug("Creating path: {}", next.u8string());
                fs::create_directories(next);
            }
        }
        log_info("Created {} folders!", count);
    }

    virtual void TearDown()
    {
        log_info("Removing test folders....");
        fs::remove_all(tmp);
        log_info("Removing db....");
        fs::remove_all(db_loc);
    };
};

TEST_F(ContentManagerTest, directoryScan)
{
    auto autoScan = std::make_shared<AutoscanDirectory>();
    autoScan->setLocation(tmp);

    auto future = subject->rescanDirectory(autoScan, INVALID_OBJECT_ID);
    log_info("waiting for 1 minute....");
    future.wait_for(std::chrono::minutes(1));
}