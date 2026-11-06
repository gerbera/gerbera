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
#include "cds/cds_item.h"
#include "cds/cds_resource.h"
#include "config/config_definition.h"
#include "config/config_val.h"
#include "config/result/autoscan.h"
#include "config/result/directory_tweak.h"
#include "content/import_service.h"
#include "context.h"
#include "metadata/metacontent_handler.h"
#include "upnp/clients.h"
#include "util/mime.h"
#include "util/string_converter.h"
#include "util/tools.h"

#include <chrono>
#include <fstream>
#include <gtest/gtest.h>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace {
class CoverArtPriorityConfig : public ConfigMock {
public:
    fs::path directory;
    std::string priority;
    std::vector<std::string> files { "object.container.png" };
    bool caseSensitive = false;
    std::shared_ptr<DirectoryConfigList> directoryTweaks;

    std::string getOption(ConfigVal option) const override
    {
        if (option == ConfigVal::IMPORT_RESOURCES_CONTAINERART_PRIORITY)
            return priority;
        if (option == ConfigVal::IMPORT_RESOURCES_CONTAINERART_LOCATION)
            return directory.string();
        if (option == ConfigVal::IMPORT_FILESYSTEM_CHARSET || option == ConfigVal::IMPORT_METADATA_CHARSET)
            return "UTF-8";
        return {};
    }

    std::int32_t getIntOption(ConfigVal option) const override
    {
        if (option == ConfigVal::IMPORT_RESOURCES_CONTAINERART_PARENTCOUNT)
            return 2;
        return ConfigMock::getIntOption(option);
    }

    bool getBoolOption(ConfigVal option) const override
    {
        return option == ConfigVal::IMPORT_RESOURCES_CASE_SENSITIVE ? caseSensitive : ConfigMock::getBoolOption(option);
    }

    std::shared_ptr<DirectoryConfigList> getDirectoryTweakOption(ConfigVal option) const override
    {
        return option == ConfigVal::IMPORT_DIRECTORIES_LIST ? directoryTweaks : ConfigMock::getDirectoryTweakOption(option);
    }

    std::map<std::string, std::string> getDictionaryOption(ConfigVal option) const override
    {
        if (option == ConfigVal::IMPORT_MAPPINGS_EXTENSION_TO_MIMETYPE_LIST)
            return { { "jpg", "image/jpeg" }, { "png", "image/png" }, { "txt", "text/plain" } };
        return ConfigMock::getDictionaryOption(option);
    }

    std::vector<std::string> getArrayOption(ConfigVal option) const override
    {
        return option == ConfigVal::IMPORT_RESOURCES_CONTAINERART_FILE_LIST ? files : ConfigMock::getArrayOption(option);
    }
};

class CoverArtPriorityDatabase : public DatabaseMock {
public:
    using DatabaseMock::DatabaseMock;
    std::shared_ptr<CdsContainer> container;
    std::map<int, std::shared_ptr<CdsObject>> tracks;

    bool addContainer(int parentId, std::string path, const std::shared_ptr<CdsContainer>& object, int* id) override
    {
        container = object;
        container->setID(42);
        container->setParentID(parentId);
        container->setLocation(path, CdsEntryType::VirtualContainer);
        *id = container->getID();
        return true;
    }

    std::shared_ptr<CdsObject> loadObject(int id, const std::string& group) override
    {
        const auto track = tracks.find(id);
        if (track != tracks.end())
            return track->second;
        return container && container->getID() == id ? container : nullptr;
    }
};

class CoverArtPriorityTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        directory = fs::temp_directory_path() / ("gerbera-container-art-" + generateRandomId());
        fs::create_directories(directory / "theme");
        fs::create_directories(directory / "album");
        config = std::make_shared<CoverArtPriorityConfig>();
        config->directory = directory / "theme";
        definition = std::make_shared<ConfigDefinition>();
        definition->init(definition);
        database = std::make_shared<CoverArtPriorityDatabase>(config);
        converters = std::make_shared<ConverterManager>(config);
        auto mime = std::make_shared<Mime>(config);
        context = std::make_shared<Context>(definition, config, nullptr, mime, database, nullptr, converters);
    }

    void TearDown() override { fs::remove_all(directory); }

    fs::path directory;
    std::shared_ptr<CoverArtPriorityConfig> config;
    std::shared_ptr<ConfigDefinition> definition;
    std::shared_ptr<CoverArtPriorityDatabase> database;
    std::shared_ptr<ConverterManager> converters;
    std::shared_ptr<Context> context;
};
}

