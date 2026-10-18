/*GRB*
    Gerbera - https://gerbera.io/

    Copyright (C) 2026 Gerbera Contributors

    Gerbera is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.
*/

#include "../mock/config_mock.h"
#include "cds/cds_item.h"
#include "cds/cds_resource.h"
#include "config/config_definition.h"
#include "config/config_option_enum.h"
#include "config/result/directory_tweak.h"
#include "context.h"
#include "iohandler/io_handler.h"
#include "metadata/ffmpeg_handler.h"
#include "metadata/metadata_enums.h"
#include "metadata/metadata_service.h"
#include "metadata/taglib_handler.h"
#include "util/mime.h"
#include "util/string_converter.h"
#include "util/tools.h"

#include <algorithm>
#include <array>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#ifdef HAVE_TAGLIB
#include <mp4file.h>
#endif

#ifdef HAVE_TAGLIB
namespace {
class BpmConfig final : public ConfigMock {
public:
    bool preferAccuracy = true;
    BpmSource preferConflict = BpmSource::BpmTag;

    std::string getOption(ConfigVal option) const override
    {
#ifdef HAVE_MAGIC
        if (option == ConfigVal::IMPORT_MAGIC_FILE)
            return {};
#endif
        if (option == ConfigVal::IMPORT_LIBOPTS_ENTRY_SEP)
            return "; ";
        if (option == ConfigVal::IMPORT_LIBOPTS_ENTRY_LEGACY_SEP)
            return "\n";
        return "UTF-8";
    }

    bool getBoolOption(ConfigVal option) const override
    {
        if (option == ConfigVal::IMPORT_LIBOPTS_ID3_PREFER_HIGHEST_BPM_ACCURACY)
            return preferAccuracy;
        if (option == ConfigVal::IMPORT_LIBOPTS_ID3_ENABLED)
            return true;
        return ConfigMock::getBoolOption(option);
    }

    std::shared_ptr<ConfigOption> getConfigOption(ConfigVal option) const override
    {
        if (option == ConfigVal::IMPORT_LIBOPTS_ID3_PREFER_AMBIGOUS_BPM_VALUE)
            return std::make_shared<EnumOption<BpmSource>>(preferConflict, "");
        return ConfigMock::getConfigOption(option);
    }

    std::map<std::string, std::string> getDictionaryOption(ConfigVal option) const override
    {
        if (option == ConfigVal::IMPORT_MAPPINGS_MIMETYPE_TO_CONTENTTYPE_LIST)
            return { { "audio/mp4", CONTENT_TYPE_MP4 }, { "video/mp4", CONTENT_TYPE_MP4 }, { "video/x-m4v", CONTENT_TYPE_MP4 }, { "video/x-ms-wmv", CONTENT_TYPE_ASF } };
        if (option == ConfigVal::IMPORT_LIBOPTS_ID3_METADATA_TAGS_LIST)
            return { { "BPM", "upnp:beatsPerMinute" }, { "INITIALKEY", "upnp:initialKey" } };
        return ConfigMock::getDictionaryOption(option);
    }

    std::shared_ptr<DirectoryConfigList> getDirectoryTweakOption(ConfigVal) const override
    {
        return std::make_shared<DirectoryConfigList>();
    }
};

struct BpmCase {
    const char* name;
    bool enabled;
    BpmSource conflict;
    int tmpo;
    const char* bpm;
    const char* expected;
};
}

class BpmMetadataTest : public ::testing::TestWithParam<BpmCase> {
protected:
    void SetUp() override
    {
        config = std::make_shared<BpmConfig>();
        auto converter = std::make_shared<ConverterManager>(config);
        auto mime = std::make_shared<Mime>(config);
        auto definition = std::make_shared<ConfigDefinition>();
        definition->init(definition);
        context = std::make_shared<Context>(definition, config, nullptr, mime, nullptr, nullptr, converter);
    }

    void TearDown() override
    {
        for (const auto& path : temporaryFiles)
            fs::remove(path);
    }

