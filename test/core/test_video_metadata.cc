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
#include "config/config_val.h"
#include "config/result/directory_tweak.h"
#include "config/setup/config_setup_dictionary.h"
#include "context.h"
#include "iohandler/io_handler.h"
#include "metadata/ffmpeg_handler.h"
#include "metadata/metadata_enums.h"
#include "metadata/taglib_handler.h"
#include "util/string_converter.h"

#include <array>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <pugixml.hpp>
#ifdef HAVE_TAGLIB
#include <taglib.h>
#endif

namespace {
std::map<std::string, std::string> defaultMappings(ConfigVal option)
{
    auto definition = std::make_shared<ConfigDefinition>();
    definition->init(definition);
    auto setup = definition->findConfigSetup<ConfigDictionarySetup>(option);
    return setup->getXmlContent({}, nullptr);
}
}

TEST(VideoMetadataMappingsTest, SelectsReadersForWebmAndWmv)
{
    auto extensions = defaultMappings(ConfigVal::IMPORT_MAPPINGS_EXTENSION_TO_MIMETYPE_LIST);
    auto contentTypes = defaultMappings(ConfigVal::IMPORT_MAPPINGS_MIMETYPE_TO_CONTENTTYPE_LIST);
    ASSERT_EQ(extensions.count("webm"), 1);
    ASSERT_EQ(extensions.count("wmv"), 1);
    EXPECT_EQ(extensions.at("webm"), "video/webm");
    EXPECT_EQ(extensions.at("wmv"), "video/x-ms-wmv");
    ASSERT_EQ(contentTypes.count(extensions.at("webm")), 1);
    ASSERT_EQ(contentTypes.count(extensions.at("wmv")), 1);
    EXPECT_EQ(contentTypes.at(extensions.at("webm")), CONTENT_TYPE_MKV);
    EXPECT_EQ(contentTypes.at(extensions.at("wmv")), CONTENT_TYPE_ASF);
}

namespace {
class VideoMetadataConfig final : public ConfigMock {
public:
    std::string getOption(ConfigVal option) const override
    {
        if (option == ConfigVal::IMPORT_LIBOPTS_ENTRY_SEP)
            return "; ";
        if (option == ConfigVal::IMPORT_LIBOPTS_ENTRY_LEGACY_SEP)
            return "\n";
        return "UTF-8";
    }

    bool getBoolOption(ConfigVal option) const override
    {
#ifdef HAVE_FFMPEG
        if (option == ConfigVal::IMPORT_LIBOPTS_FFMPEG_ENABLED || option == ConfigVal::IMPORT_LIBOPTS_FFMPEG_STREAMS_ENABLED)
            return true;
#endif
#ifdef HAVE_TAGLIB
        if (option == ConfigVal::IMPORT_LIBOPTS_ID3_ENABLED)
            return true;
#endif
        return ConfigMock::getBoolOption(option);
    }

    std::map<std::string, std::string> getDictionaryOption(ConfigVal option) const override
    {
        if (option == ConfigVal::IMPORT_MAPPINGS_MIMETYPE_TO_CONTENTTYPE_LIST)
            return defaultMappings(option);
#ifdef HAVE_TAGLIB
        if (option == ConfigVal::IMPORT_LIBOPTS_ID3_METADATA_TAGS_LIST)
            return { { "BPM", "upnp:beatsPerMinute" }, { "INITIALKEY", "upnp:initialKey" } };
#endif
#ifdef HAVE_FFMPEG
        if (option == ConfigVal::IMPORT_LIBOPTS_FFMPEG_METADATA_TAGS_LIST)
            return { { "bpm", "upnp:beatsPerMinute" }, { "initialkey", "upnp:initialKey" }, { "custom_probe", "upnp:userAnnotation" },
                { "wm/beatsperminute", "upnp:beatsPerMinute" }, { "wm/initialkey", "upnp:initialKey" } };
#endif
        return ConfigMock::getDictionaryOption(option);
    }

    std::vector<std::string> getArrayOption(ConfigVal option) const override
    {
#ifdef HAVE_TAGLIB
        if (option == ConfigVal::IMPORT_LIBOPTS_ID3_AUXDATA_TAGS_LIST)
            return { "TRACKNUMBER" };
#endif
#ifdef HAVE_FFMPEG
        if (option == ConfigVal::IMPORT_LIBOPTS_FFMPEG_AUXDATA_TAGS_LIST)
            return { "custom_probe" };
#endif
        return ConfigMock::getArrayOption(option);
    }