TEST_F(CoverArtPriorityTest, EmbeddedAudioArtworkReplacesExternalCoverAndKeepsEarliestTrack)
{
    config->priority = "Embedded, , PATTERN, external";
    config->files = { "cover.jpg" };
    const auto folderPath = directory / "album";
    std::ofstream(folderPath / "cover.jpg") << "external artwork";
    auto folder = std::make_shared<CdsContainer>(CdsEntryType::Directory);
    folder->setID(20);
    folder->setLocation(folderPath, CdsEntryType::Directory);

    auto fileCover = std::make_shared<CdsResource>(ContentHandler::CONTAINERART, ResourcePurpose::Thumbnail);
    fileCover->addAttribute(ResourceAttribute::RESOURCE_FILE, (folderPath / "cover.jpg").string());
    folder->addResource(fileCover);

    const auto makeTrack = [&](int id, int trackNumber, bool embedded) {
        auto track = std::make_shared<CdsItem>(CdsEntryType::File);
        track->setID(id);
        track->setClass(UPNP_CLASS_MUSIC_TRACK);
        track->setTrackNumber(trackNumber);
        track->setLocation(folderPath / (std::to_string(id) + ".flac"), CdsEntryType::File);
        track->addResource(std::make_shared<CdsResource>(ContentHandler::DEFAULT, ResourcePurpose::Content));
        auto art = std::make_shared<CdsResource>(embedded ? ContentHandler::ID3 : ContentHandler::FANART, ResourcePurpose::Thumbnail);
        if (!embedded)
            art->addAttribute(ResourceAttribute::RESOURCE_FILE, (folderPath / "cover.jpg").string());
        track->addResource(art);
        database->tracks.emplace(id, track);
        return track;
    };
    const auto noCover = makeTrack(101, 1, false);
    const auto earlier = makeTrack(102, 2, true);
    const auto later = makeTrack(103, 3, true);

    ImportService importer(context, converters);
    EXPECT_TRUE(importer.preferMediaArtwork(earlier, later));
    EXPECT_FALSE(importer.preferMediaArtwork(noCover, earlier));
    auto scan = std::make_shared<AutoscanDirectory>(folderPath, AutoscanScanMode::Timed, false, false);
    importer.run(nullptr, scan, folderPath);

    const auto sourceId = [](const std::shared_ptr<CdsObject>& container) {
        const auto art = container->getResource(ResourcePurpose::Thumbnail);
        return art ? art->getAttribute(ResourceAttribute::FANART_OBJ_ID) : "";
    };
    const auto scanWith = [&](const auto& track) {
        importer.finishScan(folderPath, folder, std::chrono::seconds(1), track);
    };
    scanWith(noCover);
    EXPECT_EQ(folder->getResource(ResourcePurpose::Thumbnail)->getAttribute(ResourceAttribute::RESOURCE_FILE), (folderPath / "cover.jpg").string());
    importer.updateFolderArtwork(later, folder);
    EXPECT_EQ(sourceId(folder), "103");
    scanWith(earlier);
    EXPECT_EQ(sourceId(folder), "102");
    scanWith(later);
    EXPECT_EQ(sourceId(folder), "102");

    auto album = std::make_shared<CdsContainer>("Album", UPNP_CLASS_MUSIC_ALBUM);
    album->addResource(fileCover->clone());
    const std::vector<std::shared_ptr<CdsObject>> chain { album };
    std::vector<int> newIds;
    importer.addContainerTree(0, chain, noCover, newIds);
    EXPECT_EQ(database->container->getResource(ResourcePurpose::Thumbnail)->getAttribute(ResourceAttribute::RESOURCE_FILE), (folderPath / "cover.jpg").string());

    const auto addLayoutArtwork = [&](const auto& track) {
        album->setArtworkSource(track->getID());
        importer.addContainerTree(0, chain, nullptr, newIds);
    };
    addLayoutArtwork(earlier);
    EXPECT_EQ(sourceId(database->container), "102");
    addLayoutArtwork(later);
    EXPECT_EQ(sourceId(database->container), "102");

    database->tracks.erase(earlier->getID());
    scanWith(later);
    EXPECT_EQ(sourceId(folder), "103");
    addLayoutArtwork(later);
    EXPECT_EQ(sourceId(database->container), "103");
}

