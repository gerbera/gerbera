/*GRB*
    Gerbera - https://gerbera.io/

    Copyright (C) 2026 Gerbera Contributors

    Gerbera is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.
*/

#include "config/config_definition.h"
#include "config/setup/config_setup_dictionary.h"
#include "metadata/metadata_enums.h"

#include <gtest/gtest.h>

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

#ifdef HAVE_TAGLIB
#include "../mock/config_mock.h"
#include "cds/cds_item.h"
#include "cds/cds_resource.h"
#include "context.h"
#include "iohandler/io_handler.h"
#include "metadata/taglib_handler.h"
#include "util/string_converter.h"

#include <array>
#include <gmock/gmock.h>
#include <taglib.h>

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
        return option == ConfigVal::IMPORT_LIBOPTS_ID3_ENABLED || ConfigMock::getBoolOption(option);
    }

    std::map<std::string, std::string> getDictionaryOption(ConfigVal option) const override
    {
        if (option == ConfigVal::IMPORT_MAPPINGS_MIMETYPE_TO_CONTENTTYPE_LIST)
            return defaultMappings(option);
        return ConfigMock::getDictionaryOption(option);
    }
};
}

class VideoTagLibMetadataTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        auto config = std::make_shared<VideoMetadataConfig>();
        auto converter = std::make_shared<ConverterManager>(config);
        context = std::make_shared<Context>(nullptr, config, nullptr, nullptr, nullptr, nullptr, converter);
    }

    std::shared_ptr<CdsItem> read(const fs::path& path, const std::string& mimeType, const std::string& upnpClass)
    {
        auto item = std::make_shared<CdsItem>(CdsEntryType::File);
        item->setLocation(fs::absolute(path));
        item->setMimeType(mimeType);
        item->setClass(upnpClass);
        auto resource = std::make_shared<CdsResource>(ContentHandler::DEFAULT, ResourcePurpose::Content);
        item->addResource(resource);
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
#else
    GTEST_SKIP() << "Matroska/WebM extraction requires TagLib 2.2";
#endif
}
#endif