    std::shared_ptr<DirectoryConfigList> getDirectoryTweakOption(ConfigVal) const override
    {
        return std::make_shared<DirectoryConfigList>();
    }
};
}

#ifdef HAVE_TAGLIB
class VideoTagLibMetadataTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        auto config = std::make_shared<VideoMetadataConfig>();
        auto converter = std::make_shared<ConverterManager>(config);
        context = std::make_shared<Context>(nullptr, config, nullptr, nullptr, nullptr, nullptr, converter);
    }

    std::shared_ptr<CdsItem> makeItem(const fs::path& path, const std::string& mimeType, const std::string& upnpClass)
    {
        auto item = std::make_shared<CdsItem>(CdsEntryType::File);
        item->setLocation(fs::absolute(path), CdsEntryType::File);
        item->setMimeType(mimeType);
        item->setClass(upnpClass);
        auto resource = std::make_shared<CdsResource>(ContentHandler::DEFAULT, ResourcePurpose::Content);
        resource->addAttribute(ResourceAttribute::PROTOCOLINFO, "http-get:*:" + mimeType + ":*");
        resource->addAttribute(ResourceAttribute::SIZE, static_cast<long long>(fs::file_size(path)));
        item->addResource(resource);
        return item;
    }

    std::shared_ptr<CdsItem> read(const fs::path& path, const std::string& mimeType, const std::string& upnpClass)
    {
        auto item = makeItem(path, mimeType, upnpClass);
        TagLibHandler handler(context);
        auto mappings = defaultMappings(ConfigVal::IMPORT_MAPPINGS_MIMETYPE_TO_CONTENTTYPE_LIST);
        auto mapping = mappings.find(mimeType);
        EXPECT_NE(mapping, mappings.end());
        if (mapping == mappings.end())
            return item;
        EXPECT_TRUE(handler.isSupported(mapping->second, false, mimeType, item->getMediaType()));
        std::vector<int> newIds;
        EXPECT_TRUE(handler.fillMetadata(item, newIds));
        EXPECT_THAT(item->getMetaGroup(MetadataFields::M_GENRE), ::testing::UnorderedElementsAre("Techno", "Hard Trance"));
        EXPECT_EQ(item->getMetaData(MetadataFields::M_TITLE), "Video metadata regression");
        EXPECT_EQ(item->getClass(), upnpClass);
        EXPECT_EQ(item->getMimeType(), mimeType);
        return item;
    }

    std::shared_ptr<Context> context;
};

TEST_F(VideoTagLibMetadataTest, PreservesRepeatedWmvGenresAndServesEmbeddedArtwork)
{
    auto item = read("fixtures/multiple-genres.wmv", "video/x-ms-wmv", "object.item.videoItem");
    auto artwork = item->getResource(ContentHandler::ID3);
    ASSERT_NE(artwork, nullptr);
    TagLibHandler handler(context);
    auto io = handler.serveContent(item, artwork);
    ASSERT_NE(io, nullptr);
    io->open(UPNP_READ);
    std::array<std::byte, 2> header {};
    ASSERT_EQ(io->read(header.data(), header.size()), 2);
    EXPECT_EQ(header[0], std::byte { 0xff });
    EXPECT_EQ(header[1], std::byte { 0xd8 });
    io->close();
}

TEST_F(VideoTagLibMetadataTest, ReadsExistingAsfAndWmaMappings)
{
    read("fixtures/multiple-genres.wmv", "video/x-ms-asf", "object.item.videoItem");
    read("fixtures/multiple-genres.wmv", "audio/x-ms-wma", "object.item.audioItem.musicTrack");
}

TEST_F(VideoTagLibMetadataTest, PreservesRepeatedWebmGenres)
{
#if TAGLIB_MAJOR_VERSION >= 2 && TAGLIB_MINOR_VERSION >= 2
    read("fixtures/multiple-genres.webm", "video/webm", "object.item.videoItem");
    read("fixtures/multiple-genres.webm", "video/x-matroska", "object.item.videoItem");
#else
    GTEST_SKIP() << "Matroska/WebM extraction requires TagLib 2.2";
#endif
}

#endif // HAVE_TAGLIB

