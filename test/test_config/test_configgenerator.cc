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
};

#if defined(HAVE_FFMPEG) && defined(HAVE_FFMPEGTHUMBNAILER) && defined(HAVE_MYSQL) && defined(HAVE_MAGIC) && defined(HAVE_JS) && defined(ONLINE_SERVICES)
TEST_F(ConfigGeneratorTest, GeneratesFullConfigXmlWithAllDefinitions) {
  std::string mockXml = mockConfigXml("fixtures/mock-config-all.xml");

  std::string result = subject->generate(homePath, configDir, prefixDir, magicFile);

  EXPECT_STREQ(mockXml.c_str(), result.c_str());
}
#endif

#if !defined(HAVE_FFMPEG) && !defined(HAVE_FFMPEGTHUMBNAILER) && !defined(HAVE_MYSQL) && defined(HAVE_MAGIC) && defined(HAVE_JS) && defined(ONLINE_SERVICES)
TEST_F(ConfigGeneratorTest, GeneratesConfigXmlWithDefaultDefinitions) {
  std::string mockXml = mockConfigXml("fixtures/mock-config-minimal.xml");

  std::string result = subject->generate(homePath, configDir, prefixDir, magicFile);

  EXPECT_STREQ(mockXml.c_str(), result.c_str());
}
#endif

#if defined(HAVE_FFMPEG) && defined(HAVE_FFMPEGTHUMBNAILER) && defined(HAVE_MYSQL)
TEST_F(ConfigGeneratorTest, GeneratesFullServerXmlWithAllDefinitions) {
  std::string mockXml = mockConfigXml("fixtures/mock-server-all.xml");

  zmm::Ref<mxml::Element> result = subject->generateServer(homePath, configDir, prefixDir);
  result->indent();

  EXPECT_STREQ(mockXml.c_str(), result->print().c_str());
}
#endif

TEST_F(ConfigGeneratorTest, GeneratesUiAllTheTime) {
  std::string mockXml = mockConfigXml("fixtures/mock-ui.xml");

  zmm::Ref<mxml::Element> result = subject->generateUi();
  result->indent();

  EXPECT_STREQ(mockXml.c_str(), result->print().c_str());
}

TEST_F(ConfigGeneratorTest, GeneratesImportMappingsAllTheTime) {
  std::string mockXml = mockConfigXml("fixtures/mock-import-mappings.xml");

  zmm::Ref<mxml::Element> result = subject->generateMappings();
  result->indent();

  EXPECT_STREQ(mockXml.c_str(), result->print().c_str());
}

#if defined(HAVE_FFMPEG) && defined(HAVE_FFMPEGTHUMBNAILER)
TEST_F(ConfigGeneratorTest, GeneratesExtendedRuntimeXmlWithFFMPEG) {
  std::string mockXml = mockConfigXml("fixtures/mock-extended-ffmpeg.xml");

  zmm::Ref<mxml::Element> result = subject->generateExtendedRuntime();
  result->indent();

  EXPECT_STREQ(mockXml.c_str(), result->print().c_str());
}
#endif

#if !defined(HAVE_FFMPEG) || !defined(HAVE_FFMPEGTHUMBNAILER)
TEST_F(ConfigGeneratorTest, GeneratesExtendedRuntimeXmlWithoutFFMPEG) {
  std::string mockXml = mockConfigXml("fixtures/mock-extended.xml");

  zmm::Ref<mxml::Element> result = subject->generateExtendedRuntime();
  result->indent();

  EXPECT_STREQ(mockXml.c_str(), result->print().c_str());
}
#endif

#if defined(HAVE_MYSQL)
TEST_F(ConfigGeneratorTest, GeneratesStorageXmlWithMySQLAndSqlLite) {
  std::string mockXml = mockConfigXml("fixtures/mock-storage-mysql.xml");

  zmm::Ref<mxml::Element> result = subject->generateStorage();
  result->indent();

  EXPECT_STREQ(mockXml.c_str(), result->print().c_str());
}
#endif

#if !defined(HAVE_MYSQL)
TEST_F(ConfigGeneratorTest, GeneratesStorageXmlWithSqlLiteOnly) {
  std::string mockXml = mockConfigXml("fixtures/mock-storage-sqlite.xml");

  zmm::Ref<mxml::Element> result = subject->generateStorage();
  result->indent();

  EXPECT_STREQ(mockXml.c_str(), result->print().c_str());
}
#endif



#if defined(HAVE_MAGIC) && !defined(HAVE_JS) && !defined(ONLINE_SERVICES)
TEST_F(ConfigGeneratorTest, GeneratesImportWithMagicFile) {
  std::string mockXml = mockConfigXml("fixtures/mock-import-magic.xml");

  zmm::Ref<mxml::Element> result = subject->generateImport(prefixDir, magicFile);
  result->indent();

  EXPECT_STREQ(mockXml.c_str(), result->print().c_str());
}

#elif defined(HAVE_MAGIC) && defined(HAVE_JS) && !defined(ONLINE_SERVICES)
TEST_F(ConfigGeneratorTest, GeneratesImportWithMagicAndJS) {
  std::string mockXml = mockConfigXml("fixtures/mock-import-magic-js.xml");

  zmm::Ref<mxml::Element> result = subject->generateImport(prefixDir, magicFile);
  result->indent();

  EXPECT_STREQ(mockXml.c_str(), result->print().c_str());
}

#elif defined(HAVE_MAGIC) && defined(HAVE_JS) && defined(ONLINE_SERVICES)
TEST_F(ConfigGeneratorTest, GeneratesImportWithMagicJSandOnline) {
  std::string mockXml = mockConfigXml("fixtures/mock-import-magic-js-online.xml");

  zmm::Ref<mxml::Element> result = subject->generateImport(prefixDir, magicFile);
  result->indent();

  EXPECT_STREQ(mockXml.c_str(), result->print().c_str());
}

#elif !defined(HAVE_MAGIC) && !defined(HAVE_JS) && !defined(ONLINE_SERVICES)
TEST_F(ConfigGeneratorTest, GeneratesImportNoMagicJSorOnline) {
  std::string mockXml = mockConfigXml("fixtures/mock-import-none.xml");

  zmm::Ref<mxml::Element> result = subject->generateImport(prefixDir, magicFile);
  result->indent();

  EXPECT_STREQ(mockXml.c_str(), result->print().c_str());
}
#endif

#if defined(ATRAILERS)
TEST_F(ConfigGeneratorTest, GeneratesOnlineContentWithAppleTrailers) {
  std::string mockXml = mockConfigXml("fixtures/mock-online-atrailers.xml");

  zmm::Ref<mxml::Element> result = subject->generateOnlineContent();
  result->indent();

  EXPECT_STREQ(mockXml.c_str(), result->print().c_str());
}
#else
TEST_F(ConfigGeneratorTest, GeneratesOnlineContentEmpty) {
  std::string mockXml = "<online-content/>";

  zmm::Ref<mxml::Element> result = subject->generateOnlineContent();
  result->indent();

  EXPECT_STREQ(mockXml.c_str(), result->print().c_str());
}
#endif

TEST_F(ConfigGeneratorTest, GeneratesTranscodingProfilesAlways) {
  std::string mockXml = mockConfigXml("fixtures/mock-transcoding.xml");

  zmm::Ref<mxml::Element> result = subject->generateTranscoding();
  result->indent();

  EXPECT_STREQ(mockXml.c_str(), result->print().c_str());
}


