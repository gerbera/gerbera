#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "config/config_generator.h"
#include "config/config_manager.h"
#include <fstream>
#include <regex>

using namespace ::testing;

class ConfigGeneratorTest : public ::testing::Test {

public:
    ConfigGeneratorTest() {};
    virtual ~ConfigGeneratorTest() {};

    virtual void SetUp()
    {
        subject = new ConfigGenerator();
        homePath = "/tmp";
        configDir = ".config/gerbera";
        prefixDir = "/usr/local/share/gerbera";
        magicFile = "magic.file";
    }

    virtual void TearDown()
    {
        delete subject;
    };

    std::string mockConfigXml(std::string mockFile)
    {
        std::ifstream t(mockFile);
        std::string str((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
        return str;
    }

    ConfigGenerator* subject;
    std::string homePath;
    std::string configDir;
    std::string prefixDir;
    std::string magicFile;
};

#if defined(HAVE_FFMPEG) && defined(HAVE_FFMPEGTHUMBNAILER) && defined(HAVE_MYSQL) && defined(HAVE_MAGIC) && defined(HAVE_JS) && defined(ONLINE_SERVICES)
TEST_F(ConfigGeneratorTest, GeneratesFullConfigXmlWithAllDefinitions)
{
    std::string mockXml = mockConfigXml("fixtures/mock-config-all.xml");

    std::string result = subject->generate(homePath, configDir, prefixDir, magicFile);

    // remove UUID, for simple compare...TODO: mock UUID?
    std::regex reg("<udn>uuid:[0-9A-Fa-f]{8}-[0-9A-Fa-f]{4}-[0-9A-Fa-f]{4}-[0-9A-Fa-f]{4}-[0-9A-Fa-f]{12}</udn>");
    result = std::regex_replace(result, reg, "<udn/>");

    EXPECT_STREQ(mockXml.c_str(), result.c_str());
}
#endif

#if !defined(HAVE_FFMPEG) && !defined(HAVE_FFMPEGTHUMBNAILER) && !defined(HAVE_MYSQL) && defined(HAVE_MAGIC) && defined(HAVE_JS) && defined(ONLINE_SERVICES)
TEST_F(ConfigGeneratorTest, GeneratesConfigXmlWithDefaultDefinitions)
{
    std::string mockXml = mockConfigXml("fixtures/mock-config-minimal.xml");

    std::string result = subject->generate(homePath, configDir, prefixDir, magicFile);

    // remove UUID, for simple compare...TODO: mock UUID?
    std::regex reg("<udn>uuid:[0-9A-Fa-f]{8}-[0-9A-Fa-f]{4}-[0-9A-Fa-f]{4}-[0-9A-Fa-f]{4}-[0-9A-Fa-f]{12}</udn>");
    result = std::regex_replace(result, reg, "<udn/>");
    EXPECT_STREQ(mockXml.c_str(), result.c_str());
}
#endif

#if defined(HAVE_FFMPEG) && defined(HAVE_FFMPEGTHUMBNAILER) && defined(HAVE_MYSQL)
TEST_F(ConfigGeneratorTest, GeneratesFullServerXmlWithAllDefinitions)
{
    std::string mockXml = mockConfigXml("fixtures/mock-server-all.xml");

    pugi::xml_document doc;
    auto config = doc.append_child("config");
    subject->generateServer(homePath, configDir, prefixDir, &config);

    std::ostringstream result;
    config.first_child().print(result, "  ");

    // remove UUID, for simple compare...TODO: mock UUID?
    std::regex reg("<udn>uuid:[0-9A-Fa-f]{8}-[0-9A-Fa-f]{4}-[0-9A-Fa-f]{4}-[0-9A-Fa-f]{4}-[0-9A-Fa-f]{12}</udn>");
    std::string resultStr = std::regex_replace(result.str(), reg, "<udn/>");

    EXPECT_STREQ(mockXml.c_str(), resultStr.c_str());
}
#endif

TEST_F(ConfigGeneratorTest, GeneratesUiAllTheTime)
{
    std::string mockXml = mockConfigXml("fixtures/mock-ui.xml");

    pugi::xml_document doc;
    auto server = doc.append_child("server");
    subject->generateUi(&server);

    std::ostringstream result;
    server.first_child().print(result, "  ");

    EXPECT_STREQ(mockXml.c_str(), result.str().c_str());
}

TEST_F(ConfigGeneratorTest, GeneratesImportMappingsAllTheTime)
{
    std::string mockXml = mockConfigXml("fixtures/mock-import-mappings.xml");

    pugi::xml_document doc;
    auto import = doc.append_child("import");
    subject->generateMappings(&import);

    std::ostringstream result;
    import.first_child().print(result, "  ");

    EXPECT_STREQ(mockXml.c_str(), result.str().c_str());
}

#if defined(HAVE_FFMPEG) && defined(HAVE_FFMPEGTHUMBNAILER)
TEST_F(ConfigGeneratorTest, GeneratesExtendedRuntimeXmlWithFFMPEG)
{
    std::string mockXml = mockConfigXml("fixtures/mock-extended-ffmpeg.xml");

    pugi::xml_document doc;
    auto server = doc.append_child("server");
    subject->generateExtendedRuntime(&server);

    std::ostringstream result;
    server.first_child().print(result, "  ");

    EXPECT_STREQ(mockXml.c_str(), result.str().c_str());
}
#endif

#if !defined(HAVE_FFMPEG) || !defined(HAVE_FFMPEGTHUMBNAILER)
TEST_F(ConfigGeneratorTest, GeneratesExtendedRuntimeXmlWithoutFFMPEG)
{
    std::string mockXml = mockConfigXml("fixtures/mock-extended.xml");

    pugi::xml_document doc;
    auto server = doc.append_child("server");
    subject->generateExtendedRuntime(&server);

    std::ostringstream result;
    server.first_child().print(result, "  ");

    EXPECT_STREQ(mockXml.c_str(), result.str().c_str());
}
#endif

#if defined(HAVE_MYSQL)
TEST_F(ConfigGeneratorTest, GeneratesDatabaseXmlWithMySQLAndSqlLite)
{
    std::string mockXml = mockConfigXml("fixtures/mock-database-mysql.xml");

    pugi::xml_document doc;
    auto server = doc.append_child("server");
    subject->generateDatabase(&server);

    std::ostringstream result;
    server.first_child().print(result, "  ");

    EXPECT_STREQ(mockXml.c_str(), result.str().c_str());
}
#endif

#if !defined(HAVE_MYSQL)
TEST_F(ConfigGeneratorTest, GeneratesDatabaseXmlWithSqlLiteOnly)
{
    std::string mockXml = mockConfigXml("fixtures/mock-database-sqlite.xml");

    pugi::xml_document doc;
    auto server = doc.append_child("server");
    subject->generateDatabase(&server);

    std::ostringstream result;
    server.first_child().print(result, "  ");

    EXPECT_STREQ(mockXml.c_str(), result.str().c_str());
}
#endif

#if defined(HAVE_MAGIC) && !defined(HAVE_JS) && !defined(ONLINE_SERVICES)
TEST_F(ConfigGeneratorTest, GeneratesImportWithMagicFile)
{
    std::string mockXml = mockConfigXml("fixtures/mock-import-magic.xml");

    pugi::xml_document doc;
    auto config = doc.append_child("config");
    subject->generateImport(prefixDir, magicFile, &config);

    std::ostringstream result;
    config.first_child().print(result, "  ");

    EXPECT_STREQ(mockXml.c_str(), result.str().c_str());
}

#elif defined(HAVE_MAGIC) && defined(HAVE_JS) && !defined(ONLINE_SERVICES)
TEST_F(ConfigGeneratorTest, GeneratesImportWithMagicAndJS)
{
    std::string mockXml = mockConfigXml("fixtures/mock-import-magic-js.xml");

    pugi::xml_document doc;
    auto config = doc.append_child("config");
    subject->generateImport(prefixDir, magicFile, &config);

    std::ostringstream result;
    config.first_child().print(result, "  ");

    EXPECT_STREQ(mockXml.c_str(), result.str().c_str());
}

#elif defined(HAVE_MAGIC) && defined(HAVE_JS) && defined(ONLINE_SERVICES)
TEST_F(ConfigGeneratorTest, GeneratesImportWithMagicJSandOnline)
{
    std::string mockXml = mockConfigXml("fixtures/mock-import-magic-js-online.xml");

    pugi::xml_document doc;
    auto config = doc.append_child("config");
    subject->generateImport(prefixDir, magicFile, &config);

    std::ostringstream result;
    config.first_child().print(result, "  ");

    EXPECT_STREQ(mockXml.c_str(), result.str().c_str());
}

#elif !defined(HAVE_MAGIC) && !defined(HAVE_JS) && !defined(ONLINE_SERVICES)
TEST_F(ConfigGeneratorTest, GeneratesImportNoMagicJSorOnline)
{
    std::string mockXml = mockConfigXml("fixtures/mock-import-none.xml");

    pugi::xml_document doc;
    auto config = doc.append_child("config");
    subject->generateImport(prefixDir, magicFile, &config);

    std::ostringstream result;
    config.first_child().print(result, "  ");

    EXPECT_STREQ(mockXml.c_str(), result.str().c_str());
}
#endif

#if defined(ATRAILERS)
TEST_F(ConfigGeneratorTest, GeneratesOnlineContentWithAppleTrailers)
{
    std::string mockXml = mockConfigXml("fixtures/mock-online-atrailers.xml");

    pugi::xml_document doc;
    auto import = doc.append_child("import");
    subject->generateOnlineContent(&import);

    std::ostringstream result;
    import.first_child().print(result, "  ");

    EXPECT_STREQ(mockXml.c_str(), result.str().c_str());
}
#else
TEST_F(ConfigGeneratorTest, GeneratesOnlineContentEmpty)
{
    std::string mockXml = "<online-content/>";

    pugi::xml_document doc;
    auto import = doc.append_child("import");
    subject->generateOnlineContent(&import);

    std::ostringstream result;
    import.first_child().print(result, "  ");

    EXPECT_STREQ(mockXml.c_str(), result.str().c_str());
}
#endif

TEST_F(ConfigGeneratorTest, GeneratesTranscodingProfilesAlways)
{
    std::string mockXml = mockConfigXml("fixtures/mock-transcoding.xml");

    pugi::xml_document doc;
    auto config = doc.append_child("config");
    subject->generateTranscoding(&config);

    std::ostringstream result;
    config.first_child().print(result, "  ");

    EXPECT_STREQ(mockXml.c_str(), result.str().c_str());
}

TEST_F(ConfigGeneratorTest, GeneratesUdnWithUUID)
{

    pugi::xml_document doc;
    auto server = doc.append_child("server");
    subject->generateUdn(&server);

    std::ostringstream result;
    server.first_child().first_child().print(result, "  ");

    EXPECT_THAT(result.str(), MatchesRegex("^uuid:[0-9A-Fa-f]{8}-[0-9A-Fa-f]{4}-[0-9A-Fa-f]{4}-[0-9A-Fa-f]{4}-[0-9A-Fa-f]{12}"));
}
