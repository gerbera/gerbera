#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include <array>
#include <fstream>
#include <ftw.h>

#include "config/config_generator.h"
#include "config/config_manager.h"
#include "util/tools.h"

class ConfigManagerTest : public ::testing::Test {

public:
    ConfigManagerTest() {};

    virtual ~ConfigManagerTest() {};

    virtual void SetUp()
    {
        gerberaDir = createTempPath();

        fs::path grbWeb = gerberaDir / "web";
        create_directory(grbWeb);

        fs::path grbJs = gerberaDir / "js";
        create_directory(grbJs);

        fs::path configDir = gerberaDir / ".config";
        create_directory(configDir);

        // Create mock files, allowing for .init()
        fs::path mockFiles[3] = {
            grbJs / "common.js",
            grbJs / "import.js",
            grbJs / "playlists.js"
        };
        std::ofstream file;
        for (int i = 0; i < 3; i++) {
            file.open(mockFiles[i]);
            file.close();
        }

        config_file = configDir / "config.xml";
        home = gerberaDir;
        prefix = gerberaDir;
        magic = "";
        confdir = ".config";

        // Create config using generator
        std::string cfgContent = createConfig();
        file.open(configDir / "config.xml");
        file << cfgContent;
        file.close();

        subject = nullptr;
    };

    fs::path createTempPath()
    {
        fs::path ss = fs::path(CMAKE_BINARY_DIR) / "test" / "config" / generate_random_id();
        create_directory(ss);
        return ss;
    }

    void create_directory(fs::path dir)
    {
        if (mkdir(dir.c_str(), 0777) < 0) {
            throw_std_runtime_error("Failed to create test_config temporary directory for testing");
        };
    }

    std::string createConfig()
    {
        ConfigGenerator configGenerator;
        return configGenerator.generate(home, confdir, prefix, magic);
    }

    virtual void TearDown()
    {
        if (subject)
            delete subject;
        fs::remove_all(gerberaDir);
    };

    fs::path gerberaDir;
    fs::path config_file;
    fs::path home;
    fs::path confdir;
    fs::path prefix;
    fs::path magic;
    ConfigManager* subject;
};

TEST_F(ConfigManagerTest, LoadsWebUIDefaultValues)
{
    subject = new ConfigManager(config_file, home, confdir, prefix, magic, "", "", 0, false);

    ASSERT_TRUE(subject->getBoolOption(CFG_SERVER_UI_ENABLED));
    ASSERT_TRUE(subject->getBoolOption(CFG_SERVER_UI_SHOW_TOOLTIPS));
    ASSERT_FALSE(subject->getBoolOption(CFG_SERVER_UI_ACCOUNTS_ENABLED));
    ASSERT_EQ(30, subject->getIntOption(CFG_SERVER_UI_SESSION_TIMEOUT));
}

TEST_F(ConfigManagerTest, ThrowsExceptionWhenMissingConfigFileAndNoDefault)
{
    std::ostringstream expErrMsg;
    fs::path notExistsDir = home / "not_exists";
    fs::path configFile = notExistsDir / confdir / "config.xml";

    expErrMsg << "\nThe server configuration file could not be found: ";
    expErrMsg << configFile << "\n";
    expErrMsg << "Gerbera could not find a default configuration file.\n";
    expErrMsg << "Try specifying an alternative configuration file on the command line.\n";
    expErrMsg << "For a list of options run: gerbera -h\n";

    config_file = "";

    try {
        subject = new ConfigManager(config_file, notExistsDir, confdir, prefix, magic, "", "", 0, false);
    } catch (const std::runtime_error& err) {
        EXPECT_EQ(err.what(), expErrMsg.str());
    }
}

TEST_F(ConfigManagerTest, LoadsConfigFromDefaultHomeWhenExistsButNotSpecified)
{
    config_file = "";
    subject = new ConfigManager(config_file, home, confdir, prefix, magic, "", "", 0, false);

    ASSERT_TRUE(subject->getBoolOption(CFG_SERVER_UI_ENABLED));
    ASSERT_TRUE(subject->getBoolOption(CFG_SERVER_UI_SHOW_TOOLTIPS));
    ASSERT_FALSE(subject->getBoolOption(CFG_SERVER_UI_ACCOUNTS_ENABLED));
    ASSERT_EQ(30, subject->getIntOption(CFG_SERVER_UI_SESSION_TIMEOUT));
}