#if defined(HAVE_TAGLIB) && defined(HAVE_FFMPEG)
class VideoTagLibFfmpegMetadataTest : public VideoTagLibMetadataTest {
protected:
    void expectFfmpegFallbackPreserved(const fs::path& path, const std::string& mimeType, int expectedTrackNumber = 7)
    {
        auto before = makeItem(path, mimeType, "object.item.videoItem");
        auto after = read(path, mimeType, "object.item.videoItem");
        FfmpegHandler ffmpeg(context);
        std::vector<int> newIds;
        ASSERT_TRUE(ffmpeg.fillMetadata(before, newIds));
        ASSERT_TRUE(ffmpeg.fillMetadata(after, newIds));

        // Compare every previously supplied non-genre value, including custom
        // metadata and auxdata, rather than only checking the new genres.
        const auto genreKey = MetaEnumMapper::getMetaFieldName(MetadataFields::M_GENRE);
        for (const auto& [field, value] : before->getMetaData()) {
            if (field != genreKey)
                EXPECT_THAT(after->getMetaGroup(field), ::testing::Contains(value)) << field;
        }
        for (const auto& [key, value] : before->getAuxData())
            EXPECT_EQ(after->getAuxData(key), value) << key;
        for (const auto& [attribute, value] : before->getResource(ContentHandler::DEFAULT)->getAttributes())
            EXPECT_EQ(after->getResource(ContentHandler::DEFAULT)->getAttribute(attribute), value) << static_cast<int>(attribute);

        EXPECT_EQ(after->getMetaData(MetadataFields::M_ARTIST), "Video artist");
        EXPECT_EQ(after->getMetaData(MetadataFields::M_ALBUMARTIST), "Video album artist");
        EXPECT_EQ(after->getMetaData(MetadataFields::M_ALBUM), "Video album");
        EXPECT_EQ(after->getMetaData(MetadataFields::M_DESCRIPTION), "Video comment");
        EXPECT_EQ(after->getMetaData(MetadataFields::M_DATE), "2026-01-01");
        EXPECT_EQ(after->getTrackNumber(), expectedTrackNumber);
        EXPECT_EQ(after->getPartNumber(), 2);
        EXPECT_EQ(after->getMetaData("upnp:beatsPerMinute"), "123.5");
        EXPECT_EQ(after->getMetaData("upnp:initialKey"), "Am");
        EXPECT_EQ(after->getMetaData("upnp:userAnnotation"), "fallback retained");
        EXPECT_EQ(after->getAuxData("custom_probe"), "fallback retained");
        EXPECT_THAT(after->getMetaGroup(MetadataFields::M_GENRE), ::testing::UnorderedElementsAre("Techno", "Hard Trance"));
        EXPECT_EQ(after->getClass(), before->getClass());
        EXPECT_EQ(after->getMimeType(), before->getMimeType());
    }
};

TEST_F(VideoTagLibFfmpegMetadataTest, PreservesWmvFfmpegTagsAndMediaAttributes)
{
    expectFfmpegFallbackPreserved("fixtures/multiple-genres.wmv", "video/x-ms-wmv");
}

TEST_F(VideoTagLibFfmpegMetadataTest, PreservesWebmFfmpegTagsAndMediaAttributes)
{
#if TAGLIB_MAJOR_VERSION >= 2 && TAGLIB_MINOR_VERSION >= 2
    expectFfmpegFallbackPreserved("fixtures/multiple-genres.webm", "video/webm");
#else
    GTEST_SKIP() << "Matroska/WebM extraction requires TagLib 2.2";
#endif
}

TEST_F(VideoTagLibFfmpegMetadataTest, PreservesWebmMetadataWithExistingTracknumberTag)
{
#if TAGLIB_MAJOR_VERSION >= 2 && TAGLIB_MINOR_VERSION >= 2
    // Keep the original fixture unchanged: TRACKNUMBER is exposed as a raw
    // property, but neither previous FFmpeg nor TagLib's track() maps it to
    // the numeric field. The routing change must still preserve its metadata.
    expectFfmpegFallbackPreserved("fixtures/tracknumber-tag.webm", "video/webm", 0);
    auto item = read("fixtures/tracknumber-tag.webm", "video/webm", "object.item.videoItem");
    EXPECT_EQ(item->getAuxData("TRACKNUMBER"), "7");
#else
    GTEST_SKIP() << "Matroska/WebM extraction requires TagLib 2.2";
#endif
}
#endif // HAVE_TAGLIB && HAVE_FFMPEG
