#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include <config_manager.h>
#include <config/config_generator.h>
#include <fstream>

using namespace ::testing;

class ConfigGeneratorTest : public ::testing::Test {

 public:
  ConfigGeneratorTest() {};
  virtual ~ConfigGeneratorTest() {};

  virtual void SetUp() {
    subject = new ConfigGenerator();
    homePath = "/tmp";
    configDir = ".config/gerbera";
    prefixDir = "/usr/local/share/gerbera";
    magicFile = "magic.file";
    mockConfig = mockConfigXml("fixtures/mock-config-all.xml");
  }

  virtual void TearDown() {
    delete subject;
  };

  std::string mockConfigXml(std::string mockFile) {
    std::ifstream t(mockFile);
    std::string str((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
    return str;
  }

  ConfigGenerator *subject;
  std::string homePath;
  std::string configDir;
  std::string prefixDir;
  std::string magicFile;
  std::string mockConfig;
};

TEST_F(ConfigGeneratorTest, GeneratesStringForConfiguration) {
  std::string defaultConfig = subject->generate(homePath, configDir, prefixDir, magicFile);

  ASSERT_NE(nullptr, defaultConfig.c_str());
  EXPECT_THAT(defaultConfig.c_str(), HasSubstr("<home>/tmp/.config/gerbera</home>"));
  EXPECT_THAT(defaultConfig.c_str(), HasSubstr("<webroot>/usr/local/share/gerbera/web</webroot>"));
  EXPECT_THAT(defaultConfig.c_str(), HasSubstr("<protocolInfo extend=\"no\"/>"));
  EXPECT_THAT(defaultConfig.c_str(), HasSubstr("<extended-runtime-options>"));
  EXPECT_THAT(defaultConfig.c_str(), HasSubstr("<import hidden-files=\"no\">"));
  EXPECT_THAT(defaultConfig.c_str(), HasSubstr("<scripting script-charset=\"UTF-8\">"));
  EXPECT_THAT(defaultConfig.c_str(), HasSubstr("<import-script>/usr/local/share/gerbera/js/import.js</import-script>"));
  EXPECT_THAT(defaultConfig.c_str(), HasSubstr("<common-script>/usr/local/share/gerbera/js/common.js</common-script>"));
  EXPECT_THAT(defaultConfig.c_str(), HasSubstr("<playlist-script>/usr/local/share/gerbera/js/playlists.js</playlist-script>"));
  EXPECT_THAT(defaultConfig.c_str(), HasSubstr("<mappings>"));
  EXPECT_THAT(defaultConfig.c_str(), HasSubstr("<extension-mimetype ignore-unknown=\"no\">"));
  EXPECT_THAT(defaultConfig.c_str(), HasSubstr("<mimetype-upnpclass>"));
  EXPECT_THAT(defaultConfig.c_str(), HasSubstr("<mimetype-contenttype>"));
  EXPECT_THAT(defaultConfig.c_str(), HasSubstr("<online-content>"));
  EXPECT_STREQ(mockConfig.c_str(), defaultConfig.c_str());
}

#ifdef HAVE_SQLITE3
TEST_F(ConfigGeneratorTest, GeneratesSqlLiteWhenEnabled) {
  std::string defaultConfig = subject->generate(homePath, configDir, prefixDir, magicFile);

  EXPECT_THAT(defaultConfig.c_str(), HasSubstr("<sqlite3 enabled=\"yes\">\n        <database-file>gerbera.db</database-file>\n      </sqlite3>"));
}
#endif

#ifdef HAVE_MYSQL
TEST_F(ConfigGeneratorTest, GeneratesMySqlWhenEnabled) {
  std::string defaultConfig = subject->generate(homePath, configDir, prefixDir, magicFile);

  EXPECT_THAT(defaultConfig.c_str(), HasSubstr("<mysql enabled=\"no\">\n        <host>localhost</host>\n        <username>gerbera</username>\n        <database>gerbera</database>\n      </mysql>"));
}
#endif


#if defined(HAVE_FFMPEG) && defined(HAVE_FFMPEGTHUMBNAILER)
TEST_F(ConfigGeneratorTest, GeneratesFFMPEGThumbnailer) {
  std::string defaultConfig = subject->generate(homePath, configDir, prefixDir, magicFile);

  EXPECT_THAT(defaultConfig.c_str(), HasSubstr("<ffmpegthumbnailer enabled=\"no\">"));
}
#endif

#ifdef HAVE_MAGIC
TEST_F(ConfigGeneratorTest, GeneratesMagicFile) {
  std::string defaultConfig = subject->generate(homePath, configDir, prefixDir, magicFile);

  EXPECT_THAT(defaultConfig.c_str(), HasSubstr("<magic-file>magic.file</magic-file>"));
}

TEST_F(ConfigGeneratorTest, DoesNotGenerateMagicIfNotPresent) {
  magicFile = "";
  mockConfig = mockConfigXml("fixtures/mock-config-no-magic.xml");
  std::string defaultConfig = subject->generate(homePath, configDir, prefixDir, magicFile);

  EXPECT_THAT(defaultConfig.c_str(), Not(HasSubstr("<magic-file>magic.file</magic-file>")));
}
#endif

#ifndef HAVE_JS
TEST_F(ConfigGeneratorTest, GeneratesBuiltInScriptImportWhenJSdisabled) {
  mockConfig = mockConfigXml("fixtures/mock-config-import-builtin.xml");
  std::string defaultConfig = subject->generate(homePath, configDir, prefixDir, magicFile);

  EXPECT_THAT(defaultConfig.c_str(), HasSubstr("<scripting script-charset=\"UTF-8\">\n      <virtual-layout type=\"builtin\"/>\n    </scripting>"));
}
#endif

#ifdef HAVE_JS
TEST_F(ConfigGeneratorTest, GeneratesImportWhenJSenabled) {
  mockConfig = mockConfigXml("fixtures/mock-config-import-js.xml");
  std::string defaultConfig = subject->generate(homePath, configDir, prefixDir, magicFile);

  EXPECT_THAT(defaultConfig.c_str(), HasSubstr("<import-script>/usr/local/share/gerbera/js/import.js</import-script>"));
}

TEST_F(ConfigGeneratorTest, GeneratesScriptCommonWhenJSenabled) {
  mockConfig = mockConfigXml("fixtures/mock-config-import-js.xml");
  std::string defaultConfig = subject->generate(homePath, configDir, prefixDir, magicFile);

  EXPECT_THAT(defaultConfig.c_str(), HasSubstr("<common-script>/usr/local/share/gerbera/js/common.js</common-script>"));
}

TEST_F(ConfigGeneratorTest, GeneratesScriptPlaylistWhenJSenabled) {
  mockConfig = mockConfigXml("fixtures/mock-config-import-js.xml");
  std::string defaultConfig = subject->generate(homePath, configDir, prefixDir, magicFile);

  EXPECT_THAT(defaultConfig.c_str(), HasSubstr("<playlist-script>/usr/local/share/gerbera/js/playlists.js</playlist-script>"));
}
#endif

#ifdef ONLINE_SERVICES
TEST_F(ConfigGeneratorTest, GeneratesOnlineContent) {
  mockConfig = mockConfigXml("fixtures/mock-config-import-online.xml");
  std::string defaultConfig = subject->generate(homePath, configDir, prefixDir, magicFile);

  EXPECT_THAT(defaultConfig.c_str(), HasSubstr("<online-content>"));
}

#ifdef ATRAILERS
TEST_F(ConfigGeneratorTest, GeneratesOnlineContentAppleTrailers) {
  mockConfig = mockConfigXml("fixtures/mock-config-import-online.xml");
  std::string defaultConfig = subject->generate(homePath, configDir, prefixDir, magicFile);

  EXPECT_THAT(defaultConfig.c_str(), HasSubstr("<AppleTrailers enabled=\"no\" refresh=\"43200\" update-at-start=\"no\" resolution=\"640\"/>"));
}
#endif

#endif