    fs::path makeFile(const char* fixture, int tmpo, const char* bpm, const char* key = "----:com.apple.iTunes:BPM")
    {
        const auto path = fs::temp_directory_path() / ("gerbera-bpm-" + generateRandomId() + fs::path(fixture).extension().string());
        temporaryFiles.push_back(path);
        fs::copy_file(fs::path("fixtures") / fixture, path);
        TagLib::MP4::File file(path.c_str());
        EXPECT_TRUE(file.isValid());
        auto* tag = file.tag();
        tag->removeItem("tmpo");
        tag->removeItem("----:com.apple.iTunes:BPM");
        if (tmpo >= 0)
            tag->setItem("tmpo", TagLib::MP4::Item(tmpo));
        if (*bpm)
            tag->setItem(key, TagLib::MP4::Item(TagLib::StringList(TagLib::String(bpm, TagLib::String::UTF8))));
        EXPECT_TRUE(file.save());
        return path;
    }

    std::shared_ptr<CdsItem> makeItem(const fs::path& path, const char* mime = "audio/mp4")
    {
        auto item = std::make_shared<CdsItem>(CdsEntryType::File);
        item->setLocation(fs::absolute(path), CdsEntryType::File);
        item->setMimeType(mime);
        item->setClass(std::string(mime).find("video/") == 0 ? "object.item.videoItem" : "object.item.audioItem.musicTrack");
        item->addResource(std::make_shared<CdsResource>(ContentHandler::DEFAULT, ResourcePurpose::Content));
        return item;
    }

    void read(const std::shared_ptr<CdsItem>& item)
    {
        TagLibHandler handler(context);
        std::vector<int> ids;
        ASSERT_TRUE(handler.fillMetadata(item, ids));
    }

    std::shared_ptr<BpmConfig> config;
    std::shared_ptr<Context> context;
    std::vector<fs::path> temporaryFiles;
};

TEST_P(BpmMetadataTest, SelectsMp4SourcesForAudioAndVideo)
{
    const auto& test = GetParam();
    config->preferConflict = test.conflict;
    for (const auto* fixture : { "bpm-accuracy.m4a", "bpm-accuracy.mp4" }) {
        SCOPED_TRACE(fixture);
        // Older TagLib versions do not expose the freeform INITIALKEY property.
        // Preserve whatever the unchanged reader supplies for the original file.
        config->preferAccuracy = false;
        auto original = makeItem(fs::path("fixtures") / fixture);
        read(original);
        const auto originalKey = original->getMetaData("upnp:initialKey");
        config->preferAccuracy = test.enabled;
        const auto path = makeFile(fixture, test.tmpo, test.bpm);
        for (const auto* mime : { "audio/mp4", "video/mp4", "video/x-m4v" }) {
            SCOPED_TRACE(mime);
            auto item = makeItem(path, mime);
            read(item);
            EXPECT_EQ(item->getMetaData("upnp:beatsPerMinute"), test.expected);
            EXPECT_EQ(item->getMetaData("upnp:initialKey"), originalKey);
            if (test.enabled) {
                EXPECT_EQ(std::count_if(item->getMetaData().begin(), item->getMetaData().end(), [](const auto& entry) {
                    return entry.first == "upnp:beatsPerMinute";
                }),
                    *test.expected ? 1 : 0);
            }
        }
    }
}