TEST_F(CoverArtPriorityTest, ArtworkPriorityUsesConfiguredFileOrderCaseAndTheme)
{
    const auto folderPath = directory / "album";
    const auto localCover = folderPath / "COVER.JPG";
    const auto pngCover = folderPath / "cover.png";
    const auto namedCover = folderPath / "Album.png";
    const auto folderCover = folderPath / "folder.png";
    const auto secondCover = folderPath / "cover.2.jpg";
    const auto themeCover = config->directory / "object.container.png";
    for (const auto& path : { localCover, pngCover, namedCover, folderCover, secondCover, folderPath / "cover.10.jpg", folderPath / "cover.txt", themeCover })
        std::ofstream(path) << "artwork";
    // Exact-path lookup follows the filesystem's casing rules, including on macOS.
    const auto lowercaseCover = folderPath / "cover.jpg";
    const auto exactLookup = fs::exists(lowercaseCover) ? lowercaseCover : folderCover;

    auto track = std::make_shared<CdsItem>(CdsEntryType::File);
    track->setID(101);
    track->setClass(UPNP_CLASS_MUSIC_TRACK);
    track->setLocation(folderPath / "track.flac", CdsEntryType::File);
    track->addResource(std::make_shared<CdsResource>(ContentHandler::FLAC, ResourcePurpose::Thumbnail));
    const auto scan = std::make_shared<AutoscanDirectory>(folderPath, AutoscanScanMode::Timed, false, false);

    struct TestCase {
        std::string_view priority;
        std::vector<std::string> files;
        fs::path path;
        bool caseSensitive = false;
        std::optional<bool> folderCaseSensitive = std::nullopt;
    };
    for (const auto& [priority, files, path, caseSensitive, folderCaseSensitive] : std::vector<TestCase> {
             { "pattern, embedded", { "cover.jpg", "folder.png" }, localCover },
             { "pattern, embedded", { "cover.{png, jpg}", "folder.png" }, pngCover },
             { "pattern, embedded", { "cover.{jpg, png}", "folder.png" }, localCover },
             { "pattern, embedded", { "%filename%.{jpg,png}" }, namedCover },
             { "pattern, embedded", { "cover.txt", "folder.png", "cover.jpg" }, folderCover },
             { "external, PATTERN, embedded", { "cover.2.jpg", "cover.10.jpg" }, secondCover },
             { ", EXTERNAL, PATTERN, ,", { "cover.txt", "missing.jpg", "object.container.png" }, themeCover },
             { "pattern, embedded", { "COVER.JPG" }, localCover, true },
             { "pattern, embedded", { "cover.jpg", "folder.png" }, exactLookup, true },
             { "pattern, embedded", { "cover.jpg", "folder.png" }, localCover, true, false },
             { "pattern, embedded", { "cover.jpg", "folder.png" }, exactLookup, false, true } }) {
        SCOPED_TRACE(priority);
        SCOPED_TRACE(path.string());
        config->priority = priority;
        config->files = files;
        config->caseSensitive = caseSensitive;
        config->directoryTweaks.reset();
        if (folderCaseSensitive.has_value()) {
            auto tweak = std::make_shared<DirectoryTweak>(folderPath, false);
            tweak->setCaseSensitive(*folderCaseSensitive);
            config->directoryTweaks = std::make_shared<DirectoryConfigList>();
            config->directoryTweaks->add(tweak);
        }
        ImportService importer(context, converters);
        importer.run(nullptr, scan, folderPath);
        auto folder = std::make_shared<CdsContainer>(CdsEntryType::Directory);
        folder->setID(20);
        folder->setLocation(folderPath, CdsEntryType::Directory);
        importer.finishScan(folderPath, folder, std::chrono::seconds(1), track);

        auto album = std::make_shared<CdsContainer>("Album", UPNP_CLASS_MUSIC_ALBUM);
        album->setArtworkSource(track->getID());
        std::vector<int> newIds;
        importer.addContainerTree(0, { album }, track, newIds);

        for (const auto& container : { folder, database->container }) {
            ASSERT_NE(container, nullptr);
            SCOPED_TRACE(container->getLocation().string());
            auto artwork = container->getResource(ResourcePurpose::Thumbnail);
            ASSERT_NE(artwork, nullptr);
            EXPECT_EQ(artwork->getAttribute(ResourceAttribute::RESOURCE_FILE), path.string());
        }
    }
    // The class fallback introduced by container icon themes is shared with
    // priority selection, without modifying the container being inspected.
    config->files = { "%upnpclass%.png" };
    const auto typedCover = config->directory / "object.container.album.png";
    std::ofstream(typedCover) << "album fallback";
    ContainerArtHandler handler(context);
    auto album = std::make_shared<CdsContainer>("Album", "object.container.album.musicAlbum");
    auto fallback = handler.findThemeArtwork(album);
    ASSERT_NE(fallback, nullptr);
    EXPECT_EQ(fallback->getAttribute(ResourceAttribute::RESOURCE_FILE), typedCover.string());
    EXPECT_EQ(fallback->getOption(RESOURCE_OPTION_CONTAINER_ART_FALLBACK), "yes");
    EXPECT_EQ(album->getResource(ResourcePurpose::Thumbnail), nullptr);
}

