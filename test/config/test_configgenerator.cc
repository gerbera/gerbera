/*GRB*

    Gerbera - https://gerbera.io/

    test_configgenerator.cc - this file is part of Gerbera.

    Copyright (C) 2016-2026 Gerbera Contributors

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

#include <array>
#include <fstream>
#include <iterator>
#include <map>
#include <regex>
#include <string>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using ::testing::MatchesRegex;

class ConfigGeneratorTest : public ::testing::Test {

public:
    ConfigGeneratorTest() = default;
    ~ConfigGeneratorTest() override = default;

    void SetUp() override
    {
        definition = std::make_shared<ConfigDefinition>();
        definition->init(definition);
        subject = new ConfigGenerator(definition, GERBERA_TEST, ConfigLevel::Base);
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
    std::shared_ptr<ConfigDefinition> definition;
    std::string homePath;
    std::string configDir;
    std::string prefixDir;
    std::string magicFile;
};

TEST_F(ConfigGeneratorTest, AdvancedConfigIncludesInstalledContainerIconsAndExistingCoverFallbacks)
{
    for (const auto& dataDir : std::array { fs::path("/usr/share/gerbera"), fs::path("/opt/music/share/gerbera") }) {
        const std::map<ContainerIconTheme, fs::path> themes {
            { ContainerIconTheme::Gerbera, "container-icons/gerbera" }, { ContainerIconTheme::Techno, "container-icons" }
        };
        for (const auto& [theme, themeDir] : themes) {
            ConfigGenerator advanced(definition, GERBERA_TEST, ConfigLevel::Advanced, 0, 0, theme);
            pugi::xml_document document;
            ASSERT_TRUE(document.load_string(advanced.generate(homePath, configDir, dataDir, magicFile).c_str()));
            const auto resources = document.child("config").child("import").child("resources").child("container");
            ASSERT_TRUE(resources);
            EXPECT_EQ(resources.attribute("location").as_string(), (dataDir / themeDir).string());
            std::vector<std::string> names;
            for (const auto file : resources.children("add-file"))
                names.emplace_back(file.attribute("name").as_string());
            const std::vector<std::string> expected { "%upnpclass%-%filename%.png", "folder.jpg", "poster.jpg", "cover.jpg", "albumartsmall.jpg", "%upnpclass%.png" };
            EXPECT_EQ(names, expected);
            const auto containers = document.child("config").child("server").child("containers");
            EXPECT_EQ(std::distance(containers.children("container").begin(), containers.children("container").end()), 3);
            const std::map<std::string, std::string> images {
                { "/LastAdded", "recently-added.png" }, { "/LastModified", "recently-modified.png" }, { "/LastPlayed", "recently-played.png" }
            };
            for (const auto& [location, image] : images) {
                const auto container = containers.find_child_by_attribute("container", "location", location.c_str());
                ASSERT_TRUE(container) << location;
                EXPECT_EQ(container.attribute("image").as_string(), (dataDir / themeDir / image).string());
            }
            const auto played = containers.find_child_by_attribute("container", "location", "/LastPlayed");
            EXPECT_NE(played.attribute("image").as_string(), (dataDir / themeDir / "Audio.png").string());
            EXPECT_EQ(fs::path(played.attribute("image").as_string()).extension(), ".png");
            EXPECT_STREQ(played.attribute("upnp-shortcut").as_string(), "MUSIC_LAST_PLAYED");
            EXPECT_STREQ(played.attribute("sort").as_string(), "-upnp:lastPlaybackTime");
            EXPECT_STREQ(played.child_value("filter"), R"(upnp:class derivedfrom "object.item.audioItem" and upnp:lastPlaybackTime > "@last7")");
        }
    }
}

TEST_F(ConfigGeneratorTest, BaseAndExampleConfigKeepExistingArtworkDefaults)
{
    for (const auto level : std::array { ConfigLevel::Base, ConfigLevel::Example }) {
        ConfigGenerator generator(definition, GERBERA_TEST, level);
        pugi::xml_document document;
        ASSERT_TRUE(document.load_string(generator.generate(homePath, configDir, prefixDir, magicFile).c_str()));
        EXPECT_STREQ(document.child("config").child("import").child("resources").child("container").attribute("location").as_string(), "");
        for (const auto container : document.child("config").child("server").child("containers").children("container")) {
            EXPECT_FALSE(container.attribute("image"));
        }
        EXPECT_FALSE(document.child("config").child("import").child("resources").child("container").find_child_by_attribute("add-file", "name", "%filename%.png"));
        EXPECT_FALSE(document.child("config").child("import").child("resources").child("container").find_child_by_attribute("add-file", "name", "%upnpclass%-%filename%.png"));
        EXPECT_FALSE(document.child("config").child("import").child("resources").child("container").find_child_by_attribute("add-file", "name", "%upnpclass%.png"));
    }
}

TEST_F(ConfigGeneratorTest, AdvancedIconSettingsRespectSelectedSections)
{
    for (const auto& section : std::array { "Import", "DynamicContainer" }) {
        ConfigGenerator advanced(definition, GERBERA_TEST, ConfigLevel::Advanced, ConfigGenerator::makeSections(section));
        pugi::xml_document document;
        ASSERT_TRUE(document.load_string(advanced.generate(homePath, configDir, prefixDir, magicFile).c_str()));
        const auto resources = document.child("config").child("import").child("resources").child("container");
        const auto containers = document.child("config").child("server").child("containers");
        if (std::string(section) == "Import") {
            EXPECT_EQ(resources.attribute("location").as_string(), (fs::path(prefixDir) / "container-icons" / "gerbera").string());
            EXPECT_FALSE(containers);
        } else {
            EXPECT_FALSE(resources);
            EXPECT_EQ(containers.first_child().attribute("image").as_string(), (fs::path(prefixDir) / "container-icons" / "gerbera" / "recently-added.png").string());
        }
    }
}

#if defined(HAVE_FFMPEG) && defined(HAVE_FFMPEGTHUMBNAILER) && defined(HAVE_MYSQL) && defined(HAVE_MAGIC) && defined(HAVE_JS) && defined(HAVE_PGSQL)
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

#if !defined(HAVE_FFMPEG) && !defined(HAVE_FFMPEGTHUMBNAILER) && !defined(HAVE_MYSQL) && !defined(HAVE_MAGIC) && !defined(HAVE_JS) && !defined(ONLINE_SERVICES) && !defined(HAVE_PGSQL)
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

#if defined(HAVE_FFMPEG) && defined(HAVE_FFMPEGTHUMBNAILER) && defined(HAVE_MYSQL) && defined(HAVE_PGSQL)
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

#if defined(HAVE_MYSQL) && !defined(HAVE_PGSQL)
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

#if !defined(HAVE_MYSQL) && !defined(HAVE_PGSQL)
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
    std::string mockXml = "<online-services />\n";

    subject->generateOnlineContent();
    auto import = subject->getNode("/import");

    std::ostringstream result;
    import->first_child().print(result, "  ");

    EXPECT_EQ(mockXml, result.str());
}
#endif

TEST_F(ConfigGeneratorTest, GeneratesTranscodingProfilesAlways)
{
#ifdef HAVE_FFMPEG
    const std::string fileName = "fixtures/mock-transcoding.xml";
#else
    const std::string fileName = "fixtures/mock-transcoding-minimal.xml";
#endif
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

    static std::vector<std::string> pugixml116;

    void SetUp() override
    {
        std::shared_ptr<ConfigDefinition> definition = std::make_shared<ConfigDefinition>();
        definition->init(definition);
        subject = new ConfigGenerator(definition, GERBERA_TEST, ConfigLevel::Example);
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

    static std::string fixXML(const std::string& input)
    {
        // remove UUID, for simple compare...TODO: mock UUID?
        std::regex reg("<udn>uuid:[0-9A-Fa-f]{8}-[0-9A-Fa-f]{4}-[0-9A-Fa-f]{4}-[0-9A-Fa-f]{4}-[0-9A-Fa-f]{12}</udn>");
        auto result = std::regex_replace(input, reg, "<udn/>");

        // pugixml 1.16 prints empty sections in short version
        for (auto sec : pugixml116) {
            std::regex reSec(fmt::format("\\>\\</{}\\>", sec), std::regex_constants::ECMAScript);
            result = std::regex_replace(result, reSec, " />");
        }
        return result;
    }

    ConfigGenerator* subject;
    std::string homePath;
    std::string configDir;
    std::string prefixDir;
    std::string magicFile;
};

std::vector<std::string> ExampleConfigGeneratorTest::pugixml116 = {
    "source-docs-link",
    "ip",
    "interface",
    "modelURL",
    "presentationURL",
    "virtualURL",
    "externalURL",
    "socket",
    "password",
    "cache-dir",
    "genre-map",
    "auxdata",
    "metadata",
    "comment",
    "visible-directories",
    "order",
    "metafile",
    "resource",
    "container",
    "layout",
    "directories",
};

#if defined(HAVE_FFMPEG) && defined(HAVE_FFMPEGTHUMBNAILER) && defined(HAVE_MYSQL) && defined(HAVE_MAGIC) && defined(HAVE_JS) && defined(HAVE_EXIV2) && defined(HAVE_PGSQL) && !defined(HAVE_LASTFM)
TEST_F(ExampleConfigGeneratorTest, GeneratesFullConfigXmlWithExiv2AllDefinitions)
{
#ifdef ONLINE_SERVICES
    std::string mockXml = mockConfigXml("fixtures/mock-example-exiv2-all-os.xml");
#else
    std::string mockXml = mockConfigXml("fixtures/mock-example-exiv2-all.xml");
#endif
    std::string result = subject->generate(homePath, configDir, prefixDir, magicFile);
#ifdef GERBERA_RELEASE
    replaceAllString(mockXml, "https://docs.gerbera.io/en/latest/", "https://docs.gerbera.io/en/stable/");
#endif

    mockXml = fixXML(mockXml);
    result = fixXML(result);

    EXPECT_STREQ(mockXml.c_str(), result.c_str());
}
#endif

#if defined(HAVE_FFMPEG) && defined(HAVE_FFMPEGTHUMBNAILER) && defined(HAVE_MYSQL) && defined(HAVE_MAGIC) && defined(HAVE_JS) && defined(HAVE_EXIV2) && defined(HAVE_PGSQL) && defined(HAVE_LASTFM)
TEST_F(ExampleConfigGeneratorTest, GeneratesFullConfigXmlWithExiv2AndLastfmAllDefinitions)
{
#ifdef ONLINE_SERVICES
    std::string mockXml = mockConfigXml("fixtures/mock-example-lastfm-all-os.xml");
#else
    std::string mockXml = mockConfigXml("fixtures/mock-example-lastfm-all.xml");
#endif
    std::string result = subject->generate(homePath, configDir, prefixDir, magicFile);
#ifdef GERBERA_RELEASE
    replaceAllString(mockXml, "https://docs.gerbera.io/en/latest/", "https://docs.gerbera.io/en/stable/");
#endif

    mockXml = fixXML(mockXml);
    result = fixXML(result);

    EXPECT_STREQ(mockXml.c_str(), result.c_str());
}
#endif

#if defined(HAVE_FFMPEG) && defined(HAVE_FFMPEGTHUMBNAILER) && defined(HAVE_MYSQL) && defined(HAVE_MAGIC) && defined(HAVE_JS) && !defined(HAVE_EXIV2) && defined(HAVE_PGSQL) && !defined(HAVE_LASTFM)
TEST_F(ExampleConfigGeneratorTest, GeneratesFullConfigXmlWithAllDefinitions)
{
#ifdef ONLINE_SERVICES
    std::string mockXml = mockConfigXml("fixtures/mock-example-all-os.xml");
#else
    std::string mockXml = mockConfigXml("fixtures/mock-example-all.xml");
#endif
#ifdef GERBERA_RELEASE
    replaceAllString(mockXml, "https://docs.gerbera.io/en/latest/", "https://docs.gerbera.io/en/stable/");
#endif
    std::string result = subject->generate(homePath, configDir, prefixDir, magicFile);

    mockXml = fixXML(mockXml);
    result = fixXML(result);

    EXPECT_STREQ(mockXml.c_str(), result.c_str());
}
#endif

#if !defined(HAVE_FFMPEG) && !defined(HAVE_FFMPEGTHUMBNAILER) && !defined(HAVE_MYSQL) && !defined(HAVE_MAGIC) && !defined(HAVE_JS) && !defined(ONLINE_SERVICES) && !defined(HAVE_PGSQL) && !defined(HAVE_LASTFM)
TEST_F(ExampleConfigGeneratorTest, GeneratesConfigXmlWithDefaultDefinitions)
{
    std::string mockXml = mockConfigXml("fixtures/mock-example-minimal.xml");
#ifdef GERBERA_RELEASE
    replaceAllString(mockXml, "https://docs.gerbera.io/en/latest/", "https://docs.gerbera.io/en/stable/");
#endif

    std::string result = subject->generate(homePath, configDir, prefixDir, magicFile);

    mockXml = fixXML(mockXml);
    result = fixXML(result);

    EXPECT_STREQ(mockXml.c_str(), result.c_str());
}
#endif