INSTANTIATE_TEST_SUITE_P(Sources, BpmMetadataTest, ::testing::Values(BpmCase { "Disabled", false, BpmSource::BpmTag, 123, "123.5", "123" }, BpmCase { "DisabledConflict", false, BpmSource::BpmTag, 123, "128.5", "123" }, BpmCase { "Fractional", true, BpmSource::BpmTag, 123, "123.5", "123.5" }, BpmCase { "FractionalDespiteTmpoPreference", true, BpmSource::Tmpo, 123, "123.5", "123.5" }, BpmCase { "RoundedUp", true, BpmSource::Tmpo, 124, "123.5", "123.5" }, BpmCase { "BoundaryBelow", true, BpmSource::Tmpo, 123, "123.999", "123.999" }, BpmCase { "BoundaryAbove", true, BpmSource::Tmpo, 123, "122.001", "122.001" }, BpmCase { "ConflictTag", true, BpmSource::BpmTag, 123, "128.5", "128.5" }, BpmCase { "ConflictTmpo", true, BpmSource::Tmpo, 123, "128.5", "123" }, BpmCase { "ExactlyOneTag", true, BpmSource::BpmTag, 123, "124", "124" }, BpmCase { "ExactlyOneTmpo", true, BpmSource::Tmpo, 123, "124", "123" }, BpmCase { "ExactlyOneLowerTmpo", true, BpmSource::Tmpo, 123, "122", "123" }, BpmCase { "HalfTempo", true, BpmSource::Tmpo, 120, "60", "120" }, BpmCase { "DoubleTempo", true, BpmSource::BpmTag, 120, "240", "240" }, BpmCase { "EqualValues", true, BpmSource::Tmpo, 123, "123.000", "123.000" }, BpmCase { "TmpoOnly", true, BpmSource::BpmTag, 123, "", "123" }, BpmCase { "TagOnly", true, BpmSource::Tmpo, -1, "123.5", "123.5" }, BpmCase { "Neither", true, BpmSource::BpmTag, -1, "", "" }, BpmCase { "ZeroTmpo", true, BpmSource::Tmpo, 0, "123.5", "123.5" }, BpmCase { "InvalidTag", true, BpmSource::BpmTag, 123, "invalid", "123" }, BpmCase { "TrailingText", true, BpmSource::BpmTag, 123, "123.5 BPM", "123" }, BpmCase { "ZeroTag", true, BpmSource::BpmTag, 123, "0", "123" }, BpmCase { "NegativeTag", true, BpmSource::BpmTag, 123, "-123.5", "123" }, BpmCase { "NanTag", true, BpmSource::BpmTag, 123, "NaN", "123" }, BpmCase { "InfiniteTag", true, BpmSource::BpmTag, 123, "inf", "123" }, BpmCase { "OverflowTag", true, BpmSource::BpmTag, 123, "1e999", "123" }, BpmCase { "DecimalComma", true, BpmSource::BpmTag, 123, "123,5", "123" }, BpmCase { "Whitespace", true, BpmSource::BpmTag, 123, "  123.5  ", "123.5" }, BpmCase { "OnlyInvalid", true, BpmSource::BpmTag, -1, "invalid", "" }),
    [](const auto& info) { return info.param.name; });

TEST_F(BpmMetadataTest, MatchesLowercaseFreeformName)
{
    auto item = makeItem(makeFile("bpm-accuracy.m4a", 123, "123.5", "----:com.apple.iTunes:bpm"));
    read(item);
    EXPECT_EQ(item->getMetaData("upnp:beatsPerMinute"), "123.5");
}

TEST_F(BpmMetadataTest, LeavesNonMp4BpmAndMetadataUnchanged)
{
    config->preferAccuracy = false;
    auto previous = makeItem("fixtures/multiple-genres.wmv", "video/x-ms-wmv");
    read(previous);
    config->preferAccuracy = true;
    config->preferConflict = BpmSource::Tmpo;
    auto current = makeItem("fixtures/multiple-genres.wmv", "video/x-ms-wmv");
    read(current);
    EXPECT_EQ(current->getMetaData("upnp:beatsPerMinute"), "123.5");
    EXPECT_EQ(current->getMetaData(), previous->getMetaData());
    EXPECT_EQ(current->getResource(ContentHandler::DEFAULT)->getAttributes(), previous->getResource(ContentHandler::DEFAULT)->getAttributes());
}