TEST_F(CoverArtPriorityTest, BoxCoverIsUsedOnlyForVirtualAlbum)
{
    config->priority = "pattern, embedded";
    config->files = { "cover.jpg" };
    const auto albumFolder = directory / "album";
    const auto boxCover = albumFolder / "cover.jpg";
    std::ofstream(boxCover) << "box artwork";

    ImportService importer(context, converters);
    const auto scan = std::make_shared<AutoscanDirectory>(albumFolder, AutoscanScanMode::Timed, true, false);
    importer.run(nullptr, scan, albumFolder);

    auto album = std::make_shared<CdsContainer>("Album", UPNP_CLASS_MUSIC_ALBUM);
    std::vector<int> newIds;
    const auto addToAlbum = [&](const auto& track) {
        album->setArtworkSource(track->getID());
        importer.addContainerTree(0, { album }, track, newIds);
        return database->container->getResource(ResourcePurpose::Thumbnail);
    };

    for (int part : { 1, 2 }) {
        const auto discFolder = albumFolder / ("CD " + std::to_string(part));
        fs::create_directories(discFolder);
        auto track = std::make_shared<CdsItem>(CdsEntryType::File);
        track->setID(100 + part);
        track->setClass(UPNP_CLASS_MUSIC_TRACK);
        track->setPartNumber(part);
        track->setTrackNumber(1);
        track->setLocation(discFolder / "track.flac", CdsEntryType::File);
        auto embedded = std::make_shared<CdsResource>(ContentHandler::FLAC, ResourcePurpose::Thumbnail);
        track->addResource(embedded);
        database->tracks.emplace(track->getID(), track);

        auto disc = std::make_shared<CdsContainer>(CdsEntryType::Directory);
        disc->setID(20 + part);
        disc->setLocation(discFolder, CdsEntryType::Directory);
        importer.finishScan(discFolder, disc, std::chrono::seconds(1), track);

        const auto albumArt = addToAlbum(track);
        ASSERT_NE(albumArt, nullptr);
        EXPECT_EQ(albumArt->getAttribute(ResourceAttribute::RESOURCE_FILE), boxCover.string());
        const auto discArt = disc->getResource(ResourcePurpose::Thumbnail);
        ASSERT_NE(discArt, nullptr);
        EXPECT_EQ(discArt->getAttribute(ResourceAttribute::FANART_OBJ_ID), std::to_string(track->getID()));
        EXPECT_EQ(track->getResource(ResourcePurpose::Thumbnail), embedded);
    }

    const auto firstTrack = std::static_pointer_cast<CdsItem>(database->tracks.at(101));
    firstTrack->setPartNumber(2);
    const auto mismatched = addToAlbum(firstTrack);
    ASSERT_NE(mismatched, nullptr);
    EXPECT_TRUE(mismatched->getAttribute(ResourceAttribute::RESOURCE_FILE).empty());
    EXPECT_EQ(mismatched->getAttribute(ResourceAttribute::FANART_OBJ_ID), "101");
}

