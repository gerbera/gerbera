/*GRB*
    Gerbera - https://gerbera.io/

    Copyright (C) 2026 Gerbera Contributors

    Gerbera is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.

    Gerbera is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Gerbera.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "../mock/config_mock.h"
#include "../mock/database_mock.h"
#include "cds/cds_container.h"
#include "cds/cds_resource.h"
#include "config/config_definition.h"
#include "config/config_val.h"
#include "config/result/directory_tweak.h"
#include "content/import_service.h"
#include "context.h"
#include "metadata/metacontent_handler.h"
#include "upnp/clients.h"
#include "util/mime.h"
#include "util/string_converter.h"
#include "util/tools.h"

#include <fstream>
#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <vector>

namespace {
class ContainerArtConfig : public ConfigMock {
public:
    fs::path directory;
    std::vector<std::string> files { "%upnpclass%-%filename%.png", "folder.jpg", "%upnpclass%.png" };

    std::string getOption(ConfigVal option) const override
    {
        if (option == ConfigVal::IMPORT_RESOURCES_CONTAINERART_LOCATION)
            return directory.string();
        if (option == ConfigVal::IMPORT_FILESYSTEM_CHARSET || option == ConfigVal::IMPORT_METADATA_CHARSET)
            return "UTF-8";
        return {};
    }

    std::vector<std::string> getArrayOption(ConfigVal option) const override
    {
        return option == ConfigVal::IMPORT_RESOURCES_CONTAINERART_FILE_LIST ? files : ConfigMock::getArrayOption(option);
    }
};

class ContainerArtDatabase : public DatabaseMock {
public:
    using DatabaseMock::DatabaseMock;
    std::shared_ptr<CdsContainer> container;

    bool addContainer(int parentId, std::string path, const std::shared_ptr<CdsContainer>& object, int* id) override
    {
        container = object;
        container->setID(42);
        container->setParentID(parentId);
        container->setLocation(path, CdsEntryType::VirtualContainer);
        *id = container->getID();
        return true;
    }

    std::shared_ptr<CdsObject> loadObject(int id, const std::string& group) override { return container; }
};

class ContainerArtTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        directory = fs::temp_directory_path() / ("gerbera-container-art-" + generateRandomId());
        fs::create_directories(directory / "theme");
        fs::create_directories(directory / "album");
    }

    void TearDown() override { fs::remove_all(directory); }

    fs::path directory;
};
}

TEST_F(ContainerArtTest, TypedDefaultsPreserveNamedCategoriesLocalCoversAndLaterInheritedCovers)
{
    auto config = std::make_shared<ContainerArtConfig>();
    config->directory = directory / "theme";
    for (const auto* name : { "object.container-Audio.png", "object.container.album.musicAlbum-Alben.png", "object.container.album.png", "object.container.png", "Audio.png", "titel.png" })
        std::ofstream(config->directory / name) << "artwork fixture";
    auto definition = std::make_shared<ConfigDefinition>();
    definition->init(definition);
    auto database = std::make_shared<ContainerArtDatabase>(config);
    auto converters = std::make_shared<ConverterManager>(config);
    auto mime = std::make_shared<Mime>(config);
    auto context = std::make_shared<Context>(definition, config, nullptr, mime, database, nullptr, converters);
    ContainerArtHandler handler(context);
    std::vector<int> newIds;
    auto root = std::make_shared<CdsContainer>("Audio");
    ASSERT_TRUE(handler.fillMetadata(root, newIds));
    EXPECT_EQ(root->getResource(ResourcePurpose::Thumbnail)->getAttribute(ResourceAttribute::RESOURCE_FILE), (config->directory / "object.container-Audio.png").string());
    auto plural = std::make_shared<CdsContainer>("Alben", "object.container.album.musicAlbum");
    ASSERT_TRUE(handler.fillMetadata(plural, newIds));
    EXPECT_EQ(plural->getResource(ResourcePurpose::Thumbnail)->getAttribute(ResourceAttribute::RESOURCE_FILE), (config->directory / "object.container.album.musicAlbum-Alben.png").string());

    auto album = std::make_shared<CdsContainer>("Audio", "object.container.album.photoAlbum");
    album->setLocation(directory / "album", CdsEntryType::Directory);
    std::ofstream(directory / "album/folder.jpg") << "local cover";
    ASSERT_TRUE(handler.fillMetadata(album, newIds));
    EXPECT_EQ(album->getResource(ResourcePurpose::Thumbnail)->getAttribute(ResourceAttribute::RESOURCE_FILE), (directory / "album/folder.jpg").string());
    EXPECT_FALSE(handler.fillFallbackMetadata(album, newIds));

    auto photoAlbum = std::make_shared<CdsContainer>("Audio", "object.container.album.photoAlbum");
    EXPECT_FALSE(handler.fillMetadata(photoAlbum, newIds));
    ASSERT_TRUE(handler.fillFallbackMetadata(photoAlbum, newIds));
    EXPECT_EQ(photoAlbum->getResource(ResourcePurpose::Thumbnail)->getAttribute(ResourceAttribute::RESOURCE_FILE), (config->directory / "object.container.album.png").string());

    auto generic = std::make_shared<CdsContainer>("Unknown folder");
    EXPECT_FALSE(handler.fillMetadata(generic, newIds));
    ASSERT_TRUE(handler.fillFallbackMetadata(generic, newIds));
    EXPECT_EQ(generic->getResource(ResourcePurpose::Thumbnail)->getAttribute(ResourceAttribute::RESOURCE_FILE), (config->directory / "object.container.png").string());

    ImportService importer(context, converters);
    importer.run(nullptr);
    auto nested = std::make_shared<CdsContainer>("Audio", "object.container.album.musicAlbum");
    auto reference = std::make_shared<CdsContainer>("Reference");
    reference->setID(10);
    std::vector<std::shared_ptr<CdsObject>> chain { nested };
    importer.addContainerTree(0, chain, reference, newIds);
    auto artwork = database->container->getResource(ResourcePurpose::Thumbnail);
    ASSERT_NE(artwork, nullptr);
    EXPECT_EQ(artwork->getAttribute(ResourceAttribute::RESOURCE_FILE), (config->directory / "object.container.album.png").string());
    EXPECT_EQ(artwork->getOption(RESOURCE_OPTION_CONTAINER_ART_FALLBACK), "yes");
    auto actual = std::make_shared<CdsResource>(ContentHandler::FANART, ResourcePurpose::Thumbnail);
    actual->addAttribute(ResourceAttribute::RESOURCE_FILE, "/real/embedded-cover.png");
    reference->addResource(actual);
    importer.addContainerTree(0, chain, reference, newIds);
    artwork = database->container->getResource(ResourcePurpose::Thumbnail);
    ASSERT_NE(artwork, nullptr);
    EXPECT_EQ(artwork->getAttribute(ResourceAttribute::RESOURCE_FILE), "/real/embedded-cover.png");
    EXPECT_TRUE(artwork->getOption(RESOURCE_OPTION_CONTAINER_ART_FALLBACK).empty());

    config->files = { "%filename%.png" };
    ContentPathSetup legacy(config, database, definition, ConfigVal::IMPORT_RESOURCES_CONTAINERART_FILE_LIST, ConfigVal::IMPORT_RESOURCES_CONTAINERART_DIR_LIST);
    EXPECT_TRUE(legacy.getContentPath(root, SETTING_CONTAINERART, config->directory, ContentPathMode::Fallback).front().empty());
    ContainerArtHandler legacyHandler(context);
    auto legacyContainer = std::make_shared<CdsContainer>("No configured default");
    EXPECT_FALSE(legacyHandler.fillFallbackMetadata(legacyContainer, newIds));
}