TEST_F(BpmMetadataTest, PreservesOtherMetadataResourcesAndArtwork)
{
    const auto path = makeFile("bpm-accuracy.m4a", 123, "123.5");
    config->preferAccuracy = false;
    auto previous = makeItem(path);
    read(previous);
    config->preferAccuracy = true;
    auto current = makeItem(path);
    read(current);
    auto withoutBpm = [](const auto& item) {
        auto values = item->getMetaData();
        values.erase(std::remove_if(values.begin(), values.end(), [](const auto& entry) {
            return entry.first == "upnp:beatsPerMinute";
        }),
            values.end());
        return values;
    };
    EXPECT_EQ(withoutBpm(current), withoutBpm(previous));
    EXPECT_EQ(current->getResource(ContentHandler::DEFAULT)->getAttributes(), previous->getResource(ContentHandler::DEFAULT)->getAttributes());
    EXPECT_THAT(current->getMetaGroup(MetadataFields::M_GENRE), ::testing::UnorderedElementsAre("Techno", "Hard Trance"));
#ifdef HAVE_MAGIC
    auto previousArtwork = previous->getResource(ContentHandler::ID3);
    ASSERT_NE(previousArtwork, nullptr);
    auto artwork = current->getResource(ContentHandler::ID3);
    ASSERT_NE(artwork, nullptr);
    EXPECT_EQ(artwork->getAttributes(), previousArtwork->getAttributes());
    TagLibHandler handler(context);
    auto stream = handler.serveContent(current, artwork);
    ASSERT_NE(stream, nullptr);
    stream->open(UPNP_READ);
    std::array<std::byte, 2> header {};
    ASSERT_EQ(stream->read(header.data(), header.size()), 2);
    EXPECT_EQ(header[0], std::byte { 0xff });
    EXPECT_EQ(header[1], std::byte { 0xd8 });
    stream->close();
#else
    // MP4 cover MIME detection already requires libmagic in the unchanged reader.
    EXPECT_EQ(previous->getResource(ContentHandler::ID3), nullptr);
    EXPECT_EQ(current->getResource(ContentHandler::ID3), nullptr);
#endif
}

TEST_F(BpmMetadataTest, ReimportReplacesPreviousBpm)
{
    auto item = makeItem(makeFile("bpm-accuracy.m4a", 123, "123.5"));
    config->preferAccuracy = false;
    std::vector<int> ids;
    MetadataService previous(context, nullptr);
    ASSERT_TRUE(previous.extractMetaData(item, fs::directory_entry(item->getLocation()), ids));
    EXPECT_EQ(item->getMetaData("upnp:beatsPerMinute"), "123");
    config->preferAccuracy = true;
    MetadataService current(context, nullptr);
    ASSERT_TRUE(current.extractMetaData(item, fs::directory_entry(item->getLocation()), ids));
    EXPECT_EQ(item->getMetaData("upnp:beatsPerMinute"), "123.5");
    EXPECT_EQ(std::count_if(item->getMetaData().begin(), item->getMetaData().end(), [](const auto& entry) {
        return entry.first == "upnp:beatsPerMinute";
    }),
        1);
}

#endif

#if defined(HAVE_TAGLIB) && defined(HAVE_FFMPEG)
TEST_F(BpmMetadataTest, LaterFfmpegReaderPreservesSelectedConflictSource)
{
    config->preferConflict = BpmSource::Tmpo;
    auto item = makeItem(makeFile("bpm-accuracy.mp4", 123, "128.5"), "video/mp4");
    read(item);
    // Enable FFmpeg only after the TagLib selection has been made.
    class FfmpegConfig final : public ConfigMock {
    public:
        bool getBoolOption(ConfigVal option) const override { return option == ConfigVal::IMPORT_LIBOPTS_FFMPEG_ENABLED; }
        std::string getOption(ConfigVal) const override { return "UTF-8"; }
        std::map<std::string, std::string> getDictionaryOption(ConfigVal option) const override
        {
            if (option == ConfigVal::IMPORT_LIBOPTS_FFMPEG_METADATA_TAGS_LIST)
                return { { "bpm", "upnp:beatsPerMinute" } };
            return ConfigMock::getDictionaryOption(option);
        }
        std::shared_ptr<DirectoryConfigList> getDirectoryTweakOption(ConfigVal) const override { return std::make_shared<DirectoryConfigList>(); }
    };
    auto ffmpegConfig = std::make_shared<FfmpegConfig>();
    auto converter = std::make_shared<ConverterManager>(ffmpegConfig);
    auto ffmpegContext = std::make_shared<Context>(nullptr, ffmpegConfig, nullptr, nullptr, nullptr, nullptr, converter);
    FfmpegHandler handler(ffmpegContext);
    std::vector<int> ids;
    ASSERT_TRUE(handler.fillMetadata(item, ids));
    EXPECT_EQ(item->getMetaData("upnp:beatsPerMinute"), "123");
}
#endif
