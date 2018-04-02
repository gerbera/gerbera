#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include <uuid/uuid.h>
#include <fstream>

#include <config_manager.h>

using namespace zmm;
using namespace mxml;

class ConfigManagerTest : public ::testing::Test {

public:

    ConfigManagerTest() {};

    virtual ~ConfigManagerTest() {};

    virtual void SetUp() {
        std::string gerberaDir = createTempPath();

        std::string grbWeb = gerberaDir + DIR_SEPARATOR + "web";
        create_directory(grbWeb);

        std::string grbJs = gerberaDir + DIR_SEPARATOR + "js";
        create_directory(grbJs);

        // Create mock files
        std::string mockFiles[3] = {
                grbJs + DIR_SEPARATOR + "common.js",
                grbJs + DIR_SEPARATOR + "import.js",
                grbJs + DIR_SEPARATOR + "playlists.js"
        };
        for (int i = 0; i < 3; i++){
            std::ofstream file;
            file.open(mockFiles[i]);
            file.close();
        }

        config_file = nullptr;  // require file to be generated
        home = gerberaDir;
        prefix = gerberaDir;
        magic = "";
        confdir = ".config";

        printf("test_config temporary path --> %s\n", gerberaDir.c_str());
        subject = new ConfigManager();
        subject->setStaticArgs(config_file, _(home.c_str()), _(confdir.c_str()), _(prefix.c_str()), _(magic.c_str()));
        subject->init();
    };

    std::string createTempPath() {
        uuid_t uuid;
#ifdef BSD_NATIVE_UUID
        char *uuid_str;
        uint32_t status;
        uuid_create(&uuid, &status);
        uuid_to_string(&uuid, &uuid_str, &status);
#else
        char uuid_str[37];
        uuid_generate(uuid);
        uuid_unparse(uuid, uuid_str);
#endif

        std::stringstream ss;
        ss << CMAKE_BINARY_DIR << DIR_SEPARATOR << "test"<< DIR_SEPARATOR << "test_config" << DIR_SEPARATOR << uuid_str;

        create_directory(ss.str());
        return ss.str();
    }

    void create_directory(std::string dir) {
        if (mkdir(dir.c_str(), 0777) < 0) {
            throw std::runtime_error("Failed to create test_config temporary directory for testing");
        };
    }

    virtual void TearDown() {
        delete subject;
    };

    std::string gerberaDir;
    String config_file;
    std::string home;
    std::string confdir;
    std::string prefix;
    std::string magic;
    ConfigManager *subject;
};

TEST_F(ConfigManagerTest, WebUIDefaultValues) {
    ASSERT_TRUE(subject->getBoolOption(CFG_SERVER_UI_ENABLED));
    ASSERT_TRUE(subject->getBoolOption(CFG_SERVER_UI_SHOW_TOOLTIPS));
    ASSERT_FALSE(subject->getBoolOption(CFG_SERVER_UI_ACCOUNTS_ENABLED));
    ASSERT_EQ(30, subject->getIntOption(CFG_SERVER_UI_SESSION_TIMEOUT));
}