TEST_F(CoverArtPriorityTest, VideoDirectorySeparatesEmbeddedPostersFromPreviews)
{
    const auto folderPath = directory / "album";
    const auto makeVideo = [&](int id, const std::vector<ContentHandler>& handlers) {
        auto video = std::make_shared<CdsItem>(CdsEntryType::File);
        video->setID(id);
        video->setClass(UPNP_CLASS_VIDEO_ITEM);
        video->setLocation(folderPath / (std::to_string(id) + ".mkv"), CdsEntryType::File);
        for (auto handler : handlers)
            video->addResource(std::make_shared<CdsResource>(handler, ResourcePurpose::Thumbnail));
        database->tracks.emplace(id, video);
        return video;
    };
    const auto preview = makeVideo(101, { ContentHandler::FFTH });
    const auto earlier = makeVideo(102, { ContentHandler::MATROSKA });
    const auto later = makeVideo(103, { ContentHandler::MATROSKA, ContentHandler::FFTH });

    auto folder = std::make_shared<CdsContainer>(CdsEntryType::Directory);
    folder->setID(20);
    folder->setLocation(folderPath, CdsEntryType::Directory);

    config->priority = "embedded, preview";
    ImportService importer(context, converters);
    importer.run(nullptr);
    EXPECT_TRUE(importer.preferMediaArtwork(earlier, preview));
    EXPECT_TRUE(importer.preferMediaArtwork(earlier, later));

    const auto assertSource = [](const std::shared_ptr<CdsContainer>& target, ContentHandler handler, std::string_view id) {
        auto artwork = target->getResource(ResourcePurpose::Thumbnail);
        ASSERT_NE(artwork, nullptr);
        EXPECT_EQ(artwork->getHandlerType(), handler);
        EXPECT_EQ(artwork->getAttribute(ResourceAttribute::FANART_OBJ_ID), id);
    };

    // An unmarked virtual ancestor inherits the preview before the video directory is handled.
    auto ancestor = std::make_shared<CdsContainer>("Ancestor");
    std::vector<int> inheritedIds;
    importer.addContainerTree(20, { ancestor }, preview, inheritedIds);
    assertSource(database->container, ContentHandler::FFTH, "101");
    EXPECT_TRUE(preview->getResource(ResourcePurpose::Thumbnail)->getAttribute(ResourceAttribute::FANART_OBJ_ID).empty());

    importer.updateFolderArtwork(preview, folder);
    assertSource(folder, ContentHandler::FFTH, "101");
    importer.updateFolderArtwork(later, folder);
    assertSource(folder, ContentHandler::MATROSKA, "103");
    importer.updateFolderArtwork(earlier, folder);
    importer.updateFolderArtwork(later, folder);
    assertSource(folder, ContentHandler::MATROSKA, "102");

    auto virtualFolder = std::make_shared<CdsContainer>("Video directory");
    virtualFolder->setArtworkSource(later->getID());
    std::vector<int> newIds;
    importer.addContainerTree(0, { virtualFolder }, later, newIds);
    assertSource(database->container, ContentHandler::MATROSKA, "103");

    std::ofstream(config->directory / "object.container.png") << "theme";
    config->priority = "embedded";
    ImportService withoutPreview(context, converters);
    withoutPreview.run(nullptr);
    auto anotherFolder = std::make_shared<CdsContainer>(CdsEntryType::Directory);
    anotherFolder->setID(21);
    anotherFolder->setLocation(folderPath, CdsEntryType::Directory);
    withoutPreview.updateFolderArtwork(preview, anotherFolder);
    assertSource(anotherFolder, ContentHandler::CONTAINERART, "");

    config->priority = "preview, embedded";
    ImportService previewFirst(context, converters);
    previewFirst.run(nullptr);
    EXPECT_TRUE(previewFirst.preferMediaArtwork(preview, later));
    previewFirst.updateFolderArtwork(later, anotherFolder);
    previewFirst.updateFolderArtwork(preview, anotherFolder);
    previewFirst.updateFolderArtwork(earlier, anotherFolder);
    assertSource(anotherFolder, ContentHandler::FFTH, "101");
}
