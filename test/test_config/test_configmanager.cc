#include <array>
#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include <uuid/uuid.h>
#include <fstream>

#include "config/config_manager.h"
#include "config/config_generator.h"
#include "util/exception.h"

using namespace zmm;
using namespace mxml;

class ConfigManagerTest : public ::testing::Test {

 public:

  ConfigManagerTest() {};

  virtual ~ConfigManagerTest() {};

  virtual void SetUp() {
    gerberaDir = createTempPath();

    std::string grbWeb = gerberaDir + DIR_SEPARATOR + "web";
    create_directory(grbWeb);

    std::string grbJs = gerberaDir + DIR_SEPARATOR + "js";
    create_directory(grbJs);

    std::string configDir = gerberaDir + DIR_SEPARATOR + ".config";
    create_directory(configDir);

    // Create mock files, allowing for .init()
    std::string mockFiles[3] = {
        grbJs + DIR_SEPARATOR + "common.js",
        grbJs + DIR_SEPARATOR + "import.js",
        grbJs + DIR_SEPARATOR + "playlists.js"
    };
    std::ofstream file;
    for (int i = 0; i < 3; i++) {
      file.open(mockFiles[i]);
      file.close();
    }

    config_file = configDir + DIR_SEPARATOR + "config.xml";
    home = gerberaDir;
    prefix = gerberaDir;
    magic = "";
    confdir = ".config";

    // Create config using generator
    std::string cfgContent = createConfig();
    file.open(configDir + DIR_SEPARATOR + "config.xml");
    file << cfgContent;
    file.close();

    subject = nullptr;
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
    ss << CMAKE_BINARY_DIR << DIR_SEPARATOR << "test" << DIR_SEPARATOR << "test_config" << DIR_SEPARATOR << uuid_str;

    create_directory(ss.str());
    return ss.str();
  }

  void create_directory(std::string dir) {
    if (mkdir(dir.c_str(), 0777) < 0) {
      throw std::runtime_error("Failed to create test_config temporary directory for testing");
    };
  }

  std::string createConfig() {
    ConfigGenerator configGenerator;
    return configGenerator.generate(std::string(home.c_str()), std::string(confdir.c_str()), std::string(prefix.c_str()), magic);
  }

  virtual void TearDown() {
    if (subject)
      delete subject;
  };

  std::string gerberaDir;
  std::string config_file;
  std::string home;
  std::string confdir;
  std::string prefix;
  std::string magic;
  ConfigManager *subject;
};

TEST_F(ConfigManagerTest, LoadsWebUIDefaultValues) {
  subject = new ConfigManager(config_file, home, confdir, prefix, magic, "", "", 0, false);

  ASSERT_TRUE(subject->getBoolOption(CFG_SERVER_UI_ENABLED));
  ASSERT_TRUE(subject->getBoolOption(CFG_SERVER_UI_SHOW_TOOLTIPS));
  ASSERT_FALSE(subject->getBoolOption(CFG_SERVER_UI_ACCOUNTS_ENABLED));
  ASSERT_EQ(30, subject->getIntOption(CFG_SERVER_UI_SESSION_TIMEOUT));
}

TEST_F(ConfigManagerTest, ThrowsExceptionWhenMissingConfigFileAndNoDefault) {
  std::ostringstream expErrMsg;
  std::string notExistsDir = home + DIR_SEPARATOR + "not_exists";

  expErrMsg << "\nThe server configuration file could not be found in: ";
  expErrMsg << notExistsDir << DIR_SEPARATOR << confdir << "\n";
  expErrMsg << "Gerbera could not find a default configuration file.\n";
  expErrMsg << "Try specifying an alternative configuration file on the command line.\n";
  expErrMsg << "For a list of options run: gerbera -h\n";

  config_file = "";

  try {
    subject = new ConfigManager(config_file, notExistsDir, confdir, prefix, magic, "", "", 0, false);
  } catch(Exception const & err) {
    EXPECT_EQ(err.getMessage(), expErrMsg.str());
  }
}

TEST_F(ConfigManagerTest, LoadsConfigFromDefaultHomeWhenExistsButNotSpecified) {
  config_file = "";
  subject = new ConfigManager(config_file, home, confdir, prefix, magic, "", "", 0, false);

  ASSERT_TRUE(subject->getBoolOption(CFG_SERVER_UI_ENABLED));
  ASSERT_TRUE(subject->getBoolOption(CFG_SERVER_UI_SHOW_TOOLTIPS));
  ASSERT_FALSE(subject->getBoolOption(CFG_SERVER_UI_ACCOUNTS_ENABLED));
  ASSERT_EQ(30, subject->getIntOption(CFG_SERVER_UI_SESSION_TIMEOUT));
}
