/*GRB*

    Gerbera - https://gerbera.io/

    test_configmanager.cc - this file is part of Gerbera.

    Copyright (C) 2016-2025 Gerbera Contributors

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

#include "config/config_definition.h"
#include "config/config_generator.h"
#include "config/config_manager.h"
#include "config/config_val.h"
#include "exceptions.h"
#include "util/tools.h"

#include <array>
#include <fstream>
#include <gtest/gtest.h>
#include <memory>

class ConfigManagerTest : public ::testing::Test {

public:
    ConfigManagerTest() = default;

    ~ConfigManagerTest() override = default;

    void SetUp() override
    {
        definition = std::make_shared<ConfigDefinition>();
        definition->init(definition);
        gerberaDir = createTempPath();

        fs::path grbWeb = gerberaDir / "web";
        createDirectory(grbWeb);

        fs::path grbJs = gerberaDir / "js";
        createDirectory(grbJs);

        fs::path configDir = gerberaDir / ".config";
        createDirectory(configDir);

        // Create mock files, allowing for .init()
        auto mockFiles = std::array {
            grbJs / "common.js",
            grbJs / "import.js",
            grbJs / "playlists.js",
            grbJs / "metadata.js",
            gerberaDir / "sqlite3.sql",
            gerberaDir / "sqlite3-upgrade.xml",
            gerberaDir / "mysql.sql",
            gerberaDir / "mysql-upgrade.xml",
        };
        std::ofstream file;
        for (auto&& mFile : mockFiles) {
            file.open(mFile);
            file.close();
        }

        configFile = configDir / "config.xml";
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
    }

    static fs::path createTempPath()
    {
        fs::path ss = fs::path(CMAKE_BINARY_DIR) / "test" / "config" / generateRandomId();
        createDirectory(ss);
        return ss;
    }

    static void createDirectory(const fs::path& dir)
    {
        if (mkdir(dir.c_str(), 0777) < 0) {
            throw_std_runtime_error("Failed to create test_config temporary directory for testing: {}", dir.c_str());
        }
    }

    std::string createConfig() const
    {
        ConfigGenerator configGenerator(definition, GERBERA_TEST, false);
        return configGenerator.generate(home, confdir, prefix, magic);
    }

    void TearDown() override
    {
        delete subject;
        fs::remove_all(gerberaDir);
    }

    fs::path gerberaDir;
    fs::path configFile;
    fs::path home;
    fs::path confdir;
    fs::path prefix;
    fs::path magic;
    ConfigManager* subject;
    std::shared_ptr<ConfigDefinition> definition;
};

TEST_F(ConfigManagerTest, LoadsWebUIDefaultValues)
{
    auto shared = std::make_shared<ConfigManager>(definition, configFile, home, confdir, prefix, false);
    shared->load(home);

    ASSERT_TRUE(shared->getBoolOption(ConfigVal::SERVER_UI_ENABLED));
    ASSERT_TRUE(shared->getBoolOption(ConfigVal::SERVER_UI_SHOW_TOOLTIPS));
    ASSERT_FALSE(shared->getBoolOption(ConfigVal::SERVER_UI_ACCOUNTS_ENABLED));
    ASSERT_EQ(30, shared->getIntOption(ConfigVal::SERVER_UI_SESSION_TIMEOUT));
}

TEST_F(ConfigManagerTest, ThrowsExceptionWhenMissingConfigFileAndNoDefault)
{
    std::ostringstream expErrMsg;
    fs::path notExistsDir = home / "not_exists";
    fs::path configFile = fs::weakly_canonical(notExistsDir / confdir / "config.xml");

    expErrMsg << "\nThe server configuration file could not be found: ";
    expErrMsg << configFile.string() << "\n";
    expErrMsg << "Gerbera could not find a default configuration file.\n";
    expErrMsg << "Try specifying an alternative configuration file on the command line.\n";
    expErrMsg << "For a list of options run: gerbera -h\n";

    configFile = "";

    try {
        auto shared = std::make_shared<ConfigManager>(definition, configFile, notExistsDir, confdir, prefix, false);
        shared->load(notExistsDir);
    } catch (const std::runtime_error& err) {
        EXPECT_EQ(err.what(), expErrMsg.str());
    }
}

TEST_F(ConfigManagerTest, LoadsConfigFromDefaultHomeWhenExistsButNotSpecified)
{
    configFile = "";
    auto shared = std::make_shared<ConfigManager>(definition, configFile, home, confdir, prefix, false);
    shared->load(home);

    ASSERT_TRUE(shared->getBoolOption(ConfigVal::SERVER_UI_ENABLED));
    ASSERT_TRUE(shared->getBoolOption(ConfigVal::SERVER_UI_SHOW_TOOLTIPS));
    ASSERT_FALSE(shared->getBoolOption(ConfigVal::SERVER_UI_ACCOUNTS_ENABLED));
    ASSERT_EQ(30, shared->getIntOption(ConfigVal::SERVER_UI_SESSION_TIMEOUT));
}
