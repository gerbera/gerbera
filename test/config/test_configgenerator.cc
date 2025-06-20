/*GRB*

    Gerbera - https://gerbera.io/

    test_configgenerator.cc - this file is part of Gerbera.

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

#include <fstream>
#include <regex>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using ::testing::MatchesRegex;

class ConfigGeneratorTest : public ::testing::Test {

public:
    ConfigGeneratorTest() = default;
    ~ConfigGeneratorTest() override = default;

    void SetUp() override
    {
        std::shared_ptr<ConfigDefinition> definition = std::make_shared<ConfigDefinition>();
        definition->init(definition);
        subject = new ConfigGenerator(definition, GERBERA_TEST, false);
        subject->init();
        homePath = "/tmp";
        configDir = ".config/gerbera";
        prefixDir = "/usr/local/share/gerbera";
        magicFile = "magic.file";
    }

    void TearDown() override
    {
        delete subject;
    }

    static std::string mockConfigXml(const std::string& mockFile)
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

#if defined(HAVE_FFMPEG) && defined(HAVE_FFMPEGTHUMBNAILER) && defined(HAVE_MYSQL) && defined(HAVE_MAGIC) && defined(HAVE_JS)
TEST_F(ConfigGeneratorTest, GeneratesFullConfigXmlWithAllDefinitions)
{
#ifdef ONLINE_SERVICES
    const std::string fileName = "fixtures/mock-config-all-os.xml";
#else
    const std::string fileName = "fixtures/mock-config-all.xml";
#endif
    auto mockXml = std::pair { fileName, mockConfigXml(fileName) };
    std::string result = subject->generate(homePath, configDir, prefixDir, magicFile);

    // remove UUID, for simple compare...TODO: mock UUID?
    std::regex reg("<udn>uuid:[0-9A-Fa-f]{8}-[0-9A-Fa-f]{4}-[0-9A-Fa-f]{4}-[0-9A-Fa-f]{4}-[0-9A-Fa-f]{12}</udn>");
    result = std::regex_replace(result, reg, "<udn/>");

    auto outXml = std::pair { fileName, result };
    EXPECT_EQ(mockXml, outXml);
}
#endif

#if !defined(HAVE_FFMPEG) && !defined(HAVE_FFMPEGTHUMBNAILER) && !defined(HAVE_MYSQL) && !defined(HAVE_MAGIC) && !defined(HAVE_JS) && !defined(ONLINE_SERVICES)
TEST_F(ConfigGeneratorTest, GeneratesConfigXmlWithDefaultDefinitions)
{
    const std::string fileName = "fixtures/mock-config-minimal.xml";
    auto mockXml = std::pair { fileName, mockConfigXml(fileName) };

    std::string result = subject->generate(homePath, configDir, prefixDir, magicFile);

    // remove UUID, for simple compare...TODO: mock UUID?
    std::regex reg("<udn>uuid:[0-9A-Fa-f]{8}-[0-9A-Fa-f]{4}-[0-9A-Fa-f]{4}-[0-9A-Fa-f]{4}-[0-9A-Fa-f]{12}</udn>");
    result = std::regex_replace(result, reg, "<udn/>");

    auto outXml = std::pair { fileName, result };
    EXPECT_EQ(mockXml, outXml);
}
#endif

#if defined(HAVE_FFMPEG) && defined(HAVE_FFMPEGTHUMBNAILER) && defined(HAVE_MYSQL)
TEST_F(ConfigGeneratorTest, GeneratesFullServerXmlWithAllDefinitions)
{
    const std::string fileName = "fixtures/mock-server-all.xml";
    auto mockXml = std::pair { fileName, mockConfigXml(fileName) };

    subject->generateServer(homePath, configDir, prefixDir);
    auto config = subject->getNode("");
    std::ostringstream result;
    config->first_child().print(result, "  ");

    // remove UUID, for simple compare...TODO: mock UUID?
    std::regex reg("<udn>uuid:[0-9A-Fa-f]{8}-[0-9A-Fa-f]{4}-[0-9A-Fa-f]{4}-[0-9A-Fa-f]{4}-[0-9A-Fa-f]{12}</udn>");
    std::string resultStr = std::regex_replace(result.str(), reg, "<udn/>");

    auto outXml = std::pair { fileName, resultStr };
    EXPECT_EQ(mockXml, outXml);
}
#endif

TEST_F(ConfigGeneratorTest, GeneratesUiAllTheTime)
{
    const std::string fileName = "fixtures/mock-ui.xml";
    auto mockXml = std::pair { fileName, mockConfigXml(fileName) };

    subject->generateUi();
    auto server = subject->getNode("/server");

    std::ostringstream result;
    server->first_child().print(result, "  ");

    auto outXml = std::pair { fileName, result.str() };
    EXPECT_EQ(mockXml, outXml);
}

TEST_F(ConfigGeneratorTest, GeneratesImportMappingsAllTheTime)
{
    const std::string fileName = "fixtures/mock-import-mappings.xml";
    auto mockXml = std::pair { fileName, mockConfigXml(fileName) };

    subject->generateMappings();
    auto import = subject->getNode("/import");

    std::ostringstream result;
    import->first_child().print(result, "  ");

    auto outXml = std::pair { fileName, result.str() };
    EXPECT_EQ(mockXml, outXml);
}

#if defined(HAVE_FFMPEG) && defined(HAVE_FFMPEGTHUMBNAILER)
TEST_F(ConfigGeneratorTest, GeneratesExtendedRuntimeXmlWithFFMPEG)
{
    const std::string fileName = "fixtures/mock-extended-ffmpeg.xml";
    auto mockXml = std::pair { fileName, mockConfigXml(fileName) };

    subject->generateExtendedRuntime();
    auto server = subject->getNode("/server");

    std::ostringstream result;
    server->first_child().print(result, "  ");

    auto outXml = std::pair { fileName, result.str() };
    EXPECT_EQ(mockXml, outXml);
}
#endif

#if !defined(HAVE_FFMPEG) || !defined(HAVE_FFMPEGTHUMBNAILER)
TEST_F(ConfigGeneratorTest, GeneratesExtendedRuntimeXmlWithoutFFMPEG)
{
    const std::string fileName = "fixtures/mock-extended.xml";
    auto mockXml = std::pair { fileName, mockConfigXml(fileName) };

    subject->generateExtendedRuntime();
    auto server = subject->getNode("/server");

    std::ostringstream result;
    server->first_child().print(result, "  ");

    auto outXml = std::pair { fileName, result.str() };
    EXPECT_EQ(mockXml, outXml);
}
#endif

#if defined(HAVE_MYSQL)
TEST_F(ConfigGeneratorTest, GeneratesDatabaseXmlWithMySQLAndSqlLite)
{
    const std::string fileName = "fixtures/mock-database-mysql.xml";
    auto mockXml = std::pair { fileName, mockConfigXml(fileName) };

    subject->generateDatabase(prefixDir);
    auto server = subject->getNode("/server");

    std::ostringstream result;
    server->first_child().print(result, "  ");

    auto outXml = std::pair { fileName, result.str() };
    EXPECT_EQ(mockXml, outXml);
}
#endif

#if !defined(HAVE_MYSQL)
TEST_F(ConfigGeneratorTest, GeneratesDatabaseXmlWithSqlLiteOnly)
{
    const std::string fileName = "fixtures/mock-database-sqlite.xml";
    auto mockXml = std::pair { fileName, mockConfigXml(fileName) };

    subject->generateDatabase(prefixDir);
    auto server = subject->getNode("/server");

    std::ostringstream result;
    server->first_child().print(result, "  ");

    auto outXml = std::pair { fileName, result.str() };
    EXPECT_EQ(mockXml, outXml);
}
#endif

#if defined(HAVE_MAGIC) && !defined(HAVE_JS) && !defined(ONLINE_SERVICES)
TEST_F(ConfigGeneratorTest, GeneratesImportWithMagicFile)
{
    const std::string fileName = "fixtures/mock-import-magic.xml";
    auto mockXml = std::pair { fileName, mockConfigXml(fileName) };

    subject->generateImport(prefixDir, configDir, magicFile);
    auto config = subject->getNode("");

    std::ostringstream result;
    config->first_child().print(result, "  ");

    auto outXml = std::pair { fileName, result.str() };
    EXPECT_EQ(mockXml, outXml);
}

#elif defined(HAVE_MAGIC) && defined(HAVE_JS) && !defined(ONLINE_SERVICES)
TEST_F(ConfigGeneratorTest, GeneratesImportWithMagicAndJS)
{
    const std::string fileName = "fixtures/mock-import-magic-js.xml";
    auto mockXml = std::pair { fileName, mockConfigXml(fileName) };

    subject->generateImport(prefixDir, configDir, magicFile);
    auto config = subject->getNode("");

    std::ostringstream result;
    config->first_child().print(result, "  ");

    auto outXml = std::pair { fileName, result.str() };
    EXPECT_EQ(mockXml, outXml);
}

#elif defined(HAVE_MAGIC) && defined(HAVE_JS) && defined(ONLINE_SERVICES)
TEST_F(ConfigGeneratorTest, GeneratesImportWithMagicJSandOnline)
{
    const std::string fileName = "fixtures/mock-import-magic-js-online.xml";
    auto mockXml = std::pair { fileName, mockConfigXml(fileName) };

    subject->generateImport(prefixDir, configDir, magicFile);
    auto config = subject->getNode("");

    std::ostringstream result;
    config->first_child().print(result, "  ");

    auto outXml = std::pair { fileName, result.str() };
    EXPECT_EQ(mockXml, outXml);
}

#elif !defined(HAVE_MAGIC) && !defined(HAVE_JS) && !defined(ONLINE_SERVICES)
TEST_F(ConfigGeneratorTest, GeneratesImportNoMagicJSnorOnline)
{
    const std::string fileName = "fixtures/mock-import-none.xml";
    auto mockXml = std::pair { fileName, mockConfigXml(fileName) };

    subject->generateImport(prefixDir, configDir, magicFile);
    auto config = subject->getNode("");

    std::ostringstream result;
    config->first_child().print(result, "  ");

    auto outXml = std::pair { fileName, result.str() };
    EXPECT_EQ(mockXml, outXml);
}
#endif

#ifdef ONLINE_SERVICES
TEST_F(ConfigGeneratorTest, GeneratesOnlineContentEmpty)
{
    std::string mockXml = "<online-content />\n";

    subject->generateOnlineContent();
    auto import = subject->getNode("/import");

    std::ostringstream result;
    import->first_child().print(result, "  ");

    EXPECT_EQ(mockXml, result.str());
}
#endif

TEST_F(ConfigGeneratorTest, GeneratesTranscodingProfilesAlways)
{
    const std::string fileName = "fixtures/mock-transcoding.xml";
    auto mockXml = std::pair { fileName, mockConfigXml(fileName) };

    subject->generateTranscoding();
    auto config = subject->getNode("");

    std::ostringstream result;
    config->first_child().print(result, "  ");
    auto outXml = std::pair { fileName, result.str() };
    EXPECT_EQ(mockXml, outXml);
}

TEST_F(ConfigGeneratorTest, GeneratesUdnWithUUID)
{
    subject->generateUdn();
    auto server = subject->getNode("/server");

    std::ostringstream result;
    server->first_child().first_child().print(result, "  ");

    EXPECT_THAT(result.str(), MatchesRegex("^uuid:[0-9A-Fa-f]{8}-[0-9A-Fa-f]{4}-[0-9A-Fa-f]{4}-[0-9A-Fa-f]{4}-[0-9A-Fa-f]{12}"));
}

class ExampleConfigGeneratorTest : public ::testing::Test {

public:
    ExampleConfigGeneratorTest() = default;
    ~ExampleConfigGeneratorTest() override = default;

    void SetUp() override
    {
        std::shared_ptr<ConfigDefinition> definition = std::make_shared<ConfigDefinition>();
        definition->init(definition);
        subject = new ConfigGenerator(definition, GERBERA_TEST, true);
        subject->init();
        homePath = "/tmp";
        configDir = ".config/gerbera";
        prefixDir = "/usr/local/share/gerbera";
        magicFile = "magic.file";
    }

    void TearDown() override
    {
        delete subject;
    }

    static std::string mockConfigXml(const std::string& mockFile)
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

#if defined(HAVE_FFMPEG) && defined(HAVE_FFMPEGTHUMBNAILER) && defined(HAVE_MYSQL) && defined(HAVE_MAGIC) && defined(HAVE_JS) && defined(HAVE_EXIV2)
TEST_F(ExampleConfigGeneratorTest, GeneratesFullConfigXmlWithExiv2AllDefinitions)
{
#ifdef ONLINE_SERVICES
    std::string mockXml = mockConfigXml("fixtures/mock-example-exiv2-all-os.xml");
#else
    std::string mockXml = mockConfigXml("fixtures/mock-example-exiv2-all.xml");
#endif
    std::string result = subject->generate(homePath, configDir, prefixDir, magicFile);

    // remove UUID, for simple compare...TODO: mock UUID?
    std::regex reg("<udn>uuid:[0-9A-Fa-f]{8}-[0-9A-Fa-f]{4}-[0-9A-Fa-f]{4}-[0-9A-Fa-f]{4}-[0-9A-Fa-f]{12}</udn>");
    result = std::regex_replace(result, reg, "<udn/>");

    EXPECT_STREQ(mockXml.c_str(), result.c_str());
}
#endif

#if defined(HAVE_FFMPEG) && defined(HAVE_FFMPEGTHUMBNAILER) && defined(HAVE_MYSQL) && defined(HAVE_MAGIC) && defined(HAVE_JS) && !defined(HAVE_EXIV2)
TEST_F(ExampleConfigGeneratorTest, GeneratesFullConfigXmlWithAllDefinitions)
{
#ifdef ONLINE_SERVICES
    std::string mockXml = mockConfigXml("fixtures/mock-example-all-os.xml");
#else
    std::string mockXml = mockConfigXml("fixtures/mock-example-all.xml");
#endif
    std::string result = subject->generate(homePath, configDir, prefixDir, magicFile);

    // remove UUID, for simple compare...TODO: mock UUID?
    std::regex reg("<udn>uuid:[0-9A-Fa-f]{8}-[0-9A-Fa-f]{4}-[0-9A-Fa-f]{4}-[0-9A-Fa-f]{4}-[0-9A-Fa-f]{12}</udn>");
    result = std::regex_replace(result, reg, "<udn/>");

    EXPECT_STREQ(mockXml.c_str(), result.c_str());
}
#endif

#if !defined(HAVE_FFMPEG) && !defined(HAVE_FFMPEGTHUMBNAILER) && !defined(HAVE_MYSQL) && !defined(HAVE_MAGIC) && !defined(HAVE_JS) && !defined(ONLINE_SERVICES)
TEST_F(ExampleConfigGeneratorTest, GeneratesConfigXmlWithDefaultDefinitions)
{
    std::string mockXml = mockConfigXml("fixtures/mock-example-minimal.xml");

    std::string result = subject->generate(homePath, configDir, prefixDir, magicFile);

    // remove UUID, for simple compare...TODO: mock UUID?
    std::regex reg("<udn>uuid:[0-9A-Fa-f]{8}-[0-9A-Fa-f]{4}-[0-9A-Fa-f]{4}-[0-9A-Fa-f]{4}-[0-9A-Fa-f]{12}</udn>");
    result = std::regex_replace(result, reg, "<udn/>");
    EXPECT_STREQ(mockXml.c_str(), result.c_str());
}
#endif